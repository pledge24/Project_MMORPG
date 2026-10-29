#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"
#include "Game/Entities/Npc.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"
#include "Network/ItemRequests.h"
#include "PlayerTestAccess.h"
#include "TestItemTables.h"
#include "RecordingSession.h"
#include "RoomManagerTestAccess.h"
#include "TestLevelTables.h"
#include "TestAreaTables.h"

/*--------------------------------------------------------------
    상점 요청 테스트

    상점은 NPC에 딸린다. 구매와 판매 요청은 상점을 연 NPC의 엔티티 번호를 싣고, 서버는 그 NPC가 요청한 플레이어의
    룸에 있는지, 상호작용 사거리에 여유를 더한 평면 거리 안인지 본다. 구매는 그 NPC 상점의 판매 목록에 있는 아이템만 판다.
    상점 위치와 판매 목록을 서버가 보지 않던 때는 조작한 클라이언트가 어디서나 아무 아이템이나 사고팔 수 있었다.

    픽스처 결합도: 맵 하나의 두 채널로 시작하지 않은 룸 둘을 ScopedTestRoomManager에 만든다. 요청한 플레이어를 한 룸의
    원점에 RecordingSession으로 묶어 넣고, NPC는 룸의 SpawnEntity로 둔다. 칼(상점이 판다)과 물약(상점이 팔지 않는다),
    상점 하나와 그 상점을 여는 NPC, 사거리 300cm를 Gamedata::Install로 주입한다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 MAP_ID = 1111;
    constexpr int32 AREA_ID = 10;
    constexpr int32 SWORD_TEMPLATE_ID = 1001;
    constexpr int32 POTION_TEMPLATE_ID = 2000;
    constexpr int32 SHOP_ID = 1;
    constexpr int32 NPC_ID = 7;
    constexpr float INTERACTION_RANGE = 300.f;
    constexpr int64 START_GOLD = 1000;
    constexpr int64 SWORD_PRICE = 100;
    constexpr int64 SWORD_SELL_PRICE = 10;
}

class ShopRequestTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GamedataTables tables;
        TestLevelTables::AddWarriorLevel(tables, 1).MaxHp = 100;
        TestItemTables::AddGear(tables, SWORD_TEMPLATE_ID, Protocol::GEAR_PART_WEAPON);
        tables.items[SWORD_TEMPLATE_ID].BuyPrice = SWORD_PRICE;
        tables.items[SWORD_TEMPLATE_ID].SellPrice = SWORD_SELL_PRICE;
        tables.items[SWORD_TEMPLATE_ID].Sellable = true;
        TestItemTables::AddConsumable(tables, POTION_TEMPLATE_ID, 99);
        tables.items[POTION_TEMPLATE_ID].BuyPrice = 10;
        TestAreaTables::AddArea(tables, TestAreaTables::MakeArea(AREA_ID, MAP_ID, 0.f, 0.f, 5000.f));
        tables.shops[SHOP_ID] = DesignData::ShopRow{ SHOP_ID, { SWORD_TEMPLATE_ID } };
        tables.npcs[NPC_ID] = DesignData::NpcRow{ NPC_ID, SHOP_ID };
        tables.interaction.InteractionRange = INTERACTION_RANGE;
        Gamedata::Install(std::move(tables));

        rooms = make_unique<ScopedTestRoomManager>(2);
        room = rooms->FindRoom(MAP_ID, 1);
        otherRoom = rooms->FindRoom(MAP_ID, 2);
        ASSERT_NE(room, nullptr);
        ASSERT_NE(otherRoom, nullptr);

        session = make_shared<RecordingSession>();
        PlayerProgress progress;
        progress.name = "검은늑대";
        progress.classId = Protocol::CHARACTER_CLASS_WARRIOR;
        progress.level = 1;
        progress.hp = 100;

        PlayerSpawnParams params;
        params.session = session;
        params.progress = &progress;
        requester = EntityFactory::Create<Player>(params);
        ASSERT_NE(requester, nullptr);

        RoomEnterData enterData{};
        enterData.nextRoomId = room->GetRoomId();
        enterData.enterPos = MakePos(0.f, 0.f);
        ASSERT_TRUE(room->EnterPlayer(requester, enterData));
        PlayerTestAccess::Gold(*requester) = START_GOLD;
        session->ClearSent();
    }

    void TearDown() override
    {
        requester.reset();
        room.reset();
        otherRoom.reset();
        rooms.reset();
        Gamedata::Install(GamedataTables());
    }

    static Protocol::PosInfo MakePos(float x, float y)
    {
        Protocol::PosInfo pos;
        pos.mutable_pos()->set_x(x);
        pos.mutable_pos()->set_y(y);
        pos.set_state(Protocol::MOVE_STATE_IDLE);
        return pos;
    }

    /** targetRoom의 (x, y)에 상점 NPC를 두고 엔티티 번호를 돌려준다. */
    int64 PlaceNpc(const RoomRef& targetRoom, float x, float y)
    {
        NpcSpawnParams params;
        params.npcId = NPC_ID;
        params.spawnPos = MakePos(x, y);
        NpcRef npc = targetRoom->SpawnEntity<Npc>(params);
        EXPECT_NE(npc, nullptr);
        return npc ? npc->GetEntityId() : 0;
    }

    Protocol::ItemResult Buy(int64 npcEntityId, int32 templateId)
    {
        Protocol::C_BUY_ITEM pkt;
        pkt.set_template_id(templateId);
        pkt.set_npc_entity_id(npcEntityId);
        ItemRequests::HandleBuyItem(*room, requester, pkt);

        const auto responses = session->SentPackets<Protocol::S_BUY_ITEM>(PKT_S_BUY_ITEM);
        EXPECT_EQ(responses.size(), 1u) << "거절한 요청에도 응답한다";
        session->ClearSent();
        return responses.empty() ? Protocol::ITEM_RESULT_INTERNAL_ERROR : responses[0].result();
    }

    Protocol::ItemResult SellSword(int64 npcEntityId)
    {
        const optional<SlotAddress> address = PlayerTestAccess::AddNewItem(*requester, SWORD_TEMPLATE_ID);
        EXPECT_TRUE(address.has_value());
        const ItemSlotRequest request = MakeInventoryRequest(*requester, address.value_or(SlotAddress{}));

        Protocol::C_SELL_ITEM pkt;
        pkt.mutable_address()->set_container(Protocol::CONTAINER_TYPE_INVENTORY_GEAR);
        pkt.mutable_address()->set_slot_index(request.address.slotIndex);
        pkt.set_item_uid(request.itemUid);
        pkt.set_npc_entity_id(npcEntityId);
        ItemRequests::HandleSellItem(*room, requester, pkt);

        const auto responses = session->SentPackets<Protocol::S_SELL_ITEM>(PKT_S_SELL_ITEM);
        EXPECT_EQ(responses.size(), 1u) << "거절한 요청에도 응답한다";
        session->ClearSent();
        return responses.empty() ? Protocol::ITEM_RESULT_INTERNAL_ERROR : responses[0].result();
    }

    unique_ptr<ScopedTestRoomManager> rooms;
    RoomRef room;
    RoomRef otherRoom;
    PlayerRef requester;
    shared_ptr<RecordingSession> session;
};

