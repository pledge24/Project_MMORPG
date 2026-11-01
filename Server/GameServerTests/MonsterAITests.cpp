#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"
#include "Game/Entities/Monster.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"
#include "Game/AI/MonsterAIComponent.h"
#include "Game/Action/ActionComponent.h"
#include "TestLevelTables.h"
#include "TestAreaTables.h"
#include "TestNavLevel.h"
#include "RoomManagerTestAccess.h"
#include "TestCreatureCombat.h"

/*--------------------------------------------------------------
    몬스터 AI 테스트

    몬스터는 룸 틱(Room::Tick)으로만 움직인다. 상태의 갱신(전이 판정과 목적지 고르기)은 누적 시간이 0.2초를 채울 때마다 돌고,
    경로 추종과 예약한 피격은 틱마다 돈다. 테스트는 타이머를 기다리지 않고 Room::Tick에 시간을 넘긴다.

    상태는 대기, 배회, 추적, 공격, 귀환이다. 배회는 스폰 반경 안에서 하고, 추적 한계는 스폰 지점에서 잰다.
    귀환하는 동안은 대상을 잡지 않고 체력을 회복한다.

    픽스처 결합도: CreateTestRoom으로 룸을 만들고 Start()는 부르지 않는다(몬스터 자동 스폰과 틱 예약을 피한다).
    몬스터는 Room::SpawnEntity로, 플레이어는 세션 없이 불러온 진행으로 만들어 EnterPlayer로 넣는다.
    몬스터 템플릿과 그 크리처 판정 데이터, 1레벨 전사 레벨 표를 Gamedata::Install로 주입한다.
    벽을 돌아가는 추적만 테스트 레벨(TestNavLevel.h)의 navmesh를 설치한다. 나머지 룸은 navmesh가 없어 목적지로 곧장 간다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 ROOM_ID = 20;
    constexpr int32 MONSTER_TEMPLATE_ID = 5000;
    constexpr int32 PLAYER_MAX_HP = 100;
    constexpr int32 MONSTER_MAX_HP = 100;
    constexpr int32 PLAYER_ATTACK = 10;
    constexpr float ATTACK_RANGE = 150.f;
    constexpr float DETECTION_RANGE = 900.f;
    constexpr float CHASING_MAX_RANGE = 1200.f;
    constexpr float SPEED = 300.f;
    constexpr float SPAWN_RADIUS = 500.f;
    // 몬스터 공격 몽타주의 타격 시점. 상태 갱신 주기(0.2초)와 다르게 둔다. 이진수로 정확한 값이라 남은 시간이 오차로 0을 넘지 않는다.
    constexpr float HIT_TIME = 0.375f;
    // 일반 공격의 공격 범위는 앞을 가운데에 둔 부채꼴이다. 반지름은 공격 시작 거리와 같다.
    constexpr float ATTACK_ANGLE = 90.f;

    // 상태 갱신 주기(0.2초)를 한 번에 채우는 시간.
    constexpr float STATE_UPDATE_TIME = 0.2f;
    constexpr float ROOM_TICK_TIME = 0.05f;
    // 대기를 유지하는 시간. MonsterAIComponent의 IDLE_TIME과 같다.
    constexpr float IDLE_TIME = 5.f;

    PlayerProgress MakeAliveProgress()
    {
        PlayerProgress progress;
        progress.classId = Protocol::CHARACTER_CLASS_WARRIOR;
        progress.level = 1;
        progress.hp = PLAYER_MAX_HP;
        return progress;
    }

    Protocol::PosInfo MakePos(float x, float y, float z = 0.f)
    {
        Protocol::PosInfo pos;
        pos.mutable_pos()->set_x(x);
        pos.mutable_pos()->set_y(y);
        pos.mutable_pos()->set_z(z);
        return pos;
    }

    float PlaneDistance(const Protocol::PosInfo& pos, float x, float y)
    {
        const float dx = pos.pos().x() - x;
        const float dy = pos.pos().y() - y;
        return sqrtf(dx * dx + dy * dy);
    }

    DesignData::MonsterRow MakeMonsterRow()
    {
        DesignData::MonsterRow monsterRow;
        monsterRow.Id = MONSTER_TEMPLATE_ID;
        monsterRow.MaxHp = MONSTER_MAX_HP;
        monsterRow.BaseAttack = 10;
        monsterRow.AttackInterval = 2.5f;
        monsterRow.TryAttackRange = ATTACK_RANGE;
        monsterRow.DetectionRange = DETECTION_RANGE;
        monsterRow.ChasingMaxRange = CHASING_MAX_RANGE;
        monsterRow.MovementSpeed = SPEED;
        return monsterRow;
    }
}

class MonsterAITest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GamedataTables tables;
        tables.monsters[MONSTER_TEMPLATE_ID] = MakeMonsterRow();
        TestCreatureCombat::AddMonster(tables, MONSTER_TEMPLATE_ID, HIT_TIME, TestCreatureCombat::MakeSectorHit(ATTACK_RANGE, ATTACK_ANGLE));
        TestCreatureCombat::AddClass(tables, Protocol::CHARACTER_CLASS_WARRIOR);
        DesignData::ClassLevelStatRow& warriorLevel = TestLevelTables::AddWarriorLevel(tables, 1);
        warriorLevel.MaxHp = PLAYER_MAX_HP;
        warriorLevel.PhysicalAttack = PLAYER_ATTACK;
        // 플레이어의 타격 한 번이 몬스터 HP의 절반을 깎는다.
        TestCreatureCombat::AddStrikeAction(tables, static_cast<float>(MONSTER_MAX_HP / 2) / PLAYER_ATTACK);
        Gamedata::Install(std::move(tables));

        room = CreateTestRoom(ROOM_ID, 0, 5000.f);
        ASSERT_NE(room, nullptr);
    }

    void TearDown() override
    {
        player.reset();
        monster.reset();
        room.reset();
        Gamedata::Install(GamedataTables());
    }

    /** (x, y)에 몬스터를 둔다. 스폰 마커는 (spawnX, spawnY)에 반경 radius다. */
    void SpawnMonster(float x = 0.f, float y = 0.f, float spawnX = 0.f, float spawnY = 0.f, float radius = SPAWN_RADIUS, float z = 0.f)
    {
        MonsterSpawnParams monsterParams;
        monsterParams.templateId = MONSTER_TEMPLATE_ID;
        monsterParams.spawnPos = MakePos(x, y, z);
        monsterParams.spawnPoint = SpawnPointPlacement{ 1, vector3D(spawnX, spawnY, 0.f), radius };
        monster = room->SpawnEntity<Monster>(monsterParams);
        ASSERT_NE(monster, nullptr);
    }

    /** (x, y)에 살아 있는 플레이어를 넣는다. */
    void EnterPlayerAt(float x, float y, float z = 0.f)
    {
        const PlayerProgress progress = MakeAliveProgress();
        PlayerSpawnParams params;
        params.progress = &progress;
        player = EntityFactory::Create<Player>(params);
        ASSERT_NE(player, nullptr);

        RoomEnterData enterData{};
        enterData.nextRoomId = ROOM_ID;
        enterData.enterPos = MakePos(x, y, z);
        ASSERT_TRUE(room->EnterPlayer(player, enterData));
    }

    /** 룸 틱 간격으로 seconds만큼 돌린다. until이 참이 되면 멈추고 true. */
    bool TickFor(float seconds, const function<bool()>& until = nullptr)
    {
        for (float elapsed = 0.f; elapsed < seconds; elapsed += ROOM_TICK_TIME)
        {
            room->Tick(ROOM_TICK_TIME);
            if (until && until())
                return true;
        }
        return false;
    }

    MonsterStateType State() const { return monster->GetAI().GetState(); }

    RoomRef room;
    MonsterRef monster;
    PlayerRef player;
};

