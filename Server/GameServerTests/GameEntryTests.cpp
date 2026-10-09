#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Network/GameEntry.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"
#include "Game/Inventory/InventoryComponent.h"
#include "Game/Equipment/EquipmentComponent.h"

/*--------------------------------------------------------------
    게임 입장 테스트

    불러온 진행으로 플레이어를 만들고, 검증을 통과해야 세션에 등록한다. 전에는 불러오기 전에
    세션에 플레이어를 넣어 두고 DB가 그 플레이어에 직접 채웠다. 그래서 불러오기나 검증이 실패해도
    절반만 채운 플레이어가 세션에 남았다.

    픽스처 결합도: 아이템 템플릿과 1레벨짜리 전사 레벨 표를 Gamedata::Install로 주입하고, 소켓 없는
    GameSession을 만든다. DB는 거치지 않고 DAO가 채울 진행 사본을 손으로 만든다.
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
    constexpr int32 SWORD_ATTACK = 15;

    Protocol::Slot MakeLoadedSlot(Protocol::SlotType slotType, int32 slotId, int32 templateId, int32 count)
    {
        Protocol::Slot slot;
        slot.set_type(slotType);
        slot.set_slot_id(slotId);
        slot.mutable_item()->set_template_id(templateId);
        slot.mutable_item()->set_count(count);
        return slot;
    }
}

class GameEntryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ItemTemplate sword;
        sword.templateId = SWORD_TEMPLATE_ID;
        sword.itemType = Protocol::ITEM_TYPE_GEAR;
        sword.gearType = Protocol::GEAR_TYPE_WEAPON;
        sword.physicalAttack = SWORD_ATTACK;

        ItemTemplate potion;
        potion.templateId = POTION_TEMPLATE_ID;
        potion.itemType = Protocol::ITEM_TYPE_CONSUMABLE;
        potion.maxStack = 99;

        LevelTemplate level1;
        level1.level = 1;
        level1.maxHp = MAX_HP;
        level1.maxMp = MAX_MP;
        level1.physicalAttack = PHYSICAL_ATTACK;
        level1.expRequirement = 50;

        GamedataTables tables;
        tables.items[SWORD_TEMPLATE_ID] = sword;
        tables.items[POTION_TEMPLATE_ID] = potion;
        tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ClassLevelTable({ level1 });
        Gamedata::Install(std::move(tables));

        session = make_shared<GameSession>();
        session->_userId = USER_ID;
    }

    void TearDown() override
    {
        session->_player.store(nullptr);
        session.reset();
        Gamedata::Install(GamedataTables());
    }

    // DAO가 채우는 것과 같은 모양의 진행. 무기를 하나 입고 있어서 물리 공격력에 무기 수치가 더해져 있다.
    static PlayerProgress MakeValidProgress()
    {
        PlayerProgress progress;
        progress.playerInfo.set_character_id(CHARACTER_ID);
        progress.playerInfo.set_class_(Protocol::CLASS_TYPE_WARRIOR);
        progress.playerInfo.set_level(1);
        progress.playerInfo.set_room_id(10);
        progress.playerInfo.set_map_id(1);

        auto* stats = progress.statInfo.mutable_info();
        (*stats)[Protocol::STAT_TYPE_EXP] = 20;
        (*stats)[Protocol::STAT_TYPE_HP] = MAX_HP;
        (*stats)[Protocol::STAT_TYPE_MP] = MAX_MP;
        (*stats)[Protocol::STAT_TYPE_PHYSICAL_ATTACK] = PHYSICAL_ATTACK + SWORD_ATTACK;
        (*stats)[Protocol::STAT_TYPE_MAGICAL_ATTACK] = 0;

        progress.possession.set_gold(300);
        (*progress.possession.mutable_equipped_gear())[Protocol::GEAR_TYPE_WEAPON] =
            MakeLoadedSlot(Protocol::SLOT_TYPE_EQUIPPED, Protocol::GEAR_TYPE_WEAPON, SWORD_TEMPLATE_ID, 1);
        *progress.possession.mutable_inventory()->add_consumables() =
            MakeLoadedSlot(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, 5, POTION_TEMPLATE_ID, 3);
        return progress;
    }

    GameSessionRef session;
};

TEST_F(GameEntryTest, ValidProgressRegistersPlayerInSession)
{
    const PlayerProgress progress = MakeValidProgress();

    PlayerRef player = GameEntry::SpawnPlayer(session, progress);

    ASSERT_NE(player, nullptr);
    EXPECT_EQ(session->_player.load(), player);
    EXPECT_EQ(player->_userId, USER_ID);
    EXPECT_EQ(player->_playerInfo->character_id(), CHARACTER_ID);
    EXPECT_EQ(player->GetEnteringRoomId(), 10) << "첫 룸 입장은 불러온 룸으로 간다";
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MAX_HP), MAX_HP);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_MAX_EXP), 50) << "최대 경험치는 저장하지 않고 레벨 표에서 정한다";
    EXPECT_EQ(player->_possession->gold(), 300);
}

TEST_F(GameEntryTest, LoadedSlotsGoToTheirSlotIds)
{
    const PlayerProgress progress = MakeValidProgress();

    PlayerRef player = GameEntry::SpawnPlayer(session, progress);
    ASSERT_NE(player, nullptr);

    const Protocol::Slot* potionSlot = player->_inventory->GetSlot(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, 5);
    ASSERT_TRUE(potionSlot->has_item());
    EXPECT_EQ(potionSlot->item().template_id(), POTION_TEMPLATE_ID);
    EXPECT_EQ(potionSlot->item().count(), 3);
    EXPECT_FALSE(player->_inventory->GetSlot(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, 0)->has_item());

    EXPECT_EQ(player->_equipment->GetSlot(Protocol::GEAR_TYPE_WEAPON)->item().template_id(), SWORD_TEMPLATE_ID);
    EXPECT_TRUE(player->_inventory->GetDirtyFlags(Protocol::ITEM_TYPE_CONSUMABLE)->at(5) == false)
        << "불러온 슬롯을 바뀐 슬롯으로 표시하면 다음 저장이 바뀌지 않은 행까지 쓴다";
}

TEST_F(GameEntryTest, FailedValidationLeavesSessionWithoutPlayer)
{
    PlayerProgress progress = MakeValidProgress();
    (*progress.statInfo.mutable_info())[Protocol::STAT_TYPE_HP] = MAX_HP + 1;

    PlayerRef player = GameEntry::SpawnPlayer(session, progress);

    EXPECT_EQ(player, nullptr);
    EXPECT_EQ(session->_player.load(), nullptr) << "검증에 실패한 플레이어가 세션에 남으면 끊길 때 저장 경로를 탄다";
}

TEST_F(GameEntryTest, UnknownLevelLeavesSessionWithoutPlayer)
{
    PlayerProgress progress = MakeValidProgress();
    progress.playerInfo.set_level(2);

    EXPECT_EQ(GameEntry::SpawnPlayer(session, progress), nullptr);
    EXPECT_EQ(session->_player.load(), nullptr);
}

// TD-019: 룸에 들어간 플레이어가 C_ENTER_GAME을 다시 보내면 새 플레이어가 세션을 덮어써, 이전 플레이어가
// 룸에 남은 채 끊길 때도 퇴장하지 않는다.
TEST_F(GameEntryTest, SecondEntryIsRejectedAndKeepsFirstPlayer)
{
    const PlayerProgress progress = MakeValidProgress();
    PlayerRef first = GameEntry::SpawnPlayer(session, progress);
    ASSERT_NE(first, nullptr);

    PlayerRef second = GameEntry::SpawnPlayer(session, progress);

    EXPECT_EQ(second, nullptr);
    EXPECT_EQ(session->_player.load(), first) << "세션의 플레이어가 바뀌면 이전 플레이어는 룸에서 빠지지 않는다";
}

/* TD-035: DB에서 읽은 슬롯은 믿지 않는다. 잘못된 행이 있으면 입장을 거절하고, 행은 DB에 그대로 둔다. */