/* 구매 */

TEST_F(ShopRequestTest, BuyingFromNearbyNpcSucceeds)
{
    const int64 npcId = PlaceNpc(room, 250.f, 0.f);

    EXPECT_EQ(Buy(npcId, SWORD_TEMPLATE_ID), Protocol::ITEM_RESULT_SUCCESS);
    EXPECT_EQ(requester->GetGold(), START_GOLD - SWORD_PRICE);
}

TEST_F(ShopRequestTest, BuyingItemNotOnNpcShopListIsRejected)
{
    const int64 npcId = PlaceNpc(room, 250.f, 0.f);

    EXPECT_EQ(Buy(npcId, POTION_TEMPLATE_ID), Protocol::ITEM_RESULT_INVALID_REQUEST);
    EXPECT_EQ(requester->GetGold(), START_GOLD) << "판매 목록에 없는 아이템은 아이템 표에 있어도 팔지 않는다";
}

TEST_F(ShopRequestTest, BuyingWithinToleranceBeyondRangeSucceeds)
{
    // 클라이언트는 사거리 안에서 상점을 열지만 서버가 아는 위치는 이동 패킷 하나만큼 늦을 수 있다.
    const int64 npcId = PlaceNpc(room, INTERACTION_RANGE + 50.f, 0.f);

    EXPECT_EQ(Buy(npcId, SWORD_TEMPLATE_ID), Protocol::ITEM_RESULT_SUCCESS);
}

TEST_F(ShopRequestTest, BuyingFromFarNpcIsRejected)
{
    const int64 npcId = PlaceNpc(room, 1000.f, 1000.f);

    EXPECT_EQ(Buy(npcId, SWORD_TEMPLATE_ID), Protocol::ITEM_RESULT_NPC_OUT_OF_REACH);
    EXPECT_EQ(requester->GetGold(), START_GOLD);
}