TEST_F(MonsterAITest, ChasesPlayerInDetectionRangeAfterStateUpdate)
{
    SpawnMonster();
    EnterPlayerAt(DETECTION_RANGE - 100.f, 0.f);

    room->Tick(ROOM_TICK_TIME);
    EXPECT_EQ(State(), MonsterStateType::Idle) << "상태 갱신은 0.2초가 쌓여야 돈다";

    room->Tick(STATE_UPDATE_TIME);
    EXPECT_EQ(State(), MonsterStateType::Chasing);
    EXPECT_EQ(monster->GetAI().GetTarget(), player);
}

TEST_F(MonsterAITest, IdleWandersInsideSpawnRadiusAfterIdleTime)
{
    // 스폰 마커에서 떨어진 곳에 두어, 배회 목적지를 마커 기준으로 고르는지 본다.
    SpawnMonster(0.f, 0.f, 300.f, 0.f, 200.f);

    room->Tick(IDLE_TIME - 0.5f);
    ASSERT_EQ(State(), MonsterStateType::Idle) << "대기 시간 전에는 대기한다";

    ASSERT_TRUE(TickFor(1.f, [&] { return State() == MonsterStateType::Wandering; })) << "대기 시간이 지나면 배회한다";
    TickFor(5.f, [&] { return State() != MonsterStateType::Wandering; });

    EXPECT_LE(PlaneDistance(monster->GetPosInfo(), 300.f, 0.f), 200.f + 1.f) << "배회는 스폰 반경 안에서 한다";
}

