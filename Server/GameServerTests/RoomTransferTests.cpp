#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/RoomTransfer.h"
#include "TestAreaTables.h"

/*--------------------------------------------------------------
    룸 이동 판정과 입장할 룸 배정 테스트

    리스폰 요청은 검증 없이 처리되어, 살아 있는 플레이어가 어디서든 마을로 이동했고
    목적지 룸이 없는 리스폰 유형은 서버를 죽였다.
---------------------------------------------------------------*/

TEST(RoomTransferTest, PortalEnterDataUsesPortalDestination)
{
    DesignData::PortalRow portal;
    portal.Id = 11;
    portal.DestAreaId = 20;
    portal.DestX = -8000.f;
    portal.DestY = 10000.f;
    portal.DestZ = 50.f;
    portal.DestYaw = 180.f;

    const RoomEnterData enterData = RoomTransfer::MakePortalEnterData(portal);

    EXPECT_EQ(enterData.nextRoomId, 20) << "지금은 도착 지역 하나를 룸 하나가 맡는다";
    ASSERT_TRUE(enterData.enterPos.has_value()) << "비어 있으면 포털의 도착 위치가 아니라 룸 중심에 선다";
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
    EXPECT_EQ(RoomTransfer::ValidateRespawn(false, Protocol::RESPAWN_TYPE_TOWN), Protocol::ENTRY_RESULT_NOT_DEAD)
        << "살아 있는 플레이어가 리스폰하면 어디서든 마을로 이동한다";
}

TEST(RoomTransferTest, UnsupportedRespawnTypesAreRejected)
{
    // 클라이언트가 보낸 값이라 정의되지 않은 번호도 온다.
    const Protocol::RespawnType unsupportedTypes[] = {
        Protocol::RESPAWN_TYPE_NONE,
        static_cast<Protocol::RespawnType>(2),
    };

    for (Protocol::RespawnType respawnType : unsupportedTypes)
    {
        EXPECT_EQ(RoomTransfer::ValidateRespawn(true, respawnType), Protocol::ENTRY_RESULT_UNSUPPORTED_RESPAWN_TYPE)
            << "목적지가 없는 리스폰 유형을 통과시키면 서버가 널 룸을 역참조한다. 유형: " << respawnType;
    }
}

// 마을 리스폰의 목적지는 지역 표에서 읽는다. 룸 객체를 보지 않으므로 퇴장한 뒤의 저장에서도 쓸 수 있다.
TEST(RoomTransferTest, TownRespawnComesFromAreaTable)
{
    DesignData::AreaRow town = TestAreaTables::MakeArea(10, 1001);
    TestAreaTables::SetRespawnPoint(town, 100.f, 200.f, 30.f);

    GamedataTables tables;
    TestAreaTables::AddTown(tables, town);
    Gamedata::Install(std::move(tables));

    optional<TownRespawn> respawn = RoomTransfer::FindTownRespawn();
    Gamedata::Install(GamedataTables());

    ASSERT_TRUE(respawn.has_value());
    EXPECT_EQ(respawn->mapId, 1001) << "사망한 채 끊긴 플레이어는 마을의 맵으로 저장한다";
    EXPECT_EQ(respawn->roomId, 10);
    EXPECT_FLOAT_EQ(respawn->pos.pos().x(), 100.f);
    EXPECT_FLOAT_EQ(respawn->pos.pos().y(), 200.f);
    EXPECT_FLOAT_EQ(respawn->pos.pos().z(), 30.f);
    EXPECT_EQ(respawn->pos.state(), Protocol::MOVE_STATE_IDLE);
}

TEST(RoomTransferTest, TownWithoutRespawnPointHasNoTownRespawn)
{
    GamedataTables tables;
    TestAreaTables::AddTown(tables, TestAreaTables::MakeArea(10, 1001));
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
}

// 마을(MAP_A)에는 같은 맵의 사냥터로 가는 포털과 다른 맵(MAP_B)의 룸으로 가는 포털이 하나씩 있다.
class RoomTransferMapTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        town = TestAreaTables::MakeArea(TOWN_ROOM, MAP_A);
        town.Portals.push_back(TestAreaTables::MakePortal(11, FIELD_ROOM, -1200.f, 1000.f, PORTAL_RADIUS));
        town.Portals.push_back(TestAreaTables::MakePortal(12, OTHER_MAP_ROOM, 2000.f, 0.f, PORTAL_RADIUS));

        GamedataTables tables;
        TestAreaTables::AddArea(tables, town);
        TestAreaTables::AddArea(tables, TestAreaTables::MakeArea(FIELD_ROOM, MAP_A));
        TestAreaTables::AddArea(tables, TestAreaTables::MakeArea(OTHER_MAP_ROOM, MAP_B));
        Gamedata::Install(std::move(tables));
    }

    void TearDown() override
    {
        Gamedata::Install(GamedataTables());
    }

    DesignData::AreaRow town;
};

