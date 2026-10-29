#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"
#include "TestLevelTables.h"
#include "TestNavLevel.h"
#include "RoomManagerTestAccess.h"

/*--------------------------------------------------------------
    룸의 지형 테스트

    룸은 맵의 navmesh를 함께 읽고 질의 객체만 따로 둔다. 이동을 판정할 때 지형을 묻고, 서버가 정한 위치(입장, 순간이동)는
    바닥 위 캡슐 중심 높이로 올린다. 클라이언트는 받은 높이로 캡슐 중심을 맞추므로, 바닥 높이를 그대로 보내면 캐릭터가 묻힌다.
    판정 기준은 MoveValidationTests가 덮는다. 여기서는 룸이 맵의 navmesh로 그 판정을 하는지만 본다.

    픽스처 결합도: 테스트 레벨(TestNavLevel.h)의 navmesh를 NavMeshes에 설치하고 CreateTestRoom으로 룸을 만든다. Start()는 부르지 않는다.
    플레이어는 세션 없이 만들어 EnterPlayer로 넣는다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 ROOM_ID = 10;
    constexpr int32 MAP_ID = 1;

    Protocol::PosInfo MakePos(float x, float y, float z)
    {
        Protocol::PosInfo pos;
        pos.mutable_pos()->set_x(x);
        pos.mutable_pos()->set_y(y);
        pos.mutable_pos()->set_z(z);
        return pos;
    }
}

class RoomTerrainTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GamedataTables tables;
        TestLevelTables::AddWarriorLevel(tables, 1);
        Gamedata::Install(std::move(tables));

        room = CreateTestRoom(ROOM_ID, MAP_ID, 5000.f);
        ASSERT_NE(room, nullptr);
    }

    void TearDown() override
    {
        room.reset();
        Gamedata::Install(GamedataTables());
    }

    /** 바닥 높이(0)의 위치로 입장시킨다. 배치한 지점(포털 도착, 리스폰)이 그렇다. */
    PlayerRef EnterAt(float x, float y)
    {
        PlayerProgress progress;
        progress.classId = Protocol::CHARACTER_CLASS_WARRIOR;
        progress.level = 1;
        PlayerSpawnParams params;
        params.progress = &progress;
        PlayerRef player = EntityFactory::Create<Player>(params);

        RoomEnterData enterData{};
        enterData.nextRoomId = ROOM_ID;
        enterData.enterPos = MakePos(x, y, 0.f);
        EXPECT_TRUE(room->EnterPlayer(player, enterData));
        return player;
    }

    ScopedNavMeshes navMeshes{ { { MAP_ID, TestNavLevel::Load() } } };
    RoomRef room;
};

TEST_F(RoomTerrainTest, EnterPositionIsLiftedOntoFloor)
{
    PlayerRef player = EnterAt(-500.f, -500.f);

    EXPECT_NEAR(player->GetPosInfo().pos().z(), TestNavLevel::STANDING_Z, 15.f) << "바닥(0)에서 캡슐 반높이만큼 올린다";
}

TEST_F(RoomTerrainTest, MoveThroughWallIsIgnored)
{
    PlayerRef player = EnterAt(-95.f, -500.f);
    const Protocol::PosInfo start = player->GetPosInfo();
    // 1초 전에 받아들인 위치로 둔다. 속도로는 벽 너머까지 갈 수 있다.
    player->ApplyMove(start, ::GetTickCount64() - 1000);

    Protocol::C_MOVE movePkt;
    *movePkt.mutable_pos_info() = MakePos(95.f, -500.f, start.pos().z());
    room->C_HandleMove(movePkt, player);

    EXPECT_FLOAT_EQ(player->GetPosInfo().pos().x(), -95.f) << "벽(x -50 ~ 50)을 통과한 이동은 버린다";
}

TEST_F(RoomTerrainTest, MoveOnFloorIsApplied)
{
    PlayerRef player = EnterAt(-500.f, -500.f);
    const Protocol::PosInfo start = player->GetPosInfo();
    player->ApplyMove(start, ::GetTickCount64() - 1000);

    Protocol::C_MOVE movePkt;
    *movePkt.mutable_pos_info() = MakePos(-500.f, -100.f, start.pos().z());
    room->C_HandleMove(movePkt, player);

    EXPECT_FLOAT_EQ(player->GetPosInfo().pos().y(), -100.f);
}
