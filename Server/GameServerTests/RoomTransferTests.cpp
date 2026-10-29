#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/RoomTransfer.h"
#include "TestAreaTables.h"

/*--------------------------------------------------------------
    룸 이동 판정과 입장 위치, 채널 테스트

    리스폰 요청은 검증 없이 처리되어, 살아 있는 플레이어가 어디서든 마을로 이동했고
    목적지 룸이 없는 리스폰 유형은 서버를 죽였다. 룸이 맵 하나 전체의 사본이 된 뒤로는 판정이 들어갈 맵과 위치를 정하고,
    룸은 그 맵의 채널 중에서 고른다.
---------------------------------------------------------------*/

TEST(RoomTransferTest, PortalDestinationUsesDestAreaMapAndPlacedPoint)
{
    GamedataTables tables;
    TestAreaTables::AddArea(tables, TestAreaTables::MakeArea(20, 2222));
    Gamedata::Install(std::move(tables));

    DesignData::PortalRow portal;
    portal.Id = 11;
    portal.DestAreaId = 20;

    PortalPlacement placement;
    placement.id = 11;
    placement.dest = vector3D(-8000.f, 10000.f, 50.f);
    placement.destYaw = 180.f;

    const optional<MapPosition> dest = RoomTransfer::FindPortalDestination(portal, placement);
    Gamedata::Install(GamedataTables());

    ASSERT_TRUE(dest.has_value());
    EXPECT_EQ(dest->mapId, 2222) << "도착 맵이 지금 맵과 같은지로 룸 안의 순간이동인지 맵 이동인지 가른다";
    EXPECT_FLOAT_EQ(dest->pos.pos().x(), -8000.f);
    EXPECT_FLOAT_EQ(dest->pos.pos().y(), 10000.f);
    EXPECT_FLOAT_EQ(dest->pos.pos().z(), 50.f);
    EXPECT_FLOAT_EQ(dest->pos.yaw(), 180.f);
    EXPECT_EQ(dest->pos.state(), Protocol::MOVE_STATE_IDLE);
}

TEST(RoomTransferTest, PortalToUnknownAreaHasNoDestination)
{
    DesignData::PortalRow portal;
    portal.Id = 11;
    portal.DestAreaId = 99;

    EXPECT_FALSE(RoomTransfer::FindPortalDestination(portal, PortalPlacement{}).has_value());
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

// 마을 리스폰의 목적지는 배치한 리스폰 지점이다. 룸 객체를 보지 않으므로 퇴장한 뒤의 저장에서도 쓸 수 있다.
TEST(RoomTransferTest, TownRespawnComesFromPlacedRespawnPoint)
{
    TestAreaTables::TestArea town = TestAreaTables::MakeArea(10, 1001);
    TestAreaTables::SetRespawnPoint(town, 100.f, 200.f, 30.f);

    GamedataTables tables;
    TestAreaTables::AddTown(tables, town);
    Gamedata::Install(std::move(tables));

    optional<MapPosition> respawn = RoomTransfer::FindTownRespawn();
    Gamedata::Install(GamedataTables());

    ASSERT_TRUE(respawn.has_value());
    EXPECT_EQ(respawn->mapId, 1001) << "사망한 채 끊긴 플레이어는 마을의 맵으로 저장한다";
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
    constexpr int32 TOWN_AREA = 10;
    constexpr int32 FIELD_AREA = 20;
    constexpr int32 OTHER_MAP_AREA = 50;
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

// 마을(MAP_A)에는 같은 맵의 사냥터로 가는 포털과 다른 맵(MAP_B)의 지역으로 가는 포털이 하나씩 있다.
class RoomTransferMapTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        town = TestAreaTables::MakeArea(TOWN_AREA, MAP_A);
        TestAreaTables::AddPortal(town, 11, FIELD_AREA, -1200.f, 1000.f, PORTAL_RADIUS);
        TestAreaTables::AddPortal(town, 12, OTHER_MAP_AREA, 2000.f, 0.f, PORTAL_RADIUS);

        GamedataTables tables;
        TestAreaTables::AddArea(tables, town);
        TestAreaTables::AddArea(tables, TestAreaTables::MakeArea(FIELD_AREA, MAP_A));
        TestAreaTables::AddArea(tables, TestAreaTables::MakeArea(OTHER_MAP_AREA, MAP_B));
        Gamedata::Install(std::move(tables));
    }

    void TearDown() override
    {
        Gamedata::Install(GamedataTables());
    }

    TestAreaTables::TestArea town;
};

TEST_F(RoomTransferMapTest, PortalUseWithinRadiusPasses)
{
    const PortalPlacement& portal = town.portals[0];
    EXPECT_FALSE(RoomTransfer::ValidatePortalUse(portal, MakePlayerPos(-1200.f + 300.f, 1000.f + 400.f)).has_value())
        << "반경 경계(거리 500)는 통과한다";
    EXPECT_FALSE(RoomTransfer::ValidatePortalUse(portal, MakePlayerPos(-1200.f, 1000.f, 5000.f)).has_value())
        << "높이는 보지 않는다. 포털 좌표의 z는 바닥이고 플레이어의 z는 캡슐 중심이다";
}

