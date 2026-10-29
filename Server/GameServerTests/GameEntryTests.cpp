#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Items/InventorySections.h"
#include "Network/GameEntry.h"
#include "Network/GameSessionManager.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"
#include "Game/Inventory/InventoryComponent.h"
#include "Game/Equipment/EquipmentComponent.h"
#include "Game/Room/Room.h"
#include "DB/ItemRows.h"
#include "TestItemTables.h"
#include "TestLevelTables.h"
#include "TestAreaTables.h"

/*--------------------------------------------------------------
    게임 입장 테스트

    불러온 진행으로 플레이어를 만들고, 검증을 통과해야 세션에 등록한다. 전에는 불러오기 전에
    세션에 플레이어를 넣어 두고 DB가 그 플레이어에 직접 채웠다. 그래서 불러오기나 검증이 실패해도
    절반만 채운 플레이어가 세션에 남았다.

    픽스처 결합도: 아이템 템플릿과 2레벨짜리 전사 레벨 표를
    Gamedata::Install로 주입하고, 소켓 없는 GameSession을 만든다. DB는 거치지 않고 DAO가 채울 진행 사본을
    손으로 만들거나, DB 행을 DAO와 같은 변환(ItemRows)으로 붙여 만든다.
---------------------------------------------------------------*/

namespace
{
    constexpr int64 USER_ID = 7;
    constexpr int64 CHARACTER_ID = 42;
    constexpr int32 SWORD_TEMPLATE_ID = 1001;
    constexpr int32 POTION_TEMPLATE_ID = 2000;
    constexpr int32 MAX_HP = 500;
    constexpr int32 MAX_MP = 100;
    constexpr int32 PHYSICAL_ATTACK = 10;
    /** 레벨 2의 물리 공격력. 레벨 1과 달라야 다시 계산한 레벨의 공격력이 쓰이는지 드러난다. */
    constexpr int32 LEVEL2_PHYSICAL_ATTACK = 13;
    constexpr int32 SWORD_ATTACK = 15;
    /** 레벨 1 → 2의 요구치. 레벨 2가 최대 레벨이라 누적 경험치는 이 값을 넘지 않는다. */
    constexpr int64 LEVEL1_EXP_REQUIREMENT = 50;
    constexpr int32 ENTER_ROOM_ID = 10;

    PlacedItem MakeLoadedItem(ContainerType container, int32 slotIndex, int32 templateId, int32 count)
    {
        PlacedItem placed;
        placed.address = SlotAddress{ container, slotIndex };
        placed.item.templateId = templateId;
        placed.item.count = count;
        return placed;
    }

    // DB의 행을 DAO와 같은 변환으로 진행에 붙인다.
    void AddRow(PlayerProgress& progress, const ItemRow& row)
    {
        optional<PlacedItem> placed = ItemRows::ToPlacedItem(row);
        ASSERT_TRUE(placed.has_value());
        progress.items.push_back(placed.value());
    }

    // DB의 Item 행 하나. DAO가 읽는 그대로다.
    ItemRow MakeInventoryRow(ItemRowType section, int32 slotIndex, int32 templateId, int64 itemUid)
    {
        ItemRow row;
        row.itemUid = itemUid;
        row.templateId = templateId;
        row.itemType = section;
        row.container = ItemRowContainer::INVENTORY;
        row.slotIndex = slotIndex;
        row.stackCount = 1;
        return row;
    }
}

class GameEntryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GamedataTables tables;
        TestItemTables::AddGear(tables, SWORD_TEMPLATE_ID, Protocol::GEAR_PART_WEAPON).PhysicalAttack = SWORD_ATTACK;
        TestItemTables::AddConsumable(tables, POTION_TEMPLATE_ID, 99);
        DesignData::ClassLevelStatRow& level1 = TestLevelTables::AddWarriorLevel(tables, 1, LEVEL1_EXP_REQUIREMENT);
        level1.MaxHp = MAX_HP;
        level1.MaxMp = MAX_MP;
        level1.PhysicalAttack = PHYSICAL_ATTACK;
        DesignData::ClassLevelStatRow& level2 = TestLevelTables::AddWarriorLevel(tables, 2);
        level2.MaxHp = MAX_HP;
        level2.MaxMp = MAX_MP;
        level2.PhysicalAttack = LEVEL2_PHYSICAL_ATTACK;
        Gamedata::Install(std::move(tables));

        session = make_shared<GameSession>();
        sessionManager.RegisterUser(USER_ID, session);

        const TestAreaTables::TestArea enterArea = TestAreaTables::MakeArea(ENTER_ROOM_ID, 0);
        enterRoom = Room::Create(enterArea.row, enterArea.placement);
    }

    void TearDown() override
    {
        session.reset();
        enterRoom.reset();
        Gamedata::Install(GamedataTables());
    }

    // DAO가 채우는 것과 같은 모양의 진행. 무기를 하나 입고 있다. 공격력은 DB에 없으므로 사본에도 없다.
    static PlayerProgress MakeValidProgress()
    {
        PlayerProgress progress;
        progress.characterId = CHARACTER_ID;
        progress.classId = Protocol::CHARACTER_CLASS_WARRIOR;
        progress.level = 1;
        progress.mapId = 1;
        progress.totalExp = 20;
        progress.hp = MAX_HP;
        progress.mp = MAX_MP;

        progress.gold = 300;
        progress.items.push_back(MakeLoadedItem(ContainerType::EQUIPPED, Protocol::GEAR_PART_WEAPON, SWORD_TEMPLATE_ID, 1));
        progress.items.push_back(MakeLoadedItem(ContainerType::INVENTORY_CONSUMABLE, 5, POTION_TEMPLATE_ID, 3));
        return progress;
    }

    GameSessionManager sessionManager;
    GameSessionRef session;
    /** 플레이어가 맵 로드를 기다릴 룸. 시작하지 않는다. */
    RoomRef enterRoom;
    Protocol::PosInfo enterPos;
};

TEST_F(GameEntryTest, ValidProgressRegistersPlayerInSession)
{
    const PlayerProgress progress = MakeValidProgress();

    PlayerRef player = GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos);

    ASSERT_NE(player, nullptr);
    EXPECT_EQ(session->GetPlayer(), player);
    EXPECT_EQ(player->GetUserId(), USER_ID);
    EXPECT_EQ(player->GetCharacterId(), CHARACTER_ID);
    EXPECT_EQ(player->GetRoom(), enterRoom) << "맵 로드 완료는 입장할 룸의 큐에서 돈다";
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MAX_HP), MAX_HP);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MAX_EXP), LEVEL1_EXP_REQUIREMENT) << "최대 경험치는 저장하지 않고 레벨 표에서 정한다";
    EXPECT_EQ(player->GetGold(), 300);
}

TEST_F(GameEntryTest, LoadedSlotsGoToTheirSlotIds)
{
    const PlayerProgress progress = MakeValidProgress();

    PlayerRef player = GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos);
    ASSERT_NE(player, nullptr);

    const ItemInstance* potion = player->GetInventory().FindItem(SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, 5 });
    ASSERT_NE(potion, nullptr);
    EXPECT_EQ(potion->templateId, POTION_TEMPLATE_ID);
    EXPECT_EQ(potion->count, 3);
    EXPECT_NE(potion->itemUid, 0) << "UID 없이 들어온 아이템은 저장할 수 없다";
    EXPECT_EQ(player->GetInventory().FindItem(SlotAddress{ ContainerType::INVENTORY_CONSUMABLE, 0 }), nullptr);

    const ItemInstance* sword = player->GetEquipment().FindItem(Protocol::GEAR_PART_WEAPON);
    ASSERT_NE(sword, nullptr);
    EXPECT_EQ(sword->templateId, SWORD_TEMPLATE_ID);
}

TEST_F(GameEntryTest, InventorySectionsHaveFixedSlotCount)
{
    PlayerRef player = GameEntry::SpawnPlayer(session, USER_ID, MakeValidProgress(), enterRoom, enterPos);
    ASSERT_NE(player, nullptr);

    const InventoryComponent& inventory = player->GetInventory();
    for (const InventorySection& section : InventorySections::ALL)
        EXPECT_EQ(inventory.FindSection(section.container)->GetSlotIndices().size(), size_t(InventorySections::SLOT_COUNT));
    EXPECT_EQ(inventory.FindSection(ContainerType::EQUIPPED), nullptr) << "착용 장비는 인벤토리의 구역이 아니다";
}

