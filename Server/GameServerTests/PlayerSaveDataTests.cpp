#include "Core/pch.h"
#include <gtest/gtest.h>
#include "PlayerTestAccess.h"
#include "TestItemTables.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Inventory/InventoryComponent.h"

/*--------------------------------------------------------------
    저장 스냅숏 테스트

    접속 종료 저장은 룸 큐 위에서 뜬 사본을 DB 스레드가 읽는다. 사본이 살아 있는
    Player를 가리키면 룸 스레드의 변경과 경쟁하므로, 사본을 뜬 뒤에 플레이어가 바뀌어도
    사본은 그대로여야 한다. 저장은 들고 있는 아이템 전체로 행을 맞추므로 소지품도 통째로 떠야 한다.

    픽스처 결합도: 아이템 템플릿을 Gamedata::Install로 주입하고 Player를 세션 없이 EntityFactory로만 만든다.
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
        GamedataTables tables;
        TestItemTables::AddGear(tables, SWORD_TEMPLATE_ID, Protocol::GEAR_PART_WEAPON);
        Gamedata::Install(std::move(tables));

        player = EntityFactory::Create<Player>(PlayerSpawnParams());
        ASSERT_NE(player, nullptr);
        PlayerTestAccess::SetUserId(*player, USER_ID);
        player->SetStatValue(Protocol::STAT_TYPE_HP, 100);
        PlayerTestAccess::Gold(*player) = 500;
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
    PlayerTestAccess::Gold(*player) = 0;

    EXPECT_EQ(data.userId, USER_ID);
    EXPECT_EQ(data.progress.hp, 100) << "사본이 살아 있는 스탯을 따라가면 DB 스레드가 룸 스레드와 경쟁한다";
    EXPECT_EQ(data.progress.gold, 500);
}

TEST_F(PlayerSaveDataTest, SnapshotCarriesHeldItems)
{
    optional<SlotAddress> added = PlayerTestAccess::AddNewItem(*player, SWORD_TEMPLATE_ID);
    ASSERT_TRUE(added.has_value());
    const ItemInstance addedItem = *player->GetInventory().FindItem(added.value());

    PlayerSaveData data = player->MakeSaveData();
    ASSERT_TRUE(PlayerTestAccess::Inventory(*player).RemoveItem(added.value()));

    ASSERT_EQ(data.progress.items.size(), 1u) << "사본에서 빠진 아이템은 저장 쿼리가 DB에서 지운다";
    EXPECT_EQ(data.progress.items[0].address, added.value());
    EXPECT_EQ(data.progress.items[0].item, addedItem);
}
