#include "pch.h"
#include <gtest/gtest.h>
#include "Player.h"
#include "Inventory.h"

/*--------------------------------------------------------------
    저장 스냅숏 테스트

    접속 종료 저장은 룸 큐 위에서 뜬 사본을 DB 스레드가 읽는다. 사본이 살아 있는
    Player를 가리키면 룸 스레드의 변경과 경쟁하므로, 사본을 뜬 뒤에 플레이어가 바뀌어도
    사본은 그대로여야 한다. 어느 인벤토리 칸을 저장할지 정하는 dirty flag도 함께 떠야 한다.

    픽스처 결합도: Gamedata::s_itemDataTable을 손으로 시드하고 Player를 Init()만 한다.
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
        Json sword;
        sword[string(JsonProperty::Item::ItemType)] = "weapon";
        sword[string(JsonProperty::Item::ItemSubtype)] = string(JsonProperty::Item::GearSubtype_Sword);
        sword[string(JsonProperty::Item::MaxStack)] = 1;
        Gamedata::s_itemDataTable[SWORD_TEMPLATE_ID] = sword;

        player = make_shared<Player>();
        ASSERT_TRUE(player->Init());
        player->_userId = USER_ID;
        player->SetStatValue(Protocol::STAT_TYPE_HP, 100);
        player->_possession->set_gold(500);
    }

    void TearDown() override
    {
        player.reset();
        Gamedata::s_itemDataTable.clear();
    }

    PlayerRef player;
};

TEST_F(PlayerSaveDataTest, SnapshotDoesNotFollowLaterChanges)
{
    PlayerSaveData data = player->MakeSaveData();

    player->SetStatValue(Protocol::STAT_TYPE_HP, 1);
    player->_possession->set_gold(0);

    EXPECT_EQ(data.userId, USER_ID);
    EXPECT_EQ(data.statInfo.info().at(Protocol::STAT_TYPE_HP), 100) << "사본이 살아 있는 스탯을 따라가면 DB 스레드가 룸 스레드와 경쟁한다";
    EXPECT_EQ(data.possession.gold(), 500);
}

TEST_F(PlayerSaveDataTest, SnapshotCarriesDirtyFlags)
{
    RepeatedPtrField<Protocol::Slot> addedSlots;
    ASSERT_TRUE(player->_inventory->AddItem(&addedSlots, SWORD_TEMPLATE_ID, 1));
    ASSERT_FALSE(addedSlots.empty());
    const int32 slotId = addedSlots[0].slot_id();

    PlayerSaveData data = player->MakeSaveData();
    player->_inventory->ClearDirtyFlags();

    ASSERT_TRUE(data.gearDirtyFlags.has_value());
    ASSERT_LT(slotId, (int32)data.gearDirtyFlags->size());
    EXPECT_TRUE((*data.gearDirtyFlags)[slotId]) << "dirty flag가 빠지면 새로 얻은 아이템이 저장되지 않는다";
    EXPECT_EQ(data.possession.inventory().gear(slotId).item().template_id(), SWORD_TEMPLATE_ID);
}