/* 공격력은 저장하지 않는다. 레벨 표와 착용 장비로 다시 계산한다. */

TEST_F(GameEntryTest, AttackIsCalculatedFromLevelAndEquippedGear)
{
    PlayerRef player = GameEntry::SpawnPlayer(session, USER_ID, MakeValidProgress(), enterRoom, enterPos);

    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), PHYSICAL_ATTACK + SWORD_ATTACK);
}

TEST_F(GameEntryTest, RecalculatedLevelWithOtherAttackStillEnters)
{
    PlayerProgress progress = MakeValidProgress();
    progress.totalExp = LEVEL1_EXP_REQUIREMENT;

    PlayerRef player = GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos);

    ASSERT_NE(player, nullptr) << "저장된 레벨과 다시 계산한 레벨의 공격력이 달라도 입장한다";
    EXPECT_EQ(player->GetPlayerInfo().level(), 2);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK), LEVEL2_PHYSICAL_ATTACK + SWORD_ATTACK);
}

TEST_F(GameEntryTest, FailedValidationLeavesSessionWithoutPlayer)
{
    PlayerProgress progress = MakeValidProgress();
    progress.hp = MAX_HP + 1;

    PlayerRef player = GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos);

    EXPECT_EQ(player, nullptr);
    EXPECT_EQ(session->GetPlayer(), nullptr) << "검증에 실패한 플레이어가 세션에 남으면 끊길 때 저장 경로를 탄다";
}

TEST_F(GameEntryTest, UnknownClassLeavesSessionWithoutPlayer)
{
    PlayerProgress progress = MakeValidProgress();
    progress.classId = Protocol::CHARACTER_CLASS_MAGE;

    EXPECT_EQ(GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos), nullptr);
    EXPECT_EQ(session->GetPlayer(), nullptr);
}

/* DB의 경험치는 누적값이고 레벨은 캐릭터 목록에 보일 사본이다. 입장할 때 레벨을 누적 경험치로 다시 계산한다. */

TEST_F(GameEntryTest, LevelHigherThanExpIsLoweredToCalculatedLevel)
{
    PlayerProgress progress = MakeValidProgress();
    progress.level = 2;

    PlayerRef player = GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos);

    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->GetPlayerInfo().level(), 1) << "누적 경험치 20은 레벨 2의 시작값 50에 못 미친다";
    EXPECT_EQ(player->MakeSaveData().progress.level, 1) << "저장하는 레벨 사본도 계산값이어야 한다";
}

TEST_F(GameEntryTest, LevelLowerThanExpIsRaisedToCalculatedLevel)
{
    PlayerProgress progress = MakeValidProgress();
    progress.totalExp = LEVEL1_EXP_REQUIREMENT;

    PlayerRef player = GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos);

    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->GetPlayerInfo().level(), 2) << "레벨 1의 요구치와 같은 누적 경험치는 레벨 2다";
}

TEST_F(GameEntryTest, LoadedTotalExpIsSentAsExpInLevel)
{
    PlayerProgress progress = MakeValidProgress();
    progress.totalExp = 20;

    PlayerRef player = GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos);

    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_EXP), 20);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MAX_EXP), LEVEL1_EXP_REQUIREMENT);
    EXPECT_EQ(player->MakeSaveData().progress.totalExp, 20) << "DB에는 누적 경험치를 저장한다";
}

TEST_F(GameEntryTest, TotalExpBeyondMaxLevelIsCut)
{
    PlayerProgress progress = MakeValidProgress();
    progress.totalExp = LEVEL1_EXP_REQUIREMENT + 30;

    PlayerRef player = GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos);

    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->GetPlayerInfo().level(), 2);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_EXP), 0) << "최대 레벨에서는 경험치가 쌓이지 않는다";
    EXPECT_EQ(player->MakeSaveData().progress.totalExp, LEVEL1_EXP_REQUIREMENT)
        << "최대 레벨에 이르는 누적값을 넘는 경험치는 잘라서 저장한다";
}