TEST_F(RoomTransferMapTest, PortalUseOutsideRadiusIsRejected)
{
    const PortalPlacement& portal = town.portals[0];
    EXPECT_TRUE(RoomTransfer::ValidatePortalUse(portal, MakePlayerPos(-1200.f + 301.f, 1000.f + 400.f)).has_value())
        << "조작한 클라이언트는 룸 안 어디서든 포털을 탄다";
}

// 반경은 포털마다 다르다. 지역 하나의 포털이 같은 반경을 쓰지 않는다.
TEST_F(RoomTransferMapTest, PortalUseUsesThatPortalsRadius)
{
    PortalPlacement narrow;
    narrow.id = 13;
    narrow.radius = 100.f;
    EXPECT_FALSE(RoomTransfer::ValidatePortalUse(narrow, MakePlayerPos(100.f, 0.f)).has_value());
    EXPECT_TRUE(RoomTransfer::ValidatePortalUse(narrow, MakePlayerPos(101.f, 0.f)).has_value());
}

//~ 입장 위치

// DB는 맵과 좌표만 안다. 서버는 좌표가 맵의 어느 지역에 들면 그 자리에, 아니면 마을 리스폰 지점에 세운다.
// 마을(TOWN_AREA, 중심 (0, 0), 반경 1000)과 그 서쪽에 붙은 사냥터(FIELD_AREA, 중심 (-3000, 0), 반경 2000)가 x = -1000에서
// 경계를 함께 쓴다. 다른 맵(MAP_B)의 지역은 마을과 같은 좌표 범위에 있다.
class EnterPositionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        TestAreaTables::TestArea town = TestAreaTables::MakeArea(TOWN_AREA, MAP_A, 0.f, 0.f, 1000.f);
        TestAreaTables::SetRespawnPoint(town, 100.f, 200.f, 30.f);

        GamedataTables tables;
        TestAreaTables::AddTown(tables, town);
        TestAreaTables::AddArea(tables, TestAreaTables::MakeArea(FIELD_AREA, MAP_A, -3000.f, 0.f, 2000.f));
        TestAreaTables::AddArea(tables, TestAreaTables::MakeArea(OTHER_MAP_AREA, MAP_B, 0.f, 0.f, 1000.f));
        Gamedata::Install(std::move(tables));
    }

    void TearDown() override
    {
        Gamedata::Install(GamedataTables());
    }

    static void ExpectTownRespawn(const optional<MapPosition>& position)
    {
        ASSERT_TRUE(position.has_value());
        EXPECT_EQ(position->mapId, MAP_A) << "마을로 보내면 맵도 마을의 맵으로 바뀐다";
        EXPECT_FLOAT_EQ(position->pos.pos().x(), 100.f);
        EXPECT_FLOAT_EQ(position->pos.pos().y(), 200.f);
        EXPECT_FLOAT_EQ(position->pos.pos().z(), 30.f);
    }
};

TEST_F(EnterPositionTest, PosInsideAreaKeepsPos)
{
    Protocol::PosInfo pos = MakePlayerPos(-3500.f, 1500.f, 80.f);
    pos.set_yaw(90.f);

    const optional<MapPosition> position = RoomTransfer::ResolveEnterPosition(MAP_A, pos);

    ASSERT_TRUE(position.has_value());
    EXPECT_EQ(position->mapId, MAP_A);
    EXPECT_FLOAT_EQ(position->pos.pos().x(), -3500.f);
    EXPECT_FLOAT_EQ(position->pos.pos().y(), 1500.f);
    EXPECT_FLOAT_EQ(position->pos.pos().z(), 80.f) << "높이는 지역 판정에 쓰지 않지만 그대로 돌려준다";
    EXPECT_FLOAT_EQ(position->pos.yaw(), 90.f);
}

TEST_F(EnterPositionTest, PosOnAreaBoundaryKeepsPos)
{
    const optional<MapPosition> maxEdge = RoomTransfer::ResolveEnterPosition(MAP_A, MakePlayerPos(1000.f, 1000.f));
    ASSERT_TRUE(maxEdge.has_value());
    EXPECT_FLOAT_EQ(maxEdge->pos.pos().x(), 1000.f)
        << "경계 위의 점은 지역 안쪽이다. 경계에서 저장한 플레이어를 마을로 보내면 안 된다";

    const optional<MapPosition> minEdge = RoomTransfer::ResolveEnterPosition(MAP_A, MakePlayerPos(-5000.f, -2000.f));
    ASSERT_TRUE(minEdge.has_value());
    EXPECT_FLOAT_EQ(minEdge->pos.pos().x(), -5000.f);
}

