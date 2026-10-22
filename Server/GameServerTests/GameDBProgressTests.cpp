#include "Core/pch.h"
#include <gtest/gtest.h>
#include "GameDBTestFixture.h"
#include "Lobby/AccountDAO.h"
#include "Lobby/CharacterListDAO.h"
#include "DB/DAOCommon.h"
#include "DB/ProgressStorage.h"
#include "Game/Entities/PlayerSaveData.h"

/*--------------------------------------------------------------
    진행 불러오기와 저장의 SQL 확인 (실제 DB)

    입장의 불러오기와 접속 종료의 저장을 GameDB_Test에서 실제 SQL로 돌린다. 가짜 연결은 바인딩과 흐름만 본다.
    아이템 저장은 그 캐릭터의 아이템 전체를 MERGE 한 문장으로 맞추므로, 칸 맞바꾸기와 고유 제약,
    사본에 없는 행의 삭제, 배열 파라미터의 행 상한을 넘는 저장, 실패한 저장의 되돌림은 여기서만 확인된다.
    퀵슬롯도 같은 방식으로 저장하고, 빈 퀵슬롯은 행을 두지 않는다.

    테스트마다 다른 계정 번호와 캐릭터 번호를 쓰고, 아이템 UID는 캐릭터 번호에서 만든다. 그 계정의 행만 넣고 지운다.

    기본으로 꺼져 있다. 접속 문자열과 실행 필터는 GameDBTestFixture에 있다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 GOLD_CURRENCY_ID = 1;
    constexpr int32 START_MAP_ID = 1111;
    constexpr int32 SAVED_MAP_ID = 2222;
    constexpr int32 POTION_TEMPLATE_ID = 2000;
    constexpr int32 MISC_TEMPLATE_ID = 3000;
    constexpr int32 SWORD_TEMPLATE_ID = 1001;
    constexpr int32 MP_POTION_TEMPLATE_ID = 2001;

    QuickSlot ItemQuickSlot(int32 slotIndex, int32 templateId)
    {
        return QuickSlot{ slotIndex, QuickSlotRef{ QuickSlotRefType::ITEM, templateId } };
    }

    void AddItem(PlayerSaveData& data, ContainerType container, int32 slotIndex, int64 itemUid, int32 templateId, int32 count,
        optional<GearDetail> gear = nullopt)
    {
        data.progress.items.push_back(PlacedItem{ SlotAddress{ container, slotIndex }, ItemInstance{ itemUid, templateId, count, gear } });
    }

    void AddConsumable(PlayerSaveData& data, int32 slotId, int64 itemUid, int32 count = 1)
    {
        AddItem(data, ContainerType::INVENTORY_CONSUMABLE, slotId, itemUid, POTION_TEMPLATE_ID, count);
    }

    void AddInventoryGear(PlayerSaveData& data, int32 slotId, int64 itemUid, int32 enhanceLevel)
    {
        AddItem(data, ContainerType::INVENTORY_GEAR, slotId, itemUid, SWORD_TEMPLATE_ID, 1, GearDetail{ enhanceLevel, 0 });
    }

    void EquipWeapon(PlayerSaveData& data, int64 itemUid, int32 enhanceLevel)
    {
        AddItem(data, ContainerType::EQUIPPED, Protocol::GEAR_TYPE_WEAPON, itemUid, SWORD_TEMPLATE_ID, 1, GearDetail{ enhanceLevel, 0 });
    }

    // 불러온 아이템 목록에서 그 칸의 아이템. 없으면 nullptr.
    const ItemInstance* FindItem(const PlayerProgress& progress, ContainerType container, int32 slotIndex)
    {
        for (const PlacedItem& placed : progress.items)
        {
            if (placed.address == SlotAddress{ container, slotIndex })
                return &placed.item;
        }

        return nullptr;
    }

    // 불러온 아이템 가운데 그 컨테이너에 든 것의 개수.
    size_t CountItems(const PlayerProgress& progress, ContainerType container)
    {
        return ranges::count_if(progress.items, [container](const PlacedItem& placed) { return placed.address.container == container; });
    }
}

class DISABLED_GameDBProgressTest : public GameDBTestFixture
{
protected:
    void SetUp() override
    {
        GameDBTestFixture::SetUp();
        if (HasFatalFailure())
            return;

        LevelTemplate level1;
        level1.level = 1;
        level1.maxHp = 120;
        level1.maxMp = 45;
        level1.expRequirement = 1000;
        LevelTemplate level2 = level1;
        level2.level = 2;
        level2.expRequirement = 0;

        GamedataTables tables;
        tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ClassLevelTable({ level1, level2 });
        tables.goldCurrencyId = GOLD_CURRENCY_ID;
        tables.characterCreation.startMapId = START_MAP_ID;
        tables.characterCreation.startGold = 500;
        tables.characterCreation.defaultCharacterSlotCount = 4;
        Gamedata::Install(std::move(tables));
    }

    void TearDown() override
    {
        Gamedata::Install(GamedataTables());
        GameDBTestFixture::TearDown();
    }

    // 계정과 캐릭터 하나를 만들고, 테스트가 끝날 때 그 계정의 행을 모두 지운다.
    // 외래 키 때문에 자식 행부터 지운다(정리는 넣은 역순으로 돈다). ItemGear는 Item을 지우면 함께 지워진다.
    void CreateCharacter(int64 accountId, int64 characterId, const string& name)
    {
        const wstring id = to_wstring(accountId);
        const wstring characters = L"(SELECT CharacterId FROM dbo.[Character] WHERE AccountId = " + id + L")";
        AddCleanup(L"DELETE FROM dbo.Account WHERE AccountId = " + id);
        AddCleanup(L"DELETE FROM dbo.[Character] WHERE AccountId = " + id);
        AddCleanup(L"DELETE FROM dbo.CharacterState WHERE CharacterId IN " + characters);
        AddCleanup(L"DELETE FROM dbo.CharacterCurrency WHERE CharacterId IN " + characters);
        AddCleanup(L"DELETE FROM dbo.Item WHERE OwnerId IN " + characters);
        AddCleanup(L"DELETE FROM dbo.QuickSlot WHERE CharacterId IN " + characters);

        AccountDAO::RecordLogin(Connection(), accountId);

        Protocol::CharacterOverview character;
        character.set_class_(Protocol::CLASS_TYPE_WARRIOR);
        character.set_name(name);
        const CreateCharacterResult result = CharacterListDAO::CreateCharacter(Connection(), accountId, characterId, character);
        ASSERT_FALSE(result.rejection.has_value()) << "준비 단계의 캐릭터 생성이 거절됐다";
    }

    // 아이템이 없는 저장 사본. 테스트가 아이템을 붙인다.
    static PlayerSaveData MakeSaveData(int64 accountId, int64 characterId)
    {
        PlayerSaveData data;
        data.userId = accountId;
        data.progress.playerInfo.set_character_id(characterId);
        data.progress.playerInfo.set_level(2);
        data.progress.playerInfo.set_map_id(SAVED_MAP_ID);
        data.progress.posInfo.mutable_pos()->set_x(-150.5f);
        data.progress.posInfo.mutable_pos()->set_y(40.25f);
        data.progress.posInfo.mutable_pos()->set_z(5.f);
        data.progress.posInfo.set_yaw(45.f);
        data.progress.gold = 4321;

        auto* stats = data.progress.statInfo.mutable_info();
        (*stats)[Protocol::STAT_TYPE_EXP] = 1500;
        (*stats)[Protocol::STAT_TYPE_HP] = 60;
        (*stats)[Protocol::STAT_TYPE_MP] = 30;
        return data;
    }

    bool Save(const PlayerSaveData& data)
    {
        return ProgressStorage::Save(Connection(), data);
    }

    std::optional<PlayerProgress> Load(int64 accountId, int64 characterId)
    {
        PlayerProgress progress;
        if (ProgressStorage::Load(Connection(), accountId, characterId, OUT progress) == false)
            return std::nullopt;

        return progress;
    }

    std::optional<int64> CountRows(const wstring& table, const wstring& where)
    {
        return QueryInt64(L"SELECT COUNT(*) FROM " + table + L" WHERE " + where);
    }
};

TEST_F(DISABLED_GameDBProgressTest, SavedProgressAndItemsLoadBack)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'201;
    constexpr int64 CHARACTER_ID = 9'000'000'201'001;
    constexpr int64 POTION_UID = CHARACTER_ID * 1000 + 1;
    constexpr int64 MISC_UID = CHARACTER_ID * 1000 + 2;
    constexpr int64 BAG_SWORD_UID = CHARACTER_ID * 1000 + 3;
    constexpr int64 EQUIPPED_SWORD_UID = CHARACTER_ID * 1000 + 4;
    CreateCharacter(ACCOUNT_ID, CHARACTER_ID, "DB진행_왕복");
    const wstring characterWhere = L"CharacterId = " + to_wstring(CHARACTER_ID);
    ASSERT_TRUE(Execute(L"UPDATE dbo.CharacterState SET UpdatedAt = '2000-01-01' WHERE " + characterWhere));
    ASSERT_TRUE(Execute(L"UPDATE dbo.CharacterCurrency SET UpdatedAt = '2000-01-01' WHERE " + characterWhere));

    PlayerSaveData data = MakeSaveData(ACCOUNT_ID, CHARACTER_ID);
    AddConsumable(data, 0, POTION_UID, 7);
    AddItem(data, ContainerType::INVENTORY_MISC, 2, MISC_UID, MISC_TEMPLATE_ID, 3);
    AddInventoryGear(data, 4, BAG_SWORD_UID, 3);
    EquipWeapon(data, EQUIPPED_SWORD_UID, 1);
    ASSERT_TRUE(Save(data));

    const std::optional<PlayerProgress> loaded = Load(ACCOUNT_ID, CHARACTER_ID);
    ASSERT_TRUE(loaded.has_value());

    EXPECT_EQ(loaded->playerInfo.level(), 2);
    EXPECT_EQ(loaded->playerInfo.map_id(), SAVED_MAP_ID) << "룸이 아니라 맵을 저장하고 불러온다";
    EXPECT_FLOAT_EQ(loaded->posInfo.pos().x(), -150.5f);
    EXPECT_FLOAT_EQ(loaded->posInfo.pos().y(), 40.25f);
    EXPECT_FLOAT_EQ(loaded->posInfo.pos().z(), 5.f);
    EXPECT_FLOAT_EQ(loaded->posInfo.yaw(), 45.f);
    EXPECT_EQ(loaded->statInfo.info().at(Protocol::STAT_TYPE_EXP), 1500);
    EXPECT_EQ(loaded->statInfo.info().at(Protocol::STAT_TYPE_HP), 60);
    EXPECT_EQ(loaded->statInfo.info().at(Protocol::STAT_TYPE_MP), 30);
    EXPECT_EQ(loaded->gold, 4321);

    const ItemInstance* potion = FindItem(*loaded, ContainerType::INVENTORY_CONSUMABLE, 0);
    ASSERT_NE(potion, nullptr);
    EXPECT_EQ(potion->itemUid, POTION_UID);
    EXPECT_EQ(potion->count, 7);
    const ItemInstance* misc = FindItem(*loaded, ContainerType::INVENTORY_MISC, 2);
    ASSERT_NE(misc, nullptr);
    EXPECT_EQ(misc->itemUid, MISC_UID);
    const ItemInstance* bagSword = FindItem(*loaded, ContainerType::INVENTORY_GEAR, 4);
    ASSERT_NE(bagSword, nullptr);
    EXPECT_EQ(bagSword->itemUid, BAG_SWORD_UID);
    ASSERT_TRUE(bagSword->gear.has_value());
    EXPECT_EQ(bagSword->gear->enhanceLevel, 3);
    const ItemInstance* equippedSword = FindItem(*loaded, ContainerType::EQUIPPED, Protocol::GEAR_TYPE_WEAPON);
    ASSERT_NE(equippedSword, nullptr);
    EXPECT_EQ(equippedSword->itemUid, EQUIPPED_SWORD_UID);
    ASSERT_TRUE(equippedSword->gear.has_value());
    EXPECT_EQ(equippedSword->gear->enhanceLevel, 1);

    const wstring ownerWhere = L"OwnerId = " + to_wstring(CHARACTER_ID);
    EXPECT_EQ(CountRows(L"dbo.Item", ownerWhere), 4);
    EXPECT_EQ(CountRows(L"dbo.ItemGear", L"Durability = 0 AND ItemUid IN (SELECT ItemUid FROM dbo.Item WHERE " + ownerWhere + L")"), 2)
        << "장비만 ItemGear 행을 갖고 내구도는 0을 넣는다";
    EXPECT_EQ(CountRows(L"dbo.CharacterState", characterWhere + L" AND UpdatedAt > '2001-01-01'"), 1) << "상태의 갱신 시각은 서버가 넣는다";
    EXPECT_EQ(CountRows(L"dbo.CharacterCurrency", characterWhere + L" AND UpdatedAt > '2001-01-01'"), 1);
    EXPECT_EQ(CountRows(L"dbo.[Character]", characterWhere + L" AND LastLoginAt IS NOT NULL"), 1) << "입장할 때 마지막 입장 시각을 갱신한다";
}

TEST_F(DISABLED_GameDBProgressTest, OtherAccountAndDeletedCharacterAreNotLoaded)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'202;
    constexpr int64 CHARACTER_ID = 9'000'000'202'001;
    CreateCharacter(ACCOUNT_ID, CHARACTER_ID, "DB진행_남의것");

    EXPECT_FALSE(Load(ACCOUNT_ID + 1, CHARACTER_ID).has_value()) << "다른 계정의 캐릭터로는 입장하지 못한다";
    EXPECT_EQ(CountRows(L"dbo.[Character]", L"CharacterId = " + to_wstring(CHARACTER_ID) + L" AND LastLoginAt IS NOT NULL"), 0)
        << "다른 계정의 입장 시도가 마지막 입장 시각을 바꾸면 안 된다";

    ASSERT_TRUE(CharacterListDAO::DeleteCharacter(Connection(), ACCOUNT_ID, CHARACTER_ID));
    EXPECT_FALSE(Load(ACCOUNT_ID, CHARACTER_ID).has_value()) << "삭제한 캐릭터로는 입장하지 못한다";
}

TEST_F(DISABLED_GameDBProgressTest, TwoSlotsSwapInOneSave)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'203;
    constexpr int64 CHARACTER_ID = 9'000'000'203'001;
    constexpr int64 FIRST_UID = CHARACTER_ID * 1000 + 1;
    constexpr int64 SECOND_UID = CHARACTER_ID * 1000 + 2;
    CreateCharacter(ACCOUNT_ID, CHARACTER_ID, "DB진행_교환");

    PlayerSaveData before = MakeSaveData(ACCOUNT_ID, CHARACTER_ID);
    AddConsumable(before, 0, FIRST_UID);
    AddConsumable(before, 1, SECOND_UID);
    ASSERT_TRUE(Save(before));

    PlayerSaveData after = MakeSaveData(ACCOUNT_ID, CHARACTER_ID);
    AddConsumable(after, 1, FIRST_UID);
    AddConsumable(after, 0, SECOND_UID);
    ASSERT_TRUE(Save(after)) << "두 칸을 맞바꾸는 갱신은 한 문장 안에서 고유 제약을 지킨다";

    const std::optional<PlayerProgress> loaded = Load(ACCOUNT_ID, CHARACTER_ID);
    ASSERT_TRUE(loaded.has_value());
    const ItemInstance* slot0 = FindItem(*loaded, ContainerType::INVENTORY_CONSUMABLE, 0);
    const ItemInstance* slot1 = FindItem(*loaded, ContainerType::INVENTORY_CONSUMABLE, 1);
    ASSERT_NE(slot0, nullptr);
    ASSERT_NE(slot1, nullptr);
    EXPECT_EQ(slot0->itemUid, SECOND_UID);
    EXPECT_EQ(slot1->itemUid, FIRST_UID);
}

TEST_F(DISABLED_GameDBProgressTest, GearMovesBetweenInventoryAndEquipped)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'204;
    constexpr int64 CHARACTER_ID = 9'000'000'204'001;
    constexpr int64 SWORD_UID = CHARACTER_ID * 1000 + 1;
    CreateCharacter(ACCOUNT_ID, CHARACTER_ID, "DB진행_착용");
    const wstring gearWhere = L"ItemUid = " + to_wstring(SWORD_UID) + L" AND EnhanceLevel = 5";

    PlayerSaveData inBag = MakeSaveData(ACCOUNT_ID, CHARACTER_ID);
    AddInventoryGear(inBag, 0, SWORD_UID, 5);
    ASSERT_TRUE(Save(inBag));

    // 착용
    PlayerSaveData equipped = MakeSaveData(ACCOUNT_ID, CHARACTER_ID);
    EquipWeapon(equipped, SWORD_UID, 5);
    ASSERT_TRUE(Save(equipped));

    std::optional<PlayerProgress> loaded = Load(ACCOUNT_ID, CHARACTER_ID);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(CountItems(*loaded, ContainerType::INVENTORY_GEAR), 0u) << "착용한 장비는 인벤토리에 남지 않는다";
    const ItemInstance* equippedSword = FindItem(*loaded, ContainerType::EQUIPPED, Protocol::GEAR_TYPE_WEAPON);
    ASSERT_NE(equippedSword, nullptr);
    EXPECT_EQ(equippedSword->itemUid, SWORD_UID);
    EXPECT_EQ(CountRows(L"dbo.ItemGear", gearWhere), 1) << "자리만 바뀐 장비의 세부 값은 남는다";

    // 해제
    PlayerSaveData unequipped = MakeSaveData(ACCOUNT_ID, CHARACTER_ID);
    AddInventoryGear(unequipped, 3, SWORD_UID, 5);
    ASSERT_TRUE(Save(unequipped));

    loaded = Load(ACCOUNT_ID, CHARACTER_ID);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(CountItems(*loaded, ContainerType::EQUIPPED), 0u);
    const ItemInstance* bagSword = FindItem(*loaded, ContainerType::INVENTORY_GEAR, 3);
    ASSERT_NE(bagSword, nullptr);
    EXPECT_EQ(bagSword->itemUid, SWORD_UID);
    EXPECT_EQ(CountRows(L"dbo.Item", L"OwnerId = " + to_wstring(CHARACTER_ID)), 1) << "옮긴 아이템은 행 하나다";
}

TEST_F(DISABLED_GameDBProgressTest, ItemMissingFromSaveIsDeleted)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'205;
    constexpr int64 CHARACTER_ID = 9'000'000'205'001;
    constexpr int64 POTION_UID = CHARACTER_ID * 1000 + 1;
    constexpr int64 SWORD_UID = CHARACTER_ID * 1000 + 2;
    CreateCharacter(ACCOUNT_ID, CHARACTER_ID, "DB진행_삭제");

    PlayerSaveData before = MakeSaveData(ACCOUNT_ID, CHARACTER_ID);
    AddConsumable(before, 0, POTION_UID);
    AddInventoryGear(before, 0, SWORD_UID, 2);
    ASSERT_TRUE(Save(before));

    PlayerSaveData after = MakeSaveData(ACCOUNT_ID, CHARACTER_ID);
    AddConsumable(after, 0, POTION_UID);
    ASSERT_TRUE(Save(after));

    EXPECT_EQ(CountRows(L"dbo.Item", L"ItemUid = " + to_wstring(SWORD_UID)), 0) << "사본에 없는 행은 지운다(판매, 소비)";
    EXPECT_EQ(CountRows(L"dbo.ItemGear", L"ItemUid = " + to_wstring(SWORD_UID)), 0) << "지운 장비의 세부 행도 함께 지워진다";
    EXPECT_EQ(CountRows(L"dbo.Item", L"ItemUid = " + to_wstring(POTION_UID)), 1);
}

TEST_F(DISABLED_GameDBProgressTest, ItemsBeyondParamRowLimitAreSaved)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'206;
    constexpr int64 CHARACTER_ID = 9'000'000'206'001;
    CreateCharacter(ACCOUNT_ID, CHARACTER_ID, "DB진행_많이");
    const wstring ownerWhere = L"OwnerId = " + to_wstring(CHARACTER_ID);
    const int32 manyCount = MAX_PARAM_ROWS + 50;
    const int32 fewerCount = MAX_PARAM_ROWS + 20;

    PlayerSaveData many = MakeSaveData(ACCOUNT_ID, CHARACTER_ID);
    for (int32 i = 0; i < manyCount; i++)
        AddConsumable(many, i, CHARACTER_ID * 1000 + i + 1);
    ASSERT_TRUE(Save(many));
    EXPECT_EQ(CountRows(L"dbo.Item", ownerWhere), manyCount);

    const std::optional<PlayerProgress> loaded = Load(ACCOUNT_ID, CHARACTER_ID);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(CountItems(*loaded, ContainerType::INVENTORY_CONSUMABLE), size_t(manyCount));

    // 줄인 저장도 두 묶음에 걸친다. 묶음마다 MERGE하면 앞 묶음의 행이 지워진다.
    PlayerSaveData fewer = MakeSaveData(ACCOUNT_ID, CHARACTER_ID);
    for (int32 i = 0; i < fewerCount; i++)
        AddConsumable(fewer, i, CHARACTER_ID * 1000 + i + 1);
    ASSERT_TRUE(Save(fewer));
    EXPECT_EQ(CountRows(L"dbo.Item", ownerWhere), fewerCount);
}

TEST_F(DISABLED_GameDBProgressTest, FailedSaveRollsBackEverything)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'207;
    constexpr int64 CHARACTER_ID = 9'000'000'207'001;
    constexpr int64 OTHER_CHARACTER_ID = 9'000'000'207'002;
    constexpr int64 POTION_UID = CHARACTER_ID * 1000 + 1;
    constexpr int64 OTHER_POTION_UID = OTHER_CHARACTER_ID * 1000 + 1;
    CreateCharacter(ACCOUNT_ID, CHARACTER_ID, "DB진행_롤백");

    Protocol::CharacterOverview other;
    other.set_class_(Protocol::CLASS_TYPE_WARRIOR);
    other.set_name("DB진행_롤백_남");
    ASSERT_FALSE(CharacterListDAO::CreateCharacter(Connection(), ACCOUNT_ID, OTHER_CHARACTER_ID, other).rejection.has_value());

    PlayerSaveData otherData = MakeSaveData(ACCOUNT_ID, OTHER_CHARACTER_ID);
    AddConsumable(otherData, 0, OTHER_POTION_UID);
    ASSERT_TRUE(Save(otherData));

    PlayerSaveData baseline = MakeSaveData(ACCOUNT_ID, CHARACTER_ID);
    AddConsumable(baseline, 0, POTION_UID);
    ASSERT_TRUE(Save(baseline));

    // 레벨, 상태, 골드는 갱신된 뒤에 아이템 MERGE가 다른 캐릭터의 UID로 기본 키 위반을 낸다.
    PlayerSaveData broken = MakeSaveData(ACCOUNT_ID, CHARACTER_ID);
    broken.progress.playerInfo.set_level(1);
    broken.progress.gold = 9999;
    (*broken.progress.statInfo.mutable_info())[Protocol::STAT_TYPE_EXP] = 10;
    AddConsumable(broken, 3, OTHER_POTION_UID);
    EXPECT_FALSE(Save(broken));

    const wstring characterWhere = L"CharacterId = " + to_wstring(CHARACTER_ID);
    EXPECT_EQ(CountRows(L"dbo.[Character]", characterWhere + L" AND Level = 2"), 1) << "앞서 갱신한 레벨도 되돌린다";
    EXPECT_EQ(CountRows(L"dbo.CharacterState", characterWhere + L" AND Exp = 1500"), 1);
    EXPECT_EQ(CountRows(L"dbo.CharacterCurrency", characterWhere + L" AND Amount = 4321"), 1);
    EXPECT_EQ(CountRows(L"dbo.Item", L"OwnerId = " + to_wstring(CHARACTER_ID) + L" AND ItemUid = " + to_wstring(POTION_UID)), 1)
        << "사본에 없던 물약을 지운 것도 되돌린다";
    EXPECT_EQ(CountRows(L"dbo.Item", L"OwnerId = " + to_wstring(OTHER_CHARACTER_ID) + L" AND ItemUid = " + to_wstring(OTHER_POTION_UID)), 1)
        << "다른 캐릭터의 아이템은 그대로다";

    // 실패한 저장이 연결에 남긴 임시 테이블이나 트랜잭션이 다음 저장을 막지 않는다.
    EXPECT_TRUE(Save(baseline));
}

TEST_F(DISABLED_GameDBProgressTest, SavedQuickSlotsLoadBack)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'208;
    constexpr int64 CHARACTER_ID = 9'000'000'208'001;
    CreateCharacter(ACCOUNT_ID, CHARACTER_ID, "DB진행_퀵슬롯");

    PlayerSaveData data = MakeSaveData(ACCOUNT_ID, CHARACTER_ID);
    data.progress.quickSlots = { ItemQuickSlot(4, MP_POTION_TEMPLATE_ID), ItemQuickSlot(1, POTION_TEMPLATE_ID) };
    ASSERT_TRUE(Save(data));

    const std::optional<PlayerProgress> loaded = Load(ACCOUNT_ID, CHARACTER_ID);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->quickSlots, (vector<QuickSlot>{ ItemQuickSlot(1, POTION_TEMPLATE_ID), ItemQuickSlot(4, MP_POTION_TEMPLATE_ID) }))
        << "퀵슬롯 번호 순서로 불러온다";

    const wstring characterWhere = L"CharacterId = " + to_wstring(CHARACTER_ID);
    EXPECT_EQ(CountRows(L"dbo.QuickSlot", characterWhere + L" AND RefType = 1"), 2) << "아이템 정의를 가리키는 RefType은 1이다";
    EXPECT_EQ(CountRows(L"dbo.QuickSlot", characterWhere + L" AND SlotIndex = 4 AND RefId = " + to_wstring(MP_POTION_TEMPLATE_ID)), 1);
}

TEST_F(DISABLED_GameDBProgressTest, EmptiedQuickSlotRowIsDeleted)
{
    constexpr int64 ACCOUNT_ID = 9'000'000'000'209;
    constexpr int64 CHARACTER_ID = 9'000'000'209'001;
    constexpr int64 OTHER_CHARACTER_ID = 9'000'000'209'002;
    CreateCharacter(ACCOUNT_ID, CHARACTER_ID, "DB진행_퀵해제");

    Protocol::CharacterOverview other;
    other.set_class_(Protocol::CLASS_TYPE_WARRIOR);
    other.set_name("DB진행_퀵해제_남");
    ASSERT_FALSE(CharacterListDAO::CreateCharacter(Connection(), ACCOUNT_ID, OTHER_CHARACTER_ID, other).rejection.has_value());

    PlayerSaveData otherData = MakeSaveData(ACCOUNT_ID, OTHER_CHARACTER_ID);
    otherData.progress.quickSlots = { ItemQuickSlot(0, POTION_TEMPLATE_ID) };
    ASSERT_TRUE(Save(otherData));

    PlayerSaveData before = MakeSaveData(ACCOUNT_ID, CHARACTER_ID);
    before.progress.quickSlots = { ItemQuickSlot(0, POTION_TEMPLATE_ID), ItemQuickSlot(2, MP_POTION_TEMPLATE_ID) };
    ASSERT_TRUE(Save(before));

    // 0번을 해제하고 2번에 다른 물약을 등록했다.
    PlayerSaveData after = MakeSaveData(ACCOUNT_ID, CHARACTER_ID);
    after.progress.quickSlots = { ItemQuickSlot(2, POTION_TEMPLATE_ID) };
    ASSERT_TRUE(Save(after));

    const wstring characterWhere = L"CharacterId = " + to_wstring(CHARACTER_ID);
    EXPECT_EQ(CountRows(L"dbo.QuickSlot", characterWhere + L" AND SlotIndex = 0"), 0) << "빈 퀵슬롯은 행을 두지 않는다";
    EXPECT_EQ(CountRows(L"dbo.QuickSlot", characterWhere + L" AND SlotIndex = 2 AND RefId = " + to_wstring(POTION_TEMPLATE_ID)), 1);
    EXPECT_EQ(CountRows(L"dbo.QuickSlot", L"CharacterId = " + to_wstring(OTHER_CHARACTER_ID)), 1) << "다른 캐릭터의 퀵슬롯은 그대로다";

    // 모두 해제한 저장도 남은 행을 지운다.
    ASSERT_TRUE(Save(MakeSaveData(ACCOUNT_ID, CHARACTER_ID)));
    EXPECT_EQ(CountRows(L"dbo.QuickSlot", characterWhere), 0);
}
