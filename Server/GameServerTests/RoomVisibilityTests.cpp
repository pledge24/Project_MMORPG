#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/Monster.h"
#include "Game/Entities/PlayerProgress.h"
#include "Network/ChatRequests.h"
#include "RecordingSession.h"
#include "RoomManagerTestAccess.h"
#include "TestLevelTables.h"
#include "TestAreaTables.h"
#include "TestCreatureCombat.h"

/*--------------------------------------------------------------
    시야(AOI) 테스트

    룸이 맵 하나 전체를 맡으면서 룸 브로드캐스트를 셀 단위로 거른다. 플레이어는 자기 셀과 이웃 8칸의 엔티티만 보고,
    그 엔티티의 스폰, 디스폰, 이동, 공격, 피격, 사망만 받는다. 셀을 넘으면 새로 보이게 된 쪽과 안 보이게 된 쪽에
    스폰과 디스폰이 간다. 채팅은 시야와 무관하게 룸 전체로 간다.

    픽스처 결합도: 경계 (-10000, -10000)~(10000, 10000), 셀 1000cm인 시작하지 않은 룸 하나. 셀의 x 번호는
    (x + 10000) / 1000이라 x = 1000이 셀 경계다. 플레이어는 RecordingSession에 묶어 EnterPlayer로 넣는다.
    입장 직후에는 이동 허용 거리가 50cm라서 셀 경계 근처에서 20cm씩 움직인다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 ROOM_ID = 1;
    constexpr float HALF_EXTENT = 10000.f;
    constexpr float CELL_SIZE = 1000.f;
    constexpr int32 PLAYER_MAX_HP = 100;
    constexpr int32 MONSTER_TEMPLATE_ID = 5000;

    // 셀 경계 x = 1000의 바로 왼쪽과 오른쪽. 이 둘은 이웃한 셀이다.
    constexpr float LEFT_OF_EDGE = 990.f;
    constexpr float RIGHT_OF_EDGE = 1010.f;
    // LEFT_OF_EDGE의 셀에서 두 칸, RIGHT_OF_EDGE의 셀에서 한 칸 떨어진 셀.
    constexpr float TWO_CELLS_RIGHT = 2500.f;
    // 어느 셀에서도 멀리 떨어진 곳.
    constexpr float FAR_AWAY = 8000.f;

    Protocol::PosInfo MakePos(float x, float y)
    {
        Protocol::PosInfo pos;
        pos.mutable_pos()->set_x(x);
        pos.mutable_pos()->set_y(y);
        pos.set_state(Protocol::MOVE_STATE_IDLE);
        return pos;
    }

    Protocol::C_MOVE MakeMove(float x, float y)
    {
        Protocol::C_MOVE pkt;
        *pkt.mutable_pos_info() = MakePos(x, y);
        return pkt;
    }

    vector<int64> SpawnedIds(const shared_ptr<RecordingSession>& session)
    {
        vector<int64> ids;
        for (const Protocol::S_SPAWN& spawnPkt : session->SentPackets<Protocol::S_SPAWN>(PKT_S_SPAWN))
            for (const Protocol::EntityInfo& entity : spawnPkt.entities())
                ids.push_back(entity.entity_id());
        return ids;
    }

    vector<int64> DespawnedIds(const shared_ptr<RecordingSession>& session)
    {
        vector<int64> ids;
        for (const Protocol::S_DESPAWN& despawnPkt : session->SentPackets<Protocol::S_DESPAWN>(PKT_S_DESPAWN))
            for (int64 entityId : despawnPkt.entity_ids())
                ids.push_back(entityId);
        return ids;
    }

    vector<int64> MovedIds(const shared_ptr<RecordingSession>& session)
    {
        vector<int64> ids;
        for (const Protocol::S_MOVE& movePkt : session->SentPackets<Protocol::S_MOVE>(PKT_S_MOVE))
            for (const Protocol::EntityMove& move : movePkt.moves())
                ids.push_back(move.entity_id());
        return ids;
    }
}

class RoomVisibilityTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        DesignData::MonsterRow monsterRow;
        monsterRow.Id = MONSTER_TEMPLATE_ID;
        monsterRow.MaxHp = 100;
        monsterRow.AttackInterval = 2.5f;
        monsterRow.TryAttackRange = 150.f;
        monsterRow.DetectionRange = 100.f;
        monsterRow.ChasingMaxRange = 200.f;
        monsterRow.MovementSpeed = 300.f;

        GamedataTables tables;
        tables.monsters[MONSTER_TEMPLATE_ID] = monsterRow;
        TestCreatureCombat::AddMonster(tables, MONSTER_TEMPLATE_ID);
        TestLevelTables::AddWarriorLevel(tables, 1).MaxHp = PLAYER_MAX_HP;
        Gamedata::Install(std::move(tables));

        room = CreateTestRoom(ROOM_ID, 0, HALF_EXTENT, CELL_SIZE);
    }

    void TearDown() override
    {
        room.reset();
        sessions.clear();
        Gamedata::Install(GamedataTables());
    }

    /** (x, y)에 들어간 1레벨 전사. clearSent면 입장까지 받은 패킷을 비운다. */
    PlayerRef EnterPlayerAt(float x, float y, shared_ptr<RecordingSession>& outSession, bool clearSent = true)
    {
        outSession = make_shared<RecordingSession>();
        sessions.push_back(outSession);

        PlayerProgress progress;
        progress.classId = Protocol::CHARACTER_CLASS_WARRIOR;
        progress.level = 1;
        progress.hp = PLAYER_MAX_HP;

        PlayerSpawnParams params;
        params.session = outSession;
        params.progress = &progress;
        PlayerRef player = EntityFactory::Create<Player>(params);

        RoomEnterData enterData{};
        enterData.nextRoomId = ROOM_ID;
        enterData.enterPos = MakePos(x, y);
        EXPECT_TRUE(room->EnterPlayer(player, enterData));
        if (clearSent)
            outSession->ClearSent();
        return player;
    }

    MonsterRef SpawnMonsterAt(float x, float y)
    {
        MonsterSpawnParams params;
        params.templateId = MONSTER_TEMPLATE_ID;
        params.spawnPos = MakePos(x, y);
        params.wanderBounds = PlaneBounds::FromCenter(x, y, 0.f, 0.f);
        return room->SpawnEntity<Monster>(params);
    }

    RoomRef room;
    vector<shared_ptr<RecordingSession>> sessions;
};

