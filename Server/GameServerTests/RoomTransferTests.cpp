#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/RoomTransfer.h"

/*--------------------------------------------------------------
    룸 이동 판정 테스트

    리스폰 요청은 검증 없이 처리되어, 살아 있는 플레이어가 어디서든 마을로 이동했고
    목적지 룸이 없는 리스폰 유형은 서버를 죽였다. 입장 요청의 룸 번호 검증은 최초 입장과
    맵 간 이동 분기에 같은 코드로 두 번 있었다.
---------------------------------------------------------------*/

namespace
{
    Protocol::C_ENTER_ROOM MakeEnterRequest(Protocol::EnterType enterType)
    {
        Protocol::C_ENTER_ROOM pkt;
        pkt.set_enter_type(enterType);
        return pkt;
    }

    Protocol::C_ENTER_ROOM MakeEnterRequest(Protocol::EnterType enterType, int32 roomId)
    {
        Protocol::C_ENTER_ROOM pkt = MakeEnterRequest(enterType);
        pkt.set_room_id(roomId);
        return pkt;
    }
}

TEST(RoomTransferTest, InitialEnterWithEnteringRoomIdPasses)
{
    EXPECT_FALSE(RoomTransfer::ValidateEnterRequest(MakeEnterRequest(Protocol::ENTER_TYPE_INITIAL, 10), 10).has_value());
}

TEST(RoomTransferTest, InitialEnterWithoutRoomIdIsRejected)
{
    EXPECT_TRUE(RoomTransfer::ValidateEnterRequest(MakeEnterRequest(Protocol::ENTER_TYPE_INITIAL), 10).has_value());
}

TEST(RoomTransferTest, CrossMapTransferToOtherRoomIsRejected)
{
    EXPECT_TRUE(RoomTransfer::ValidateEnterRequest(MakeEnterRequest(Protocol::ENTER_TYPE_CROSS_MAP_TRANSFER, 20), 10).has_value())
        << "C_ENTER_MAP으로 받아 둔 룸이 아닌 곳으로는 입장할 수 없다";
}

TEST(RoomTransferTest, CrossMapTransferToEnteringRoomPasses)
{
    EXPECT_FALSE(RoomTransfer::ValidateEnterRequest(MakeEnterRequest(Protocol::ENTER_TYPE_CROSS_MAP_TRANSFER, 20), 20).has_value());
}

TEST(RoomTransferTest, SameMapTransferNeedsPortalId)
{
    EXPECT_TRUE(RoomTransfer::ValidateEnterRequest(MakeEnterRequest(Protocol::ENTER_TYPE_SAME_MAP_TRANSFER), -1).has_value());

    Protocol::C_ENTER_ROOM pkt = MakeEnterRequest(Protocol::ENTER_TYPE_SAME_MAP_TRANSFER);
    pkt.set_portal_id(11);
    EXPECT_FALSE(RoomTransfer::ValidateEnterRequest(pkt, -1).has_value());
}

TEST(RoomTransferTest, RespawnAndNoneEnterTypesAreRejected)
{
    EXPECT_TRUE(RoomTransfer::ValidateEnterRequest(MakeEnterRequest(Protocol::ENTER_TYPE_RESPAWN, 10), 10).has_value())
        << "리스폰 입장은 서버만 만드는 값이다";
    EXPECT_TRUE(RoomTransfer::ValidateEnterRequest(MakeEnterRequest(Protocol::ENTER_TYPE_NONE, 10), 10).has_value());
}

TEST(RoomTransferTest, PortalEnterDataUsesPortalDestination)
{
    PortalTemplate portal;
    portal.portalId = 11;
    portal.dstRoomId = 20;
    portal.dstPos = TemplatePos{ -8000.f, 10000.f, 50.f };
    portal.dstYaw = 180.f;

    const RoomEnterData enterData = RoomTransfer::MakePortalEnterData(portal, 7);

    EXPECT_EQ(enterData.nextRoomId, 20);
    EXPECT_EQ(enterData.enterType, Protocol::ENTER_TYPE_SAME_MAP_TRANSFER);
    ASSERT_TRUE(enterData.enterPos.has_value()) << "클라이언트는 내 플레이어를 enter_pos로 옮기므로 반드시 채워야 한다";
    EXPECT_EQ(enterData.enterPos->entity_id(), 7);
    EXPECT_FLOAT_EQ(enterData.enterPos->pos().x(), -8000.f);
    EXPECT_FLOAT_EQ(enterData.enterPos->pos().y(), 10000.f);
    EXPECT_FLOAT_EQ(enterData.enterPos->pos().z(), 50.f);
    EXPECT_FLOAT_EQ(enterData.enterPos->yaw(), 180.f);
    EXPECT_EQ(enterData.enterPos->state(), Protocol::MOVE_STATE_IDLE);
}