TEST_F(MonsterAITest, WanderingChasesPlayerEnteringDetectionRange)
{
    SpawnMonster();
    ASSERT_TRUE(TickFor(IDLE_TIME + 1.f, [&] { return State() == MonsterStateType::Wandering; }));

    EnterPlayerAt(DETECTION_RANGE - 100.f, 0.f);
    room->Tick(STATE_UPDATE_TIME);

    EXPECT_EQ(State(), MonsterStateType::Chasing);
}

TEST_F(MonsterAITest, ChasingAttacksWhenTargetComesInRange)
{
    SpawnMonster();
    EnterPlayerAt(600.f, 0.f);
    room->Tick(STATE_UPDATE_TIME);
    ASSERT_EQ(State(), MonsterStateType::Chasing);

    EXPECT_TRUE(TickFor(5.f, [&] { return State() == MonsterStateType::Attacking; })) << "쫓아가 공격 사거리에 들면 공격한다";
    EXPECT_LE(PlaneDistance(monster->GetPosInfo(), 600.f, 0.f), ATTACK_RANGE);
}

// 추적 한계를 대상과의 거리로 쟀다. 플레이어가 달아나며 끌고 가면 몬스터가 스폰한 곳에서 한없이 멀어졌다.
TEST_F(MonsterAITest, ChasingBeyondLimitFromSpawnPointReturns)
{
    // 몬스터는 스폰 지점에서 추적 한계보다 먼 곳에 있다. 대상과는 가깝다.
    SpawnMonster(0.f, 0.f, -(CHASING_MAX_RANGE + 300.f), 0.f, 100.f);
    EnterPlayerAt(400.f, 0.f);
    room->Tick(STATE_UPDATE_TIME);
    ASSERT_EQ(State(), MonsterStateType::Chasing);

    room->Tick(STATE_UPDATE_TIME);

    EXPECT_EQ(State(), MonsterStateType::Returning);
    EXPECT_EQ(monster->GetAI().GetTarget(), nullptr) << "귀환하면 대상을 놓는다";
}

TEST_F(MonsterAITest, ReturningIgnoresPlayersAndRecoversHp)
{
    SpawnMonster(0.f, 0.f, -(CHASING_MAX_RANGE + 300.f), 0.f, 100.f);
    EnterPlayerAt(400.f, 0.f);
    room->Tick(STATE_UPDATE_TIME);
    room->HandleHit(player, TestCreatureCombat::STRIKE_ACTION_ID, 0, TestCreatureCombat::StrikePoseAt(monster->GetPosInfo()));
    ASSERT_EQ(monster->GetStatValue(Protocol::STAT_TYPE_HP), MONSTER_MAX_HP / 2);
    room->Tick(STATE_UPDATE_TIME);
    ASSERT_EQ(State(), MonsterStateType::Returning);
    const int64 damagedHp = monster->GetStatValue(Protocol::STAT_TYPE_HP);

    // 귀환하는 몬스터를 플레이어가 따라가 감지 범위 안에 둔다.
    room->Tick(STATE_UPDATE_TIME);
    player->ApplyMove(MakePos(monster->GetPosInfo().pos().x() + 200.f, 0.f), ::GetTickCount64());
    room->Tick(STATE_UPDATE_TIME);

    EXPECT_EQ(State(), MonsterStateType::Returning) << "귀환하는 동안은 대상을 잡지 않는다";
    EXPECT_GT(monster->GetStatValue(Protocol::STAT_TYPE_HP), damagedHp) << "귀환하는 동안 체력을 회복한다";
}