TEST_F(ShopRequestTest, BuyingFromNpcOfOtherRoomIsRejected)
{
    // 같은 맵의 다른 채널이라 NPC의 위치는 요청한 플레이어 바로 옆이다.
    const int64 otherRoomNpcId = PlaceNpc(otherRoom, 100.f, 0.f);

    EXPECT_EQ(Buy(otherRoomNpcId, SWORD_TEMPLATE_ID), Protocol::ITEM_RESULT_NPC_OUT_OF_REACH);
    EXPECT_EQ(requester->GetGold(), START_GOLD);
}

TEST_F(ShopRequestTest, BuyingWithEntityIdOfNonNpcIsRejected)
{
    PlaceNpc(room, 100.f, 0.f);

    EXPECT_EQ(Buy(requester->GetEntityId(), SWORD_TEMPLATE_ID), Protocol::ITEM_RESULT_NPC_OUT_OF_REACH);
    EXPECT_EQ(Buy(0, SWORD_TEMPLATE_ID), Protocol::ITEM_RESULT_NPC_OUT_OF_REACH) << "NPC 번호를 싣지 않은 옛 요청";
}

/* 판매 */

TEST_F(ShopRequestTest, SellingToNearbyNpcSucceeds)
{
    const int64 npcId = PlaceNpc(room, 0.f, -250.f);

    EXPECT_EQ(SellSword(npcId), Protocol::ITEM_RESULT_SUCCESS);
    EXPECT_EQ(requester->GetGold(), START_GOLD + SWORD_SELL_PRICE);
}

TEST_F(ShopRequestTest, SellingToFarNpcIsRejected)
{
    const int64 npcId = PlaceNpc(room, 0.f, -2000.f);

    EXPECT_EQ(SellSword(npcId), Protocol::ITEM_RESULT_NPC_OUT_OF_REACH);
    EXPECT_EQ(requester->GetGold(), START_GOLD);
}

TEST_F(ShopRequestTest, SellingToNpcOfOtherRoomIsRejected)
{
    const int64 otherRoomNpcId = PlaceNpc(otherRoom, 0.f, 100.f);

    EXPECT_EQ(SellSword(otherRoomNpcId), Protocol::ITEM_RESULT_NPC_OUT_OF_REACH);
    EXPECT_EQ(requester->GetGold(), START_GOLD);
}

/* 배치 */

TEST(NpcPlacementTest, StartedRoomPlacesNpcAtMarkerAndShowsItToNearbyPlayer)
{
    GamedataTables tables;
    TestLevelTables::AddWarriorLevel(tables, 1).MaxHp = 100;
    TestAreaTables::AddArea(tables, TestAreaTables::MakeArea(AREA_ID, MAP_ID, 0.f, 0.f, 5000.f));
    tables.shops[SHOP_ID] = DesignData::ShopRow{ SHOP_ID, { SWORD_TEMPLATE_ID } };
    tables.npcs[NPC_ID] = DesignData::NpcRow{ NPC_ID, SHOP_ID };
    tables.npcPlacements[NPC_ID] = NpcPlacement{ NPC_ID, MAP_ID, vector3D(120.f, -40.f, 20.f), 90.f };
    Gamedata::Install(std::move(tables));

    {
        RoomRef room = CreateTestRoom(1, MAP_ID, 5000.f);
        ASSERT_TRUE(room->Start());

        auto session = make_shared<RecordingSession>();
        PlayerProgress progress;
        progress.name = "검은늑대";
        progress.classId = Protocol::CHARACTER_CLASS_WARRIOR;
        progress.level = 1;
        progress.hp = 100;
        PlayerSpawnParams params;
        params.session = session;
        params.progress = &progress;
        PlayerRef player = EntityFactory::Create<Player>(params);
        ASSERT_NE(player, nullptr);

        RoomEnterData enterData{};
        enterData.nextRoomId = room->GetRoomId();
        ASSERT_TRUE(room->EnterPlayer(player, enterData));

        vector<Protocol::EntityInfo> npcs;
        for (const Protocol::S_SPAWN& spawn : session->SentPackets<Protocol::S_SPAWN>(PKT_S_SPAWN))
        {
            for (const Protocol::EntityInfo& entity : spawn.entities())
            {
                if (entity.has_npc_info())
                    npcs.push_back(entity);
            }
        }

        ASSERT_EQ(npcs.size(), 1u) << "들어온 플레이어는 주변 NPC의 스폰을 받는다";
        EXPECT_EQ(npcs[0].npc_info().npc_id(), NPC_ID);
        EXPECT_FLOAT_EQ(npcs[0].pos_info().pos().x(), 120.f);
        EXPECT_FLOAT_EQ(npcs[0].pos_info().pos().z(), 20.f) << "NPC의 높이는 마커의 바닥 높이다";
        EXPECT_FLOAT_EQ(npcs[0].pos_info().yaw(), 90.f);

        room->LeavePlayer(player, false);
    }

    Gamedata::Install(GamedataTables());
}