TEST(RoomTransferTest, DeadPlayerTownRespawnPasses)
{
    EXPECT_FALSE(RoomTransfer::ValidateRespawn(true, Protocol::RESPAWN_TYPE_TOWN).has_value());
}

TEST(RoomTransferTest, LivingPlayerRespawnIsRejected)
{
    EXPECT_TRUE(RoomTransfer::ValidateRespawn(false, Protocol::RESPAWN_TYPE_TOWN).has_value())
        << "살아 있는 플레이어가 리스폰하면 어디서든 마을로 이동한다";
}

TEST(RoomTransferTest, UnsupportedRespawnTypesAreRejected)
{
    const Protocol::RespawnType unsupportedTypes[] = {
        Protocol::RESPAWN_TYPE_NONE,
        Protocol::RESPAWN_TYPE_CHECKPOINT,
        Protocol::RESPAWN_TYPE_RESURRECTION_ITEM,
        Protocol::RESPAWN_TYPE_IN_PLACE,
        Protocol::RESPAWN_TYPE_PARTY_MEMBER,
        Protocol::RESPAWN_TYPE_GUILD_BASE,
        Protocol::RESPAWN_TYPE_CASH_ITEM,
        Protocol::RESPAWN_TYPE_BATTLE_RESURRECTION,
    };

    for (Protocol::RespawnType respawnType : unsupportedTypes)
    {
        EXPECT_TRUE(RoomTransfer::ValidateRespawn(true, respawnType).has_value())
            << "목적지가 없는 리스폰 유형을 통과시키면 서버가 널 룸을 역참조한다. 유형: " << respawnType;
    }
}

// 마을 리스폰의 목적지는 맵 표에서 읽는다. 룸 객체를 보지 않으므로 퇴장한 뒤의 저장에서도 쓸 수 있다.
TEST(RoomTransferTest, TownRespawnComesFromMapTable)
{
    MapTemplate town;
    town.templateId = 10;
    town.respawnPoint = TemplatePos{ 100.f, 200.f, 30.f };

    GamedataTables tables;
    tables.maps[town.templateId] = town;
    tables.townRoomId = town.templateId;
    Gamedata::Install(std::move(tables));

    optional<TownRespawn> respawn = RoomTransfer::FindTownRespawn();
    Gamedata::Install(GamedataTables());

    ASSERT_TRUE(respawn.has_value());
    EXPECT_EQ(respawn->roomId, 10);
    EXPECT_FLOAT_EQ(respawn->pos.pos().x(), 100.f);
    EXPECT_FLOAT_EQ(respawn->pos.pos().y(), 200.f);
    EXPECT_FLOAT_EQ(respawn->pos.pos().z(), 30.f);
    EXPECT_EQ(respawn->pos.state(), Protocol::MOVE_STATE_IDLE);
}

TEST(RoomTransferTest, TownWithoutRespawnPointHasNoTownRespawn)
{
    MapTemplate town;
    town.templateId = 10;

    GamedataTables tables;
    tables.maps[town.templateId] = town;
    tables.townRoomId = town.templateId;
    Gamedata::Install(std::move(tables));

    EXPECT_FALSE(RoomTransfer::FindTownRespawn().has_value());
    Gamedata::Install(GamedataTables());
}

//~ 위치를 보는 판정(TD-001)

namespace
{
    constexpr int32 MAP_A = 1111;
    constexpr int32 MAP_B = 2222;
    constexpr int32 TOWN_ROOM = 10;
    constexpr int32 FIELD_ROOM = 20;
    constexpr int32 OTHER_MAP_ROOM = 50;
    constexpr float PORTAL_RADIUS = 500.f;

    Protocol::PosInfo MakePlayerPos(float x, float y, float z = 0.f)
    {
        Protocol::PosInfo pos;
        pos.mutable_pos()->set_x(x);
        pos.mutable_pos()->set_y(y);
        pos.mutable_pos()->set_z(z);
        return pos;
    }

    PortalTemplate MakePortal(int32 portalId, int32 dstRoomId, float srcX, float srcY)
    {
        PortalTemplate portal;
        portal.portalId = portalId;
        portal.dstRoomId = dstRoomId;
        portal.srcPos = TemplatePos{ srcX, srcY, 0.f };
        return portal;
    }

    Protocol::C_ENTER_MAP MakeEnterMap(int32 mapId, int32 roomId)
    {
        Protocol::C_ENTER_MAP pkt;
        pkt.set_map_id(mapId);
        pkt.set_room_id(roomId);
        return pkt;
    }
}

