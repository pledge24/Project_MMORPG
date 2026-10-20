#include "Core/pch.h"
#include <gtest/gtest.h>
#include "PlayerTestAccess.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Inventory/InventoryComponent.h"

/*--------------------------------------------------------------
    저장 스냅숏 테스트

    접속 종료 저장은 룸 큐 위에서 뜬 사본을 DB 스레드가 읽는다. 사본이 살아 있는
    Player를 가리키면 룸 스레드의 변경과 경쟁하므로, 사본을 뜬 뒤에 플레이어가 바뀌어도
    사본은 그대로여야 한다. 저장은 들고 있는 아이템 전체로 행을 맞추므로 소지품도 통째로 떠야 한다.

    픽스처 결합도: 아이템 템플릿과 인벤토리 구역 표를 Gamedata::Install로 주입하고 Player를 세션 없이 EntityFactory로만 만든다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 SWORD_TEMPLATE_ID = 1001;
    constexpr int64 USER_ID = 7;
}

class PlayerSaveDataTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ItemTemplate sword;
        sword.templateId = SWORD_TEMPLATE_ID;
        sword.itemType = Protocol::ITEM_TYPE_GEAR;
        sword.gearType = Protocol::GEAR_TYPE_WEAPON;

        GamedataTables tables;
        tables.items[SWORD_TEMPLATE_ID] = sword;
        tables.inventorySlotCounts = MakeInventorySections();
        Gamedata::Install(std::move(tables));

        player = EntityFactory::Create<Player>(PlayerSpawnParams());
        ASSERT_NE(player, nullptr);
        PlayerTestAccess::SetUserId(*player, USER_ID);
        player->SetStatValue(Protocol::STAT_TYPE_HP, 100);
        PlayerTestAccess::Possession(*player).set_gold(500);
    }

    void TearDown() override
    {
        player.reset();
        Gamedata::Install(GamedataTables());
    }

    PlayerRef player;
};

TEST_F(PlayerSaveDataTest, SnapshotDoesNotFollowLaterChanges)
{
    PlayerSaveData data = player->MakeSaveData();

    player->SetStatValue(Protocol::STAT_TYPE_HP, 1);
    PlayerTestAccess::Possession(*player).set_gold(0);

    EXPECT_EQ(data.userId, USER_ID);
    EXPECT_EQ(data.progress.statInfo.info().at(Protocol::STAT_TYPE_HP), 100) << "사본이 살아 있는 스탯을 따라가면 DB 스레드가 룸 스레드와 경쟁한다";
    EXPECT_EQ(data.progress.possession.gold(), 500);
}

TEST_F(PlayerSaveDataTest, SnapshotCarriesHeldItems)
{
    RepeatedPtrField<Protocol::Slot> addedSlots;
    ASSERT_TRUE(PlayerTestAccess::Inventory(*player).AddItem(&addedSlots, SWORD_TEMPLATE_ID, 1));
    ASSERT_FALSE(addedSlots.empty());
    const Protocol::Slot added = addedSlots[0];

    PlayerSaveData data = player->MakeSaveData();
    Protocol::Slot removed;
    ASSERT_TRUE(PlayerTestAccess::Inventory(*player).RemoveItem(added, OUT &removed));

    const Protocol::Slot& saved = data.progress.possession.inventory().gear(added.slot_id());
    ASSERT_TRUE(saved.has_item()) << "사본에서 빠진 아이템은 저장 쿼리가 DB에서 지운다";
    EXPECT_EQ(saved.item().template_id(), SWORD_TEMPLATE_ID);
    EXPECT_EQ(saved.item().item_uid(), added.item().item_uid());
}