TEST_F(GameEntryTest, TwoRowsInOneSlotAreRejected)
{
    PlayerProgress progress = MakeValidProgress();
    *progress.possession.mutable_inventory()->add_consumables() =
        MakeLoadedSlot(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, 5, POTION_TEMPLATE_ID, 2);

    EXPECT_EQ(GameEntry::SpawnPlayer(session, progress), nullptr) << "같은 칸의 두 행을 합치면 다음 저장이 한 행을 지운다";
    EXPECT_EQ(session->_player.load(), nullptr);
}

TEST_F(GameEntryTest, GearEquippedInOtherPartIsRejected)
{
    PlayerProgress progress = MakeValidProgress();
    progress.possession.mutable_equipped_gear()->clear();
    (*progress.possession.mutable_equipped_gear())[Protocol::GEAR_TYPE_HELMET] =
        MakeLoadedSlot(Protocol::SLOT_TYPE_EQUIPPED, Protocol::GEAR_TYPE_HELMET, SWORD_TEMPLATE_ID, 1);
    (*progress.statInfo.mutable_info())[Protocol::STAT_TYPE_PHYSICAL_ATTACK] = PHYSICAL_ATTACK + SWORD_ATTACK;

    EXPECT_EQ(GameEntry::SpawnPlayer(session, progress), nullptr) << "무기를 투구 칸에 입은 채로 두면 그대로 저장된다";
}

TEST_F(GameEntryTest, ItemInOtherKindOfBagIsRejected)
{
    PlayerProgress progress = MakeValidProgress();
    *progress.possession.mutable_inventory()->add_consumables() =
        MakeLoadedSlot(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, 7, SWORD_TEMPLATE_ID, 1);

    EXPECT_EQ(GameEntry::SpawnPlayer(session, progress), nullptr) << "소모품 표의 행에 장비가 들어 있으면 DB가 어긋난 것이다";
}

// 수정 전에는 범위 밖 칸 번호가 Debug 빌드의 DCHECK로 테스트 실행 파일을 멈춰서 빨강 단계를 돌리지 못했다.
TEST_F(GameEntryTest, SlotIdOutOfRangeIsRejected)
{
    PlayerProgress progress = MakeValidProgress();
    *progress.possession.mutable_inventory()->add_consumables() =
        MakeLoadedSlot(Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, MAX_SLOTS, POTION_TEMPLATE_ID, 1);

    EXPECT_EQ(GameEntry::SpawnPlayer(session, progress), nullptr) << "범위 밖 번호는 Release 빌드에서 배열 밖에 쓴다";
}