TEST_F(MonsterAITest, ReturnCompletedGoesIdleInsideSpawnRadius)
{
    constexpr float SPAWN_X = -(CHASING_MAX_RANGE + 300.f);
    SpawnMonster(0.f, 0.f, SPAWN_X, 0.f, 100.f);
    EnterPlayerAt(400.f, 0.f);
    room->Tick(STATE_UPDATE_TIME);
    room->Tick(STATE_UPDATE_TIME);
    ASSERT_EQ(State(), MonsterStateType::Returning);

    // 1500cm를 초속 300cm로 간다.
    EXPECT_TRUE(TickFor(8.f, [&] { return State() != MonsterStateType::Returning; }));

    EXPECT_EQ(State(), MonsterStateType::Idle) << "스폰 반경에 들면 대기한다";
    EXPECT_LE(PlaneDistance(monster->GetPosInfo(), SPAWN_X, 0.f), 100.f + 1.f);
    EXPECT_EQ(monster->GetStatValue(Protocol::STAT_TYPE_HP), MONSTER_MAX_HP);
}

// 대상이 룸에 남았는지는 0.2초마다 도는 상태 갱신만 봤다. 그사이의 틱은 다른 룸으로 떠난 플레이어의 위치를
// 읽었다. 그 위치는 새 룸 큐가 쓰므로, 다른 룸의 오브젝트에 손대지 않는다는 불변식을 어긴다.
TEST_F(MonsterAITest, LosesTargetOnFirstTickAfterTargetLeavesRoom)
{
    SpawnMonster();
    EnterPlayerAt(DETECTION_RANGE - 100.f, 0.f);
    room->Tick(STATE_UPDATE_TIME);
    ASSERT_EQ(monster->GetAI().GetTarget(), player);

    ASSERT_TRUE(room->LeavePlayer(player, true));
    room->Tick(ROOM_TICK_TIME);

    EXPECT_EQ(monster->GetAI().GetTarget(), nullptr) << "다음 상태 갱신까지 떠난 대상의 위치를 읽는다";
    EXPECT_EQ(State(), MonsterStateType::Returning) << "대상을 놓치면 스폰 지점으로 돌아간다";
}

// 일반 공격은 공격을 알린 뒤 액션의 타격 시점이 지나서, 공격 범위 안의 대상을 판정한다.
TEST_F(MonsterAITest, NormalAttackHitsTargetInAttackArea)
{
    SpawnMonster();
    EnterPlayerAt(ATTACK_RANGE - 50.f, 0.f);
    room->Tick(STATE_UPDATE_TIME);
    ASSERT_EQ(State(), MonsterStateType::Attacking);

    // 다음 상태 갱신에서 공격을 시작한다. 피격은 다음 틱 이후로 예약된다.
    room->Tick(2.5f);
    ASSERT_TRUE(monster->GetAction().IsActing());
    ASSERT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), PLAYER_MAX_HP);

    room->Tick(HIT_TIME);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), PLAYER_MAX_HP - 10) << "피해량은 몬스터의 공격력 × 액션의 계수(1)다";
}

// 타격 지연이 상수(0.2초)였다. 애니메이션이 맞는 순간보다 먼저나 나중에 피격되었다.
TEST_F(MonsterAITest, NormalAttackHitsAtExtractedHitTime)
{
    SpawnMonster();
    EnterPlayerAt(ATTACK_RANGE - 50.f, 0.f);
    room->Tick(STATE_UPDATE_TIME);
    ASSERT_EQ(State(), MonsterStateType::Attacking);
    room->Tick(2.5f);

    constexpr float LAST_TICK = 0.125f;
    room->Tick(HIT_TIME - LAST_TICK);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), PLAYER_MAX_HP) << "타격 시점 전에는 맞지 않는다";

    room->Tick(LAST_TICK);
    EXPECT_LT(player->GetStatValue(Protocol::STAT_TYPE_HP), PLAYER_MAX_HP) << "타격 시점에 맞는다";
}

// 공격을 시작한 뒤 타격 시점 전에 공격 범위를 빠져나간 플레이어는 공격을 피한 것이다.
TEST_F(MonsterAITest, NormalAttackMissesTargetThatLeftAttackAreaBeforeHit)
{
    SpawnMonster();
    EnterPlayerAt(ATTACK_RANGE - 20.f, 0.f);
    room->Tick(STATE_UPDATE_TIME);
    ASSERT_EQ(State(), MonsterStateType::Attacking);
    room->Tick(2.5f);

    // 캡슐의 가장자리가 범위 밖으로 나가야 피한다. 반지름만큼 더 물러난다.
    // 이동 판정은 실제 시각으로 거리를 제한하므로 테스트는 판정을 거치지 않고 옮긴다.
    player->ApplyMove(MakePos(ATTACK_RANGE + TestCreatureCombat::PLAYER_CAPSULE_RADIUS + 20.f, 0.f), ::GetTickCount64());
    room->Tick(HIT_TIME);

    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), PLAYER_MAX_HP) << "피한 공격에 맞으면 클라이언트가 보여 주는 범위와 판정이 어긋난다";
}