TEST_F(EnterPositionTest, PosOutsideEveryAreaGoesToTownRespawn)
{
    ExpectTownRespawn(RoomTransfer::ResolveEnterPosition(MAP_A, MakePlayerPos(1000.1f, 0.f)));
    ExpectTownRespawn(RoomTransfer::ResolveEnterPosition(MAP_A, MakePlayerPos(-3000.f, 2000.1f)));
}

TEST_F(EnterPositionTest, AreaOfOtherMapIsNotCandidate)
{
    // (-3000, 0)은 MAP_A 사냥터의 범위 안이지만 MAP_B에는 그 좌표를 덮는 지역이 없다.
    ExpectTownRespawn(RoomTransfer::ResolveEnterPosition(MAP_B, MakePlayerPos(-3000.f, 0.f)));

    const optional<MapPosition> otherMap = RoomTransfer::ResolveEnterPosition(MAP_B, MakePlayerPos(0.f, 0.f));
    ASSERT_TRUE(otherMap.has_value());
    EXPECT_EQ(otherMap->mapId, MAP_B) << "범위가 같은 마을이 지역 번호가 더 작아도 맵이 다르면 후보가 아니다";
    EXPECT_FLOAT_EQ(otherMap->pos.pos().x(), 0.f);
}

TEST_F(EnterPositionTest, MapIdNotInMapTableGoesToTownRespawn)
{
    ExpectTownRespawn(RoomTransfer::ResolveEnterPosition(9999, MakePlayerPos(0.f, 0.f)));
}

TEST_F(EnterPositionTest, NoPositionWithoutTownRespawn)
{
    GamedataTables tables;
    TestAreaTables::AddArea(tables, TestAreaTables::MakeArea(FIELD_AREA, MAP_A, -3000.f, 0.f, 2000.f));
    TestAreaTables::AddTown(tables, TestAreaTables::MakeArea(TOWN_AREA, MAP_A, 0.f, 0.f, 1000.f));
    Gamedata::Install(std::move(tables));

    EXPECT_TRUE(RoomTransfer::ResolveEnterPosition(MAP_A, MakePlayerPos(-3000.f, 0.f)).has_value())
        << "지역 안의 좌표는 마을 리스폰 지점이 없어도 그 자리에 세운다";
    EXPECT_FALSE(RoomTransfer::ResolveEnterPosition(MAP_A, MakePlayerPos(50000.f, 0.f)).has_value());
}

//~ 채널

TEST(ChannelSelectionTest, LeastPopulatedChannelWins)
{
    EXPECT_EQ(RoomTransfer::SelectLeastPopulatedChannel({ 3, 1, 2 }), 2);
}

TEST(ChannelSelectionTest, TieGoesToLowerChannel)
{
    EXPECT_EQ(RoomTransfer::SelectLeastPopulatedChannel({ 2, 1, 1 }), 2) << "인원이 같으면 1채널 쪽부터 채운다";
    EXPECT_EQ(RoomTransfer::SelectLeastPopulatedChannel({ 0, 0 }), 1);
}

TEST(ChannelSelectionTest, NoChannelSelectsNothing)
{
    EXPECT_EQ(RoomTransfer::SelectLeastPopulatedChannel({}), 0);
}

namespace
{
    constexpr int32 CHANNEL_COUNT = 3;
}

TEST(ChannelChangeTest, LivingPlayerInRoomMovesToOtherChannel)
{
    EXPECT_FALSE(RoomTransfer::ValidateChannelChange(false, false, 1, 3, CHANNEL_COUNT).has_value());
}

TEST(ChannelChangeTest, DeadPlayerIsRejected)
{
    EXPECT_EQ(RoomTransfer::ValidateChannelChange(true, false, 1, 2, CHANNEL_COUNT), Protocol::ENTRY_RESULT_DEAD)
        << "사망한 채 채널을 바꾸면 리스폰을 거치지 않고 사망한 룸을 빠져나간다";
}

TEST(ChannelChangeTest, PlayerLoadingMapIsRejected)
{
    EXPECT_EQ(RoomTransfer::ValidateChannelChange(false, true, 1, 2, CHANNEL_COUNT), Protocol::ENTRY_RESULT_MAP_LOADING)
        << "맵 로드를 기다리는 플레이어는 룸에 없어서 옮길 수 없다";
}

TEST(ChannelChangeTest, SameChannelIsRejected)
{
    EXPECT_EQ(RoomTransfer::ValidateChannelChange(false, false, 2, 2, CHANNEL_COUNT), Protocol::ENTRY_RESULT_SAME_CHANNEL);
}

TEST(ChannelChangeTest, ChannelOutsideMapIsRejected)
{
    // 클라이언트가 보낸 값이라 0이나 채널 수보다 큰 번호도 온다.
    for (int32 channel : { 0, -1, CHANNEL_COUNT + 1 })
    {
        SCOPED_TRACE(channel);
        EXPECT_EQ(RoomTransfer::ValidateChannelChange(false, false, 1, channel, CHANNEL_COUNT), Protocol::ENTRY_RESULT_INVALID_CHANNEL);
    }
}