TEST_F(RoomTransferMapTest, PortalUseWithinRadiusPasses)
{
    const DesignData::PortalRow& portal = town.Portals[0];
    EXPECT_FALSE(RoomTransfer::ValidatePortalUse(portal, MakePlayerPos(-1200.f + 300.f, 1000.f + 400.f)).has_value())
        << "반경 경계(거리 500)는 통과한다";
    EXPECT_FALSE(RoomTransfer::ValidatePortalUse(portal, MakePlayerPos(-1200.f, 1000.f, 5000.f)).has_value())
        << "높이는 보지 않는다. 포털 좌표의 z는 바닥이고 플레이어의 z는 캡슐 중심이다";
}

TEST_F(RoomTransferMapTest, PortalUseOutsideRadiusIsRejected)
{
    const DesignData::PortalRow& portal = town.Portals[0];
    EXPECT_TRUE(RoomTransfer::ValidatePortalUse(portal, MakePlayerPos(-1200.f + 301.f, 1000.f + 400.f)).has_value())
        << "조작한 클라이언트는 룸 안 어디서든 포털을 탄다";
}

// 반경은 포털마다 다르다. 지역 하나의 포털이 같은 반경을 쓰지 않는다.
TEST_F(RoomTransferMapTest, PortalUseUsesThatPortalsRadius)
{
    const DesignData::PortalRow narrow = TestAreaTables::MakePortal(13, FIELD_ROOM, 0.f, 0.f, 100.f);
    EXPECT_FALSE(RoomTransfer::ValidatePortalUse(narrow, MakePlayerPos(100.f, 0.f)).has_value());
    EXPECT_TRUE(RoomTransfer::ValidatePortalUse(narrow, MakePlayerPos(101.f, 0.f)).has_value());
}

//~ 입장할 룸 배정

// DB는 맵과 좌표만 안다. 들어갈 룸은 서버가 좌표가 드는 지역으로 고른다.
// 마을(TOWN_ROOM, 중심 (0, 0), 반경 1000)과 그 서쪽에 붙은 사냥터(FIELD_ROOM, 중심 (-3000, 0), 반경 2000)가 x = -1000에서
// 경계를 함께 쓴다. 다른 맵(MAP_B)의 룸은 마을과 같은 좌표 범위에 있다.
class RoomAssignmentTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        DesignData::AreaRow town = TestAreaTables::MakeArea(TOWN_ROOM, MAP_A, 0.f, 0.f, 1000.f);
        TestAreaTables::SetRespawnPoint(town, 100.f, 200.f, 30.f);

        GamedataTables tables;
        TestAreaTables::AddTown(tables, town);
        TestAreaTables::AddArea(tables, TestAreaTables::MakeArea(FIELD_ROOM, MAP_A, -3000.f, 0.f, 2000.f));
        TestAreaTables::AddArea(tables, TestAreaTables::MakeArea(OTHER_MAP_ROOM, MAP_B, 0.f, 0.f, 1000.f));
        Gamedata::Install(std::move(tables));
    }

    void TearDown() override
    {
        Gamedata::Install(GamedataTables());
    }

    static void ExpectTownRespawn(const optional<RoomAssignment>& assignment)
    {
        ASSERT_TRUE(assignment.has_value());
        EXPECT_EQ(assignment->mapId, MAP_A) << "마을로 보내면 맵도 마을의 맵으로 바뀐다";
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

TEST_F(RoomAssignmentTest, SharedBoundaryGoesToSmallerAreaId)
{
    const optional<RoomAssignment> assignment = RoomTransfer::AssignRoom(MAP_A, MakePlayerPos(-1000.f, 0.f));

    ASSERT_TRUE(assignment.has_value());
    EXPECT_EQ(assignment->roomId, TOWN_ROOM) << "두 지역이 함께 쓰는 경계는 지역 번호가 작은 쪽이 갖는다";
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
    EXPECT_EQ(otherMap->mapId, MAP_B);
}

TEST_F(RoomAssignmentTest, MapIdNotInMapTableGoesToTownRespawn)
{
    ExpectTownRespawn(RoomTransfer::AssignRoom(9999, MakePlayerPos(0.f, 0.f)));
}

TEST_F(RoomAssignmentTest, NoAssignmentWithoutTownRespawn)
{
    GamedataTables tables;
    TestAreaTables::AddArea(tables, TestAreaTables::MakeArea(FIELD_ROOM, MAP_A, -3000.f, 0.f, 2000.f));
    TestAreaTables::AddTown(tables, TestAreaTables::MakeArea(TOWN_ROOM, MAP_A, 0.f, 0.f, 1000.f));
    Gamedata::Install(std::move(tables));

    EXPECT_TRUE(RoomTransfer::AssignRoom(MAP_A, MakePlayerPos(-3000.f, 0.f)).has_value())
        << "지역 안의 좌표는 마을 리스폰 지점이 없어도 배정한다";
    EXPECT_FALSE(RoomTransfer::AssignRoom(MAP_A, MakePlayerPos(50000.f, 0.f)).has_value());
}
