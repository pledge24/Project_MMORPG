#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/RoomTransfer.h"

/*--------------------------------------------------------------
    룸 이동 판정과 입장할 룸 배정 테스트

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

//~ 위치를 보는 판정

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

//~ 입장할 룸 배정

// DB와 기획 데이터는 맵과 좌표만 안다. 들어갈 룸은 서버가 맵 표 행의 범위로 고른다.
// 마을(TOWN_ROOM, 중심 (0, 0), 반경 1000)과 그 서쪽에 붙은 사냥터(FIELD_ROOM, 중심 (-3000, 0), 반경 2000)가 x = -1000에서
// 경계를 함께 쓴다. 다른 맵(MAP_B)의 룸은 마을과 같은 좌표 범위에 있다.
class RoomAssignmentTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        MapTemplate town = MakeRange(TOWN_ROOM, MAP_A, 0.f, 0.f, 1000.f);
        town.respawnPoint = TemplatePos{ 100.f, 200.f, 30.f };

        GamedataTables tables;
        tables.maps[TOWN_ROOM] = town;
        tables.maps[FIELD_ROOM] = MakeRange(FIELD_ROOM, MAP_A, -3000.f, 0.f, 2000.f);
        tables.maps[OTHER_MAP_ROOM] = MakeRange(OTHER_MAP_ROOM, MAP_B, 0.f, 0.f, 1000.f);
        tables.townRoomId = TOWN_ROOM;
        Gamedata::Install(std::move(tables));
    }

    void TearDown() override
    {
        Gamedata::Install(GamedataTables());
    }

    static MapTemplate MakeRange(int32 roomId, int32 mapId, float centerX, float centerY, float halfExtent)
    {
        MapTemplate range;
        range.templateId = roomId;
        range.mapId = mapId;
        range.center = TemplatePos{ centerX, centerY, 0.f };
        range.depthHalfExtent = halfExtent;
        range.widthHalfExtent = halfExtent;
        return range;
    }

    static void ExpectTownRespawn(const optional<RoomAssignment>& assignment)
    {
        ASSERT_TRUE(assignment.has_value());
        EXPECT_EQ(assignment->roomId, TOWN_ROOM);
        EXPECT_FLOAT_EQ(assignment->pos.pos().x(), 100.f);
        EXPECT_FLOAT_EQ(assignment->pos.pos().y(), 200.f);
        EXPECT_FLOAT_EQ(assignment->pos.pos().z(), 30.f);
    }
};

TEST_F(RoomAssignmentTest, PosInsideRangeKeepsPosInThatRoom)
{
    Protocol::PosInfo pos = MakePlayerPos(-3500.f, 1500.f, 80.f);
    pos.set_yaw(90.f);

    const optional<RoomAssignment> assignment = RoomTransfer::AssignRoom(MAP_A, pos);

    ASSERT_TRUE(assignment.has_value());
    EXPECT_EQ(assignment->roomId, FIELD_ROOM);
    EXPECT_FLOAT_EQ(assignment->pos.pos().x(), -3500.f);
    EXPECT_FLOAT_EQ(assignment->pos.pos().y(), 1500.f);
    EXPECT_FLOAT_EQ(assignment->pos.pos().z(), 80.f) << "높이는 범위 판정에 쓰지 않지만 그대로 돌려준다";
    EXPECT_FLOAT_EQ(assignment->pos.yaw(), 90.f);
}

TEST_F(RoomAssignmentTest, PosOnRangeBoundaryBelongsToThatRoom)
{
    const optional<RoomAssignment> maxEdge = RoomTransfer::AssignRoom(MAP_A, MakePlayerPos(1000.f, 1000.f));
    ASSERT_TRUE(maxEdge.has_value());
    EXPECT_EQ(maxEdge->roomId, TOWN_ROOM)
        << "이동 판정은 경계 위의 점을 룸 안쪽으로 받는다. 경계에서 저장한 플레이어를 마을로 보내면 안 된다";

    const optional<RoomAssignment> minEdge = RoomTransfer::AssignRoom(MAP_A, MakePlayerPos(-5000.f, -2000.f));
    ASSERT_TRUE(minEdge.has_value());
    EXPECT_EQ(minEdge->roomId, FIELD_ROOM);
    EXPECT_FLOAT_EQ(minEdge->pos.pos().x(), -5000.f);
}

TEST_F(RoomAssignmentTest, SharedBoundaryGoesToSmallerMapTableRow)
{
    const optional<RoomAssignment> assignment = RoomTransfer::AssignRoom(MAP_A, MakePlayerPos(-1000.f, 0.f));

    ASSERT_TRUE(assignment.has_value());
    EXPECT_EQ(assignment->roomId, TOWN_ROOM) << "두 범위가 함께 쓰는 경계는 맵 표 행 번호가 작은 쪽이 갖는다";
}

TEST_F(RoomAssignmentTest, PosOutsideEveryRangeGoesToTownRespawn)
{
    ExpectTownRespawn(RoomTransfer::AssignRoom(MAP_A, MakePlayerPos(1000.1f, 0.f)));
    ExpectTownRespawn(RoomTransfer::AssignRoom(MAP_A, MakePlayerPos(-3000.f, 2000.1f)));
}

TEST_F(RoomAssignmentTest, RangeOfOtherMapIsNotCandidate)
{
    // (-3000, 0)은 MAP_A 사냥터의 범위 안이지만 MAP_B에는 그 좌표를 덮는 룸이 없다.
    ExpectTownRespawn(RoomTransfer::AssignRoom(MAP_B, MakePlayerPos(-3000.f, 0.f)));

    const optional<RoomAssignment> otherMap = RoomTransfer::AssignRoom(MAP_B, MakePlayerPos(0.f, 0.f));
    ASSERT_TRUE(otherMap.has_value());
    EXPECT_EQ(otherMap->roomId, OTHER_MAP_ROOM) << "범위가 같은 마을이 룸 번호가 더 작아도 맵이 다르면 후보가 아니다";
}

TEST_F(RoomAssignmentTest, MapIdNotInMapTableGoesToTownRespawn)
{
    ExpectTownRespawn(RoomTransfer::AssignRoom(9999, MakePlayerPos(0.f, 0.f)));
}

TEST_F(RoomAssignmentTest, NoAssignmentWithoutTownRespawn)
{
    GamedataTables tables;
    tables.maps[FIELD_ROOM] = MakeRange(FIELD_ROOM, MAP_A, -3000.f, 0.f, 2000.f);
    tables.maps[TOWN_ROOM] = MakeRange(TOWN_ROOM, MAP_A, 0.f, 0.f, 1000.f);
    tables.townRoomId = TOWN_ROOM;
    Gamedata::Install(std::move(tables));

    EXPECT_TRUE(RoomTransfer::AssignRoom(MAP_A, MakePlayerPos(-3000.f, 0.f)).has_value())
        << "범위 안의 좌표는 마을 리스폰 지점이 없어도 배정한다";
    EXPECT_FALSE(RoomTransfer::AssignRoom(MAP_A, MakePlayerPos(50000.f, 0.f)).has_value());
}