// 룸에 들어간 플레이어가 C_ENTER_GAME을 다시 보내면 새 플레이어가 세션을 덮어써, 이전 플레이어가
// 룸에 남은 채 끊길 때도 퇴장하지 않는다.
TEST_F(GameEntryTest, SecondEntryIsRejectedAndKeepsFirstPlayer)
{
    const PlayerProgress progress = MakeValidProgress();
    PlayerRef first = GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos);
    ASSERT_NE(first, nullptr);

    PlayerRef second = GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos);

    EXPECT_EQ(second, nullptr);
    EXPECT_EQ(session->GetPlayer(), first) << "세션의 플레이어가 바뀌면 이전 플레이어는 룸에서 빠지지 않는다";
}

/* DB에서 읽은 칸은 믿지 않는다. 잘못된 행이 있으면 입장을 거절하고, 행은 DB에 그대로 둔다. */

TEST_F(GameEntryTest, TwoRowsInOneSlotAreRejected)
{
    PlayerProgress progress = MakeValidProgress();
    progress.items.push_back(MakeLoadedItem(ContainerType::INVENTORY_CONSUMABLE, 5, POTION_TEMPLATE_ID, 2));

    EXPECT_EQ(GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos), nullptr) << "같은 칸의 두 행을 합치면 다음 저장이 한 행을 지운다";
    EXPECT_EQ(session->GetPlayer(), nullptr);
}

TEST_F(GameEntryTest, GearEquippedInOtherPartIsRejected)
{
    PlayerProgress progress = MakeValidProgress();
    erase_if(progress.items, [](const PlacedItem& placed) { return placed.address.container == ContainerType::EQUIPPED; });
    progress.items.push_back(MakeLoadedItem(ContainerType::EQUIPPED, Protocol::GEAR_PART_HELMET, SWORD_TEMPLATE_ID, 1));

    EXPECT_EQ(GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos), nullptr) << "무기를 투구 칸에 입은 채로 두면 그대로 저장된다";
}

TEST_F(GameEntryTest, SecondGearInSamePartIsRejected)
{
    PlayerProgress progress = MakeValidProgress();
    progress.items.push_back(MakeLoadedItem(ContainerType::EQUIPPED, Protocol::GEAR_PART_WEAPON, SWORD_TEMPLATE_ID, 1));

    EXPECT_EQ(GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos), nullptr) << "부위 하나에 두 장비를 겹치면 하나가 사라진다";
    EXPECT_EQ(session->GetPlayer(), nullptr);
}

TEST_F(GameEntryTest, GearPartNoneIsRejected)
{
    PlayerProgress progress = MakeValidProgress();
    erase_if(progress.items, [](const PlacedItem& placed) { return placed.address.container == ContainerType::EQUIPPED; });
    progress.items.push_back(MakeLoadedItem(ContainerType::EQUIPPED, Protocol::GEAR_PART_NONE, SWORD_TEMPLATE_ID, 1));

    EXPECT_EQ(GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos), nullptr) << "착용 장비에는 0번 칸이 없다";
}

TEST_F(GameEntryTest, ItemInOtherKindOfSectionIsRejected)
{
    PlayerProgress progress = MakeValidProgress();
    ASSERT_NO_FATAL_FAILURE(AddRow(progress, MakeInventoryRow(ItemRowType::CONSUMABLE, 7, SWORD_TEMPLATE_ID, 501)));

    EXPECT_EQ(GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos), nullptr) << "소모품 구역의 행에 장비가 들어 있으면 DB가 어긋난 것이다";
}

// 수정 전에는 범위 밖 칸 번호가 Debug 빌드의 DCHECK로 테스트 실행 파일을 멈춰서 빨강 단계를 돌리지 못했다.
TEST_F(GameEntryTest, SlotIndexBeyondSectionSlotCountIsRejected)
{
    PlayerProgress progress = MakeValidProgress();
    ASSERT_NO_FATAL_FAILURE(AddRow(progress, MakeInventoryRow(ItemRowType::CONSUMABLE, InventorySections::SLOT_COUNT, POTION_TEMPLATE_ID, 501)));

    EXPECT_EQ(GameEntry::SpawnPlayer(session, USER_ID, progress, enterRoom, enterPos), nullptr) << "범위 밖 번호는 Release 빌드에서 배열 밖에 쓴다";
}