// 마을(MAP_A)에는 같은 맵의 사냥터로 가는 포털과 다른 맵(MAP_B)의 룸으로 가는 포털이 하나씩 있다.
class RoomTransferMapTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        town.templateId = TOWN_ROOM;
        town.mapId = MAP_A;
        town.portalRadius = PORTAL_RADIUS;
        town.portals.push_back(MakePortal(11, FIELD_ROOM, -1200.f, 1000.f));
        town.portals.push_back(MakePortal(12, OTHER_MAP_ROOM, 2000.f, 0.f));

        MapTemplate field;
        field.templateId = FIELD_ROOM;
        field.mapId = MAP_A;
        field.portalRadius = PORTAL_RADIUS;

        MapTemplate otherMapRoom;
        otherMapRoom.templateId = OTHER_MAP_ROOM;
        otherMapRoom.mapId = MAP_B;
        otherMapRoom.portalRadius = PORTAL_RADIUS;

        GamedataTables tables;
        tables.maps[TOWN_ROOM] = town;
        tables.maps[FIELD_ROOM] = field;
        tables.maps[OTHER_MAP_ROOM] = otherMapRoom;
        Gamedata::Install(std::move(tables));
    }

    void TearDown() override
    {
        Gamedata::Install(GamedataTables());
    }

    MapTemplate town;
};

TEST_F(RoomTransferMapTest, PortalUseWithinRadiusPasses)
{
    const PortalTemplate& portal = town.portals[0];
    EXPECT_FALSE(RoomTransfer::ValidatePortalUse(portal, PORTAL_RADIUS, MakePlayerPos(-1200.f + 300.f, 1000.f + 400.f)).has_value())
        << "반경 경계(거리 500)는 통과한다";
    EXPECT_FALSE(RoomTransfer::ValidatePortalUse(portal, PORTAL_RADIUS, MakePlayerPos(-1200.f, 1000.f, 5000.f)).has_value())
        << "높이는 보지 않는다. 포털 좌표의 z는 바닥이고 플레이어의 z는 캡슐 중심이다";
}

TEST_F(RoomTransferMapTest, PortalUseOutsideRadiusIsRejected)
{
    const PortalTemplate& portal = town.portals[0];
    EXPECT_TRUE(RoomTransfer::ValidatePortalUse(portal, PORTAL_RADIUS, MakePlayerPos(-1200.f + 301.f, 1000.f + 400.f)).has_value())
        << "조작한 클라이언트는 룸 안 어디서든 포털을 탄다";
}

TEST_F(RoomTransferMapTest, FirstEnterMapOnlyToLoadedRoom)
{
    EXPECT_FALSE(RoomTransfer::ValidateFirstEnterMap(MakeEnterMap(MAP_A, FIELD_ROOM), FIELD_ROOM).has_value());
    EXPECT_TRUE(RoomTransfer::ValidateFirstEnterMap(MakeEnterMap(MAP_A, TOWN_ROOM), FIELD_ROOM).has_value())
        << "불러온 룸이 아닌 곳으로 첫 입장을 하면 아무 룸에서나 시작한다";
    EXPECT_TRUE(RoomTransfer::ValidateFirstEnterMap(MakeEnterMap(MAP_B, FIELD_ROOM), FIELD_ROOM).has_value())
        << "맵 번호는 룸의 맵과 같아야 한다";
}

TEST_F(RoomTransferMapTest, CrossMapEnterThroughNearbyPortalPasses)
{
    EXPECT_FALSE(RoomTransfer::ValidateCrossMapEnter(MakeEnterMap(MAP_B, OTHER_MAP_ROOM), town, MakePlayerPos(2100.f, 0.f)).has_value());
}

TEST_F(RoomTransferMapTest, CrossMapEnterIsRejectedWithoutNearbyPortal)
{
    EXPECT_TRUE(RoomTransfer::ValidateCrossMapEnter(MakeEnterMap(MAP_B, OTHER_MAP_ROOM), town, MakePlayerPos(0.f, 0.f)).has_value())
        << "포털 반경 밖에서는 맵 간 이동을 할 수 없다";
    EXPECT_TRUE(RoomTransfer::ValidateCrossMapEnter(MakeEnterMap(MAP_A, 99), town, MakePlayerPos(2100.f, 0.f)).has_value())
        << "현재 룸의 포털이 가지 않는 룸으로는 갈 수 없다";
}

TEST_F(RoomTransferMapTest, CrossMapEnterRejectsSameMapRoomAndWrongMapId)
{
    EXPECT_TRUE(RoomTransfer::ValidateCrossMapEnter(MakeEnterMap(MAP_A, FIELD_ROOM), town, MakePlayerPos(-1200.f, 1000.f)).has_value())
        << "같은 맵 안의 이동은 포털 이동(C_ENTER_ROOM)으로 한다";
    EXPECT_TRUE(RoomTransfer::ValidateCrossMapEnter(MakeEnterMap(MAP_A, OTHER_MAP_ROOM), town, MakePlayerPos(2100.f, 0.f)).has_value())
        << "맵 번호는 목적지 룸의 맵과 같아야 한다";
}