TEST_F(RoomVisibilityTest, EnterSpawnsOnlyNearbyEntities)
{
    shared_ptr<RecordingSession> nearSession;
    PlayerRef nearPlayer = EnterPlayerAt(RIGHT_OF_EDGE, 0.f, nearSession);
    shared_ptr<RecordingSession> farSession;
    PlayerRef farPlayer = EnterPlayerAt(FAR_AWAY, 0.f, farSession);

    shared_ptr<RecordingSession> session;
    PlayerRef newcomer = EnterPlayerAt(LEFT_OF_EDGE, 0.f, session, false);

    EXPECT_EQ(SpawnedIds(session), vector<int64>({ nearPlayer->GetEntityId() })) << "주변 셀의 엔티티만 싣는다";
    EXPECT_EQ(SpawnedIds(nearSession), vector<int64>({ newcomer->GetEntityId() }));
    EXPECT_TRUE(SpawnedIds(farSession).empty()) << "멀리 있는 플레이어는 새로 온 플레이어를 모른다";
}

TEST_F(RoomVisibilityTest, FarPlayerDoesNotReceiveMove)
{
    shared_ptr<RecordingSession> session;
    PlayerRef mover = EnterPlayerAt(LEFT_OF_EDGE, 0.f, session);
    shared_ptr<RecordingSession> nearSession;
    EnterPlayerAt(LEFT_OF_EDGE, 500.f, nearSession);
    shared_ptr<RecordingSession> farSession;
    EnterPlayerAt(FAR_AWAY, 0.f, farSession);

    room->C_HandleMove(MakeMove(LEFT_OF_EDGE - 20.f, 0.f), mover);

    EXPECT_EQ(MovedIds(nearSession), vector<int64>({ mover->GetEntityId() }));
    EXPECT_TRUE(MovedIds(farSession).empty()) << "보이지 않는 플레이어의 이동은 대역폭만 쓴다";
}

TEST_F(RoomVisibilityTest, CrossingIntoViewSpawnsBothWays)
{
    shared_ptr<RecordingSession> session;
    PlayerRef mover = EnterPlayerAt(LEFT_OF_EDGE, 0.f, session);
    shared_ptr<RecordingSession> otherSession;
    PlayerRef other = EnterPlayerAt(TWO_CELLS_RIGHT, 0.f, otherSession);
    ASSERT_TRUE(SpawnedIds(otherSession).empty()) << "준비: 처음에는 서로 보이지 않는다";

    room->C_HandleMove(MakeMove(RIGHT_OF_EDGE, 0.f), mover);

    EXPECT_EQ(SpawnedIds(otherSession), vector<int64>({ mover->GetEntityId() })) << "새로 보이게 된 쪽은 스폰을 받는다";
    EXPECT_EQ(SpawnedIds(session), vector<int64>({ other->GetEntityId() })) << "옮긴 본인도 새로 보이는 엔티티의 스폰을 받는다";
    EXPECT_EQ(MovedIds(otherSession), vector<int64>({ mover->GetEntityId() })) << "스폰 다음에 이동이 간다";
}