// 몬스터는 공격을 시작할 때 방향을 고정한다. 대상을 따라 돌면 옆으로 피한 플레이어도 맞는다.
TEST_F(MonsterAITest, NormalAttackKeepsDirectionFixedFromStart)
{
    SpawnMonster();
    EnterPlayerAt(ATTACK_RANGE - 50.f, 0.f);
    room->Tick(STATE_UPDATE_TIME);
    ASSERT_EQ(State(), MonsterStateType::Attacking);
    room->Tick(2.5f);
    ASSERT_TRUE(monster->GetAction().IsActing());
    const float startYaw = monster->GetPosInfo().yaw();

    // 부채꼴의 옆으로 비켜 선다. 공격 시작 거리 안이라 몬스터는 공격 상태에 남는다.
    player->ApplyMove(MakePos(0.f, ATTACK_RANGE - 50.f), ::GetTickCount64());
    room->Tick(HIT_TIME);

    EXPECT_FLOAT_EQ(monster->GetPosInfo().yaw(), startYaw) << "공격하는 동안 대상을 따라 돌지 않는다";
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), PLAYER_MAX_HP) << "판정은 공격을 시작한 방향으로 한다";
}

// 룸에서 빠진 크리처의 예약한 타격은 사라진다. 남아 있으면 다시 들어온 룸에서 지난 공격이 판정된다.
TEST_F(MonsterAITest, AttackerLeavingRoomCancelsPendingHit)
{
    SpawnMonster();
    EnterPlayerAt(ATTACK_RANGE - 50.f, 0.f);
    room->Tick(STATE_UPDATE_TIME);
    room->Tick(2.5f);
    ASSERT_TRUE(monster->GetAction().IsActing());

    room->HandleDie(monster);
    EXPECT_FALSE(monster->GetAction().IsActing()) << "룸에서 빠지면 진행 중인 액션과 예약한 타격을 버린다";

    room->Tick(HIT_TIME);
    EXPECT_EQ(player->GetStatValue(Protocol::STAT_TYPE_HP), PLAYER_MAX_HP);
}

/*--------------------------------------------------------------
    지형 위의 추적

    navmesh가 있는 룸에서 몬스터는 길찾기 결과를 따라 움직인다. 테스트 레벨의 벽(x -50 ~ 50, y -1000 ~ 400)은
    y 400 위가 비어 있어 돌아갈 수 있다.
---------------------------------------------------------------*/

class MonsterTerrainChaseTest : public MonsterAITest
{
protected:
    void SetUp() override
    {
        MonsterAITest::SetUp();
        room = CreateTestRoom(ROOM_ID, MAP_ID, 5000.f);
        ASSERT_NE(room, nullptr);
    }

    static constexpr int32 MAP_ID = 1;
    ScopedNavMeshes navMeshes{ { { MAP_ID, TestNavLevel::Load() } } };
};

// 몬스터가 대상을 향해 곧장 움직여 벽을 뚫고 지나갔다.
TEST_F(MonsterTerrainChaseTest, ChasesTargetBehindWallAroundIt)
{
    SpawnMonster(-300.f, -500.f, -300.f, -500.f, 100.f, TestNavLevel::STANDING_Z);
    EnterPlayerAt(300.f, -500.f);
    room->Tick(STATE_UPDATE_TIME);
    ASSERT_EQ(State(), MonsterStateType::Chasing);

    bool crossedWall = false;
    float maxY = monster->GetPosInfo().pos().y();
    const bool attacked = TickFor(15.f, [&]
        {
            const Protocol::Vector& pos = monster->GetPosInfo().pos();
            maxY = max(maxY, pos.y());
            if (fabsf(pos.x()) < 50.f && pos.y() < 400.f)
                crossedWall = true;
            return State() == MonsterStateType::Attacking;
        });

    EXPECT_FALSE(crossedWall) << "벽(x -50 ~ 50, y 400 아래)을 지나갔다";
    EXPECT_GT(maxY, 400.f) << "벽 끝(y 400)을 돌아간다";
    EXPECT_TRUE(attacked) << "돌아가서 대상에게 닿는다";
}
