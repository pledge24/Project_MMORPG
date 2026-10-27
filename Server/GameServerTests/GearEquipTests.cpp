#include "Core/pch.h"
#include <gtest/gtest.h>
#include "PlayerTestAccess.h"
#include "TestItemTables.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Inventory/InventoryComponent.h"
#include "Game/Equipment/EquipmentComponent.h"
#include "TestLevelTables.h"

/*--------------------------------------------------------------
    장비 장착·탈착 결과 테스트

    클라는 S_GEAR_APPEARANCE의 gear_part와 template_id로 캐릭터 외형을 바꾼다.
    그래서 두 값은 요청한 칸이 아니라 처리 결과여야 한다. gear_part는 장비 부위, template_id는
    처리 뒤 그 부위의 아이템이다. 나중에 룸에 들어온 플레이어가 보는 외형은 gear_appearance로 간다.

    픽스처 결합도: 아이템 템플릿, 2레벨까지 있는 전사 레벨 표, 인벤토리 구역 표를 Gamedata::Install로 주입하고 Player를 세션 없이
    EntityFactory로만 만든다. 장착과 해제가 레벨 표로 최종 스탯을 다시 계산하므로 레벨 표가 있어야 한다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 SWORD_TEMPLATE_ID = 1001;
    constexpr int32 HELMET_TEMPLATE_ID = 1000;
    constexpr int32 BASE_PHYSICAL_ATTACK = 5;
    constexpr int32 BASE_MAX_HP = 500;
    constexpr int32 BASE_MAX_MP = 100;
    constexpr int32 HELMET_HP = 100;
    constexpr int32 HELMET_MP = 50;
    constexpr int32 HIGH_LEVEL_SWORD_TEMPLATE_ID = 1030;
    constexpr int32 MAGE_SWORD_TEMPLATE_ID = 1031;
}

class GearEquipTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GamedataTables tables;
        for (int32 swordId : { SWORD_TEMPLATE_ID, HIGH_LEVEL_SWORD_TEMPLATE_ID, MAGE_SWORD_TEMPLATE_ID })
            TestItemTables::AddGear(tables, swordId, Protocol::GEAR_PART_WEAPON).PhysicalAttack = 10;
        tables.items[HIGH_LEVEL_SWORD_TEMPLATE_ID].LevelRequirement = 2;
        tables.items[MAGE_SWORD_TEMPLATE_ID].AllowedClasses = { Protocol::CHARACTER_CLASS_MAGE };

        DesignData::ItemEquipStatRow& helmet = TestItemTables::AddGear(tables, HELMET_TEMPLATE_ID, Protocol::GEAR_PART_HELMET);
        helmet.Hp = HELMET_HP;
        helmet.Mp = HELMET_MP;

        for (int32 level : { 1, 2 })
        {
            DesignData::ClassLevelStatRow& stat = TestLevelTables::AddWarriorLevel(tables, level, level == 1 ? 100 : 0);
            stat.MaxHp = BASE_MAX_HP;
            stat.MaxMp = BASE_MAX_MP;
            stat.PhysicalAttack = BASE_PHYSICAL_ATTACK;
        }
        tables.inventorySlotCounts = MakeInventorySections();
        Gamedata::Install(std::move(tables));

        player = EntityFactory::Create<Player>(PlayerSpawnParams());
        ASSERT_NE(player, nullptr);
        PlayerTestAccess::CharacterClass(*player) = Protocol::CHARACTER_CLASS_WARRIOR;
        PlayerTestAccess::PlayerInfo(*player).set_level(1);
        player->SetStatValue(Protocol::STAT_TYPE_HP, 0);
        player->SetStatValue(Protocol::STAT_TYPE_MP, 0);
        player->SetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK, BASE_PHYSICAL_ATTACK);
        player->SetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK, 0);
        ASSERT_TRUE(player->OnLoaded()) << "입장을 마친 플레이어처럼 최대치까지 계산해 둔다";
    }

    void TearDown() override
    {
        player.reset();
        Gamedata::Install(GamedataTables());
    }

    // 인벤토리에 아이템을 넣고, 클라가 장착을 요청할 때 보내는 인벤토리 칸을 돌려준다.
    ItemSlotRequest AddToInventory(int32 templateId)
    {
        optional<SlotAddress> address = PlayerTestAccess::AddNewItem(*player, templateId);
        EXPECT_TRUE(address.has_value());
        return address.has_value() ? MakeInventoryRequest(*player, address.value()) : ItemSlotRequest();
    }

    ItemSlotRequest AddSwordToInventory() { return AddToInventory(SWORD_TEMPLATE_ID); }

    // 저장 사본을 DB가 불러오는 것처럼 새 플레이어에 채운다. DB는 레벨, 직업, 현재 HP와 MP, 장착 장비만 돌려주고 공격력은 다시 계산한다.
    PlayerRef LoadFromSaveData(const PlayerSaveData& data)
    {
        PlayerRef loaded = EntityFactory::Create<Player>(PlayerSpawnParams());
        PlayerTestAccess::CharacterClass(*loaded) = static_cast<Protocol::CharacterClass>(data.progress.classId);
        PlayerTestAccess::PlayerInfo(*loaded).set_level(data.progress.level);

        loaded->SetStatValue(Protocol::STAT_TYPE_HP, data.progress.hp);
        loaded->SetStatValue(Protocol::STAT_TYPE_MP, data.progress.mp);
        for (Protocol::StatType statType : { Protocol::STAT_TYPE_PHYSICAL_ATTACK, Protocol::STAT_TYPE_MAGICAL_ATTACK })
            loaded->SetStatValue(statType, 0);

        for (const PlacedItem& placed : data.progress.items)
        {
            if (placed.address.container == ContainerType::EQUIPPED)
                EXPECT_TRUE(PlayerTestAccess::Equipment(*loaded).LoadEquipped(placed.item, placed.address.slotIndex));
        }

        return loaded;
    }

    PlayerRef player;
};

TEST_F(GearEquipTest, EquipReportsGearTypeNotInventorySlot)
{
    ItemSlotRequest request = AddSwordToInventory();

    auto result = player->ProcessEquipGear(request);
    ASSERT_TRUE(result.IsSuccess());

    EXPECT_EQ(result->gearPart, Protocol::GEAR_PART_WEAPON) << "인벤토리 칸 번호가 실려 가면 클라가 무기 부위를 찾지 못한다";
    EXPECT_EQ(result->templateId, SWORD_TEMPLATE_ID);

    const auto& summary = player->GetPlayerInfo().gear_appearance();
    ASSERT_TRUE(summary.contains(Protocol::GEAR_PART_WEAPON));
    EXPECT_EQ(summary.at(Protocol::GEAR_PART_WEAPON), SWORD_TEMPLATE_ID);
}

TEST_F(GearEquipTest, UnequipReportsEmptiedGearType)
{
    auto equipResult = player->ProcessEquipGear(AddSwordToInventory());
    ASSERT_TRUE(equipResult.IsSuccess());

    // 클라는 탈착할 때 장비 칸을 그대로 보낸다.
    auto result = player->ProcessUnequipGear(MakeEquippedRequest(*player, Protocol::GEAR_PART_WEAPON));
    ASSERT_TRUE(result.IsSuccess());

    EXPECT_EQ(result->gearPart, Protocol::GEAR_PART_WEAPON);
    EXPECT_EQ(result->templateId, 0) << "뺀 아이템의 템플릿이 실려 가면 클라가 그 아이템을 다시 입힌다";
    EXPECT_FALSE(player->GetPlayerInfo().gear_appearance().contains(Protocol::GEAR_PART_WEAPON));
}

// 장착과 해제는 레벨 표의 기본 스탯과 착용 장비의 합으로 최종 스탯을 다시 계산하고, 바뀐 스탯만 응답에 싣는다.
TEST_F(GearEquipTest, EquipAndUnequipReportRecalculatedStats)
{
    auto equipResult = player->ProcessEquipGear(AddSwordToInventory());
    ASSERT_TRUE(equipResult.IsSuccess());

    ASSERT_EQ(equipResult->changedStats.size(), 1) << "값이 바뀌지 않은 스탯까지 실으면 클라이언트가 불필요하게 갱신한다";
    EXPECT_EQ(equipResult->changedStats[0].type(), Protocol::STAT_TYPE_PHYSICAL_ATTACK);
    EXPECT_EQ(equipResult->changedStats[0].value(), BASE_PHYSICAL_ATTACK + 10);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), BASE_PHYSICAL_ATTACK + 10);

    auto unequipResult = player->ProcessUnequipGear(MakeEquippedRequest(*player, Protocol::GEAR_PART_WEAPON));
    ASSERT_TRUE(unequipResult.IsSuccess());

    ASSERT_EQ(unequipResult->changedStats.size(), 1);
    EXPECT_EQ(unequipResult->changedStats[0].value(), BASE_PHYSICAL_ATTACK);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), BASE_PHYSICAL_ATTACK);
}

// 해제로 최대치가 줄면 현재 HP와 MP도 새 최대치로 잘려야 한다. 잘리지 않은 채 저장되면 다음 입장이
// 「현재 HP가 최대 HP를 초과」로 거절된다.
TEST_F(GearEquipTest, UnequippingAtFullHpSavesCopyThatPassesNextEntry)
{
    auto equipResult = player->ProcessEquipGear(AddToInventory(HELMET_TEMPLATE_ID));
    ASSERT_TRUE(equipResult.IsSuccess());
    player->SetStatValue(Protocol::STAT_TYPE_HP, BASE_MAX_HP + HELMET_HP);
    player->SetStatValue(Protocol::STAT_TYPE_MP, BASE_MAX_MP + HELMET_MP);

    auto unequipResult = player->ProcessUnequipGear(MakeEquippedRequest(*player, Protocol::GEAR_PART_HELMET));
    ASSERT_TRUE(unequipResult.IsSuccess());

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), BASE_MAX_HP);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MP), BASE_MAX_MP);

    // 클라이언트도 잘린 현재 HP와 MP를 받아야 화면의 값이 최대치를 넘지 않는다.
    map<int32, int64> reported;
    for (const Protocol::Stat& stat : unequipResult->changedStats)
        reported[stat.type()] = stat.value();
    EXPECT_EQ(reported[Protocol::STAT_TYPE_HP], BASE_MAX_HP);
    EXPECT_EQ(reported[Protocol::STAT_TYPE_MP], BASE_MAX_MP);

    PlayerRef reloaded = LoadFromSaveData(player->MakeSaveData());
    EXPECT_TRUE(reloaded->OnLoaded()) << "저장 사본의 현재 HP가 최대 HP를 넘으면 그 캐릭터는 다시 들어오지 못한다";
}

// 착용 조건은 클라이언트만 보던 것을 서버가 다시 판정한다. 조작한 클라이언트가 보낸 요청도 거절해야 한다.
TEST_F(GearEquipTest, EquippingAboveLevelRequirementIsRejected)
{
    ItemSlotRequest request = AddToInventory(HIGH_LEVEL_SWORD_TEMPLATE_ID);

    auto result = player->ProcessEquipGear(request);
    EXPECT_EQ(result.GetResult(), Protocol::ITEM_RESULT_REQUIREMENT_NOT_MET) << "1레벨이 2레벨 장비를 입으면 안 된다";

    EXPECT_EQ(player->GetEquipment().FindItem(Protocol::GEAR_PART_WEAPON), nullptr);
    EXPECT_NE(player->GetInventory().FindItem(request.address), nullptr);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), BASE_PHYSICAL_ATTACK);
}

TEST_F(GearEquipTest, EquippingOtherClassGearIsRejected)
{
    ItemSlotRequest request = AddToInventory(MAGE_SWORD_TEMPLATE_ID);

    auto result = player->ProcessEquipGear(request);
    EXPECT_EQ(result.GetResult(), Protocol::ITEM_RESULT_REQUIREMENT_NOT_MET) << "전사가 마법사 장비를 입으면 안 된다";

    EXPECT_EQ(player->GetEquipment().FindItem(Protocol::GEAR_PART_WEAPON), nullptr);
    EXPECT_NE(player->GetInventory().FindItem(request.address), nullptr);
}

TEST_F(GearEquipTest, EquippingAtRequiredLevelSucceeds)
{
    PlayerTestAccess::PlayerInfo(*player).set_level(2);

    auto result = player->ProcessEquipGear(AddToInventory(HIGH_LEVEL_SWORD_TEMPLATE_ID));
    EXPECT_TRUE(result.IsSuccess()) << "요구 레벨과 같은 레벨은 입는다";
}

// DB에서 장착 장비를 불러올 때는 스텟을 바꾸지 않는다. 스텟은 OnLoaded가 대조한 뒤 RefreshFinalStat이 계산한다.
TEST_F(GearEquipTest, LoadingEquippedGearPlacesItemWithoutTouchingStats)
{
    ItemInstance sword;
    sword.itemUid = 1;
    sword.templateId = SWORD_TEMPLATE_ID;

    ASSERT_TRUE(PlayerTestAccess::Equipment(*player).LoadEquipped(sword, Protocol::GEAR_PART_WEAPON));

    const ItemInstance* weapon = player->GetEquipment().FindItem(Protocol::GEAR_PART_WEAPON);
    ASSERT_NE(weapon, nullptr);
    EXPECT_EQ(weapon->templateId, SWORD_TEMPLATE_ID);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), BASE_PHYSICAL_ATTACK) << "불러오기에서 장비 스텟을 더하면 OnLoaded의 대조와 어긋난다";
}