TEST_F(RoomVisibilityTest, CrossingOutOfViewDespawnsBothWays)
{
    shared_ptr<RecordingSession> session;
    PlayerRef mover = EnterPlayerAt(RIGHT_OF_EDGE, 0.f, session);
    shared_ptr<RecordingSession> otherSession;
    PlayerRef other = EnterPlayerAt(TWO_CELLS_RIGHT, 0.f, otherSession);

    room->C_HandleMove(MakeMove(LEFT_OF_EDGE, 0.f), mover);

    EXPECT_EQ(DespawnedIds(otherSession), vector<int64>({ mover->GetEntityId() }));
    EXPECT_EQ(DespawnedIds(session), vector<int64>({ other->GetEntityId() }));
    EXPECT_TRUE(MovedIds(otherSession).empty()) << "안 보이게 된 쪽에는 이동이 가지 않는다";
}

TEST_F(RoomVisibilityTest, MovingWithinViewSendsNoSpawnOrDespawn)
{
    shared_ptr<RecordingSession> session;
    PlayerRef mover = EnterPlayerAt(LEFT_OF_EDGE, 0.f, session);
    shared_ptr<RecordingSession> otherSession;
    EnterPlayerAt(RIGHT_OF_EDGE + 500.f, 0.f, otherSession);

    room->C_HandleMove(MakeMove(RIGHT_OF_EDGE, 0.f), mover);

    EXPECT_TRUE(SpawnedIds(otherSession).empty()) << "셀을 넘어도 계속 보이던 쪽은 스폰을 다시 받지 않는다";
    EXPECT_TRUE(DespawnedIds(otherSession).empty());
    EXPECT_EQ(MovedIds(otherSession).size(), 1u);
}

TEST_F(RoomVisibilityTest, AttackGoesOnlyToViewers)
{
    shared_ptr<RecordingSession> session;
    PlayerRef attacker = EnterPlayerAt(0.f, 0.f, session);
    shared_ptr<RecordingSession> nearSession;
    EnterPlayerAt(500.f, 0.f, nearSession);
    shared_ptr<RecordingSession> farSession;
    EnterPlayerAt(FAR_AWAY, 0.f, farSession);

    Protocol::C_NORMAL_ATTACK attackPkt;
    attackPkt.set_combo(1);
    room->C_HandleNormalAttack(attackPkt, attacker);

    EXPECT_EQ(nearSession->SentPackets<Protocol::S_ATTACK>(PKT_S_ATTACK).size(), 1u);
    EXPECT_TRUE(farSession->SentPackets<Protocol::S_ATTACK>(PKT_S_ATTACK).empty());
}

TEST_F(RoomVisibilityTest, LeavingDespawnsOnlyForViewers)
{
    shared_ptr<RecordingSession> session;
    PlayerRef leaver = EnterPlayerAt(0.f, 0.f, session);
    shared_ptr<RecordingSession> nearSession;
    EnterPlayerAt(500.f, 0.f, nearSession);
    shared_ptr<RecordingSession> farSession;
    EnterPlayerAt(FAR_AWAY, 0.f, farSession);

    ASSERT_TRUE(room->LeavePlayer(leaver, false));

    EXPECT_EQ(DespawnedIds(nearSession), vector<int64>({ leaver->GetEntityId() }));
    EXPECT_TRUE(DespawnedIds(farSession).empty()) << "스폰을 받은 적 없는 쪽에 디스폰을 보내지 않는다";
}

TEST_F(RoomVisibilityTest, MonsterMovesGoOnlyToViewers)
{
    MonsterRef monster = SpawnMonsterAt(0.f, 0.f);
    ASSERT_NE(monster, nullptr);
    shared_ptr<RecordingSession> nearSession;
    EnterPlayerAt(1500.f, 0.f, nearSession);
    shared_ptr<RecordingSession> farSession;
    EnterPlayerAt(FAR_AWAY, 0.f, farSession);

    // 몬스터 위치는 0.2초마다 묶어서 보낸다.
    room->Tick(0.2f);

    EXPECT_EQ(MovedIds(nearSession), vector<int64>({ monster->GetEntityId() }));
    EXPECT_TRUE(MovedIds(farSession).empty());
}

TEST_F(RoomVisibilityTest, ChatReachesWholeRoom)
{
    shared_ptr<RecordingSession> session;
    PlayerRef speaker = EnterPlayerAt(0.f, 0.f, session);
    shared_ptr<RecordingSession> farSession;
    EnterPlayerAt(FAR_AWAY, 0.f, farSession);

    Protocol::C_CHAT chatPkt;
    chatPkt.set_message("안녕");
    ChatRequests::HandleChat(*room, speaker, chatPkt);

    EXPECT_EQ(farSession->SentPackets<Protocol::S_CHAT_MESSAGE>(PKT_S_CHAT_MESSAGE).size(), 1u)
        << "채팅은 시야와 무관하게 룸 전체로 간다";
}
