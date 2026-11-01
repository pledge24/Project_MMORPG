#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/Monster.h"
#include "Game/Entities/PlayerProgress.h"
#include "Game/Action/ActionComponent.h"
#include "RecordingSession.h"
#include "TestLevelTables.h"
#include "TestAreaTables.h"
#include "TestCreatureCombat.h"
#include "RoomManagerTestAccess.h"

/*--------------------------------------------------------------
    룸의 액션과 이동 요청 테스트

    요청한 플레이어는 응답을 받고, 같은 룸의 다른 플레이어는 통지를 받는다. 통지는 엔티티 번호를
    메시지의 한 곳에만 싣는다.

    서버는 플레이어의 액션을 알리기만 하고 판정하지 않았다. 지금은 액션 요청을 검증해 거절하면 사유를 응답하고,
    받아들인 액션은 타격 시점마다 공격 범위 안의 몬스터를 맞힌다. 타격의 기준 위치는 검증한 시작 위치에서
    루트 모션으로 나간 곳이다.

    픽스처 결합도: CreateTestRoom으로 룸을 만들고 Start()는 부르지 않는다. 두 플레이어를 RecordingSession에 묶어
    EnterPlayer로 넣는다. 1·2레벨 전사 레벨 표, 직업 표, 2단 콤보 액션과 그 판정 데이터, 몬스터 템플릿을
    Gamedata::Install로 주입한다. 몬스터는 감지 범위가 없어 플레이어를 쫓거나 때리지 않는다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 ROOM_ID = 10;
    constexpr int32 PLAYER_MAX_HP = 100;
    constexpr int32 PLAYER_ATTACK = 10;
    constexpr int64 LEVEL_1_EXP = 10;

    constexpr int32 MONSTER_TEMPLATE_ID = 5000;
    constexpr int32 MONSTER_MAX_HP = 100;
    constexpr int64 MONSTER_EXP = LEVEL_1_EXP;
    constexpr int64 MONSTER_GOLD = 7;

    // 2단 콤보. 1단계는 몬스터 HP의 30%를, 2단계는 남은 HP 이상을 깎는다.
    constexpr int32 COMBO_1 = 1001;
    constexpr int32 COMBO_2 = 1002;
    constexpr float COMBO_1_COEFFICIENT = 3.f;
    constexpr float COMBO_2_COEFFICIENT = 10.f;
    constexpr float ACTION_LENGTH = 1.f;
    constexpr float HIT_TIME = 0.5f;
    constexpr ComboWindow COMBO_1_WINDOW{ 0.5f, 0.9f };

    // 1단계는 0.1초마다 앞으로 20cm씩 나가 타격 순간에 100cm 앞에 있다. 공격 범위는 기준 위치 둘레 반지름 60이다.
    constexpr int32 ROOT_MOTION_SAMPLE_RATE = 10;
    constexpr float ROOT_MOTION_STEP = 20.f;
    constexpr float FORWARD_AT_HIT = 100.f;
    constexpr float HIT_RADIUS = 60.f;
    constexpr float HIT_REACH = HIT_RADIUS + TestCreatureCombat::MONSTER_CAPSULE_RADIUS;

    // 이진수로 정확한 값이라 타격 시점과 입력 창의 시작에 오차 없이 닿는다.
    constexpr float ROOM_TICK_TIME = 0.0625f;

    DesignData::ActionRow MakeComboAction(int32 actionId, float coefficient, optional<int32> nextAction)
    {
        DesignData::ActionHitRow hit = TestCreatureCombat::MakeCircleHit(HIT_RADIUS);
        hit.ZMin = -1000.f;
        hit.ZMax = 1000.f;

        DesignData::ActionRow action = TestCreatureCombat::MakeAction(actionId, hit, coefficient);
        action.NextAction = nextAction;
        return action;
    }

    ActionTiming MakeComboTiming(optional<ComboWindow> window, bool withRootMotion)
    {
        ActionTiming timing = TestCreatureCombat::MakeTiming(HIT_TIME);
        timing.length = ACTION_LENGTH;
        timing.comboWindow = window;
        if (withRootMotion)
        {
            for (int32 i = 0; i <= static_cast<int32>(ACTION_LENGTH * ROOT_MOTION_SAMPLE_RATE); ++i)
                timing.rootMotion.push_back(vector3D(ROOT_MOTION_STEP * i, 0.f, 0.f));
        }
        return timing;
    }

    DesignData::MonsterRow MakeMonsterRow()
    {
        DesignData::MonsterRow monsterRow;
        monsterRow.Id = MONSTER_TEMPLATE_ID;
        monsterRow.MaxHp = MONSTER_MAX_HP;
        monsterRow.MinExp = MONSTER_EXP;
        monsterRow.MaxExp = MONSTER_EXP;
        monsterRow.MinGold = MONSTER_GOLD;
        monsterRow.MaxGold = MONSTER_GOLD;
        monsterRow.AttackInterval = 2.5f;
        monsterRow.TryAttackRange = 150.f;
        monsterRow.ChasingMaxRange = 1200.f;
        monsterRow.MovementSpeed = 300.f;
        return monsterRow;
    }

    Protocol::PosInfo MakePos(float x, float y)
    {
        Protocol::PosInfo pos;
        pos.mutable_pos()->set_x(x);
        pos.mutable_pos()->set_y(y);
        return pos;
    }
}

class CombatRequestTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        GamedataTables tables;
        DesignData::ClassLevelStatRow& level1 = TestLevelTables::AddWarriorLevel(tables, 1, LEVEL_1_EXP);
        level1.MaxHp = PLAYER_MAX_HP;
        level1.PhysicalAttack = PLAYER_ATTACK;
        DesignData::ClassLevelStatRow& level2 = TestLevelTables::AddWarriorLevel(tables, 2);
        level2.MaxHp = PLAYER_MAX_HP * 2;
        level2.PhysicalAttack = PLAYER_ATTACK * 2;

        tables.classes[Protocol::CHARACTER_CLASS_WARRIOR] = DesignData::ClassRow{ Protocol::CHARACTER_CLASS_WARRIOR, COMBO_1 };
        TestCreatureCombat::AddClass(tables, Protocol::CHARACTER_CLASS_WARRIOR);
        tables.actions[COMBO_1] = MakeComboAction(COMBO_1, COMBO_1_COEFFICIENT, COMBO_2);
        tables.actions[COMBO_2] = MakeComboAction(COMBO_2, COMBO_2_COEFFICIENT, nullopt);
        tables.actionTimings[COMBO_1] = MakeComboTiming(COMBO_1_WINDOW, true);
        tables.actionTimings[COMBO_2] = MakeComboTiming(nullopt, false);
        tables.rootMotionSampleRate = ROOT_MOTION_SAMPLE_RATE;

        tables.monsters[MONSTER_TEMPLATE_ID] = MakeMonsterRow();
        TestCreatureCombat::AddMonster(tables, MONSTER_TEMPLATE_ID);
        Gamedata::Install(std::move(tables));

        room = CreateTestRoom(ROOM_ID, 0, 5000.f);
        ASSERT_NE(room, nullptr);

        attacker = EnterPlayer(attackerSession, 0.f, 0.f);
        observer = EnterPlayer(observerSession, -300.f, 0.f);
        ASSERT_NE(attacker, nullptr);
        ASSERT_NE(observer, nullptr);

        attackerSession->ClearSent();
        observerSession->ClearSent();
    }

    void TearDown() override
    {
        attacker.reset();
        observer.reset();
        room.reset();
        Gamedata::Install(GamedataTables());
    }

    PlayerRef EnterPlayer(shared_ptr<RecordingSession>& outSession, float x, float y)
    {
        outSession = make_shared<RecordingSession>();

        PlayerProgress progress;
        progress.classId = Protocol::CHARACTER_CLASS_WARRIOR;
        progress.level = 1;
        progress.hp = PLAYER_MAX_HP;

        PlayerSpawnParams params;
        params.session = outSession;
        params.progress = &progress;
        PlayerRef player = EntityFactory::Create<Player>(params);
        if (player == nullptr)
            return nullptr;

        RoomEnterData enterData{};
        enterData.nextRoomId = ROOM_ID;
        enterData.enterPos = MakePos(x, y);
        if (room->EnterPlayer(player, enterData) == false)
            return nullptr;

        return player;
    }

    MonsterRef SpawnMonster(float x, float y)
    {
        MonsterSpawnParams params;
        params.templateId = MONSTER_TEMPLATE_ID;
        params.spawnPos = MakePos(x, y);
        params.spawnPoint = SpawnPointPlacement{ 1, vector3D(x, y, 0.f), 0.f };
        return room->SpawnEntity<Monster>(params);
    }

    /** 공격자의 지금 위치에서 yaw 방향으로 시작하는 액션 요청. */
    Protocol::C_ACTION MakeAction(int32 actionId, float yaw = 0.f)
    {
        const Protocol::Vector& pos = attacker->GetPosInfo().pos();

        Protocol::C_ACTION pkt;
        pkt.set_action_id(actionId);
        pkt.set_yaw(yaw);
        pkt.mutable_start_pos()->CopyFrom(pos);
        return pkt;
    }

    /** 공격자가 받은 마지막 액션 응답의 결과. 응답이 없으면 실패한다. */
    Protocol::CombatResult LastActionResult()
    {
        const auto responses = attackerSession->SentPackets<Protocol::S_ACTION>(PKT_S_ACTION);
        EXPECT_FALSE(responses.empty()) << "요청한 플레이어는 언제나 응답을 받는다";
        return responses.empty() ? Protocol::COMBAT_RESULT_SUCCESS : responses.back().result();
    }

    /** 룸 틱 간격으로 seconds만큼 돌린다. */
    void TickFor(float seconds)
    {
        for (float elapsed = 0.f; elapsed < seconds - 0.001f; elapsed += ROOM_TICK_TIME)
            room->Tick(ROOM_TICK_TIME);
    }

    int64 HpOf(const MonsterRef& monster) const { return monster->GetStatValue(Protocol::STAT_TYPE_HP); }

    RoomRef room;
    PlayerRef attacker;
    PlayerRef observer;
    shared_ptr<RecordingSession> attackerSession;
    shared_ptr<RecordingSession> observerSession;
};

TEST_F(CombatRequestTest, ActionRespondsToAttackerAndNotifiesOthers)
{
    room->C_HandleAction(MakeAction(COMBO_1, 45.f), attacker);

    const auto responses = attackerSession->SentPackets<Protocol::S_ACTION>(PKT_S_ACTION);
    ASSERT_EQ(responses.size(), 1u) << "요청한 플레이어는 언제나 응답을 받는다";
    EXPECT_EQ(responses[0].result(), Protocol::COMBAT_RESULT_SUCCESS);
    EXPECT_TRUE(attackerSession->SentPackets<Protocol::S_ACTION_START>(PKT_S_ACTION_START).empty()) << "공격한 플레이어는 자기 공격을 이미 재생했다";

    const auto notices = observerSession->SentPackets<Protocol::S_ACTION_START>(PKT_S_ACTION_START);
    ASSERT_EQ(notices.size(), 1u);
    EXPECT_EQ(notices[0].entity_id(), attacker->GetEntityId());
    EXPECT_EQ(notices[0].action_id(), COMBO_1);
    EXPECT_FLOAT_EQ(notices[0].yaw(), 45.f) << "공격한 순간의 방향은 클라이언트가 보낸 값이다";
    EXPECT_TRUE(observerSession->SentPackets<Protocol::S_ACTION>(PKT_S_ACTION).empty()) << "응답은 요청한 플레이어에게만 간다";
}

TEST_F(CombatRequestTest, AcceptedActionMovesAttackerToStartPosition)
{
    Protocol::C_ACTION pkt = MakeAction(COMBO_1, 30.f);
    pkt.mutable_start_pos()->set_x(pkt.start_pos().x() + 30.f);

    room->C_HandleAction(pkt, attacker);

    ASSERT_EQ(LastActionResult(), Protocol::COMBAT_RESULT_SUCCESS);
    EXPECT_FLOAT_EQ(attacker->GetPosInfo().pos().x(), 30.f) << "이동 패킷보다 앞선 시작 위치를 서버의 위치로 받아들인다";
    EXPECT_FLOAT_EQ(attacker->GetPosInfo().yaw(), 30.f);
}

TEST_F(CombatRequestTest, DeadPlayerActionIsRejectedWithoutNotice)
{
    AttackInfo lethalAttack;
    lethalAttack.damage = PLAYER_MAX_HP * 10;
    attacker->OnHit(nullptr, lethalAttack);
    ASSERT_TRUE(attacker->IsDead());

    room->C_HandleAction(MakeAction(COMBO_1), attacker);

    EXPECT_EQ(LastActionResult(), Protocol::COMBAT_RESULT_DEAD);
    EXPECT_TRUE(observerSession->SentPackets<Protocol::S_ACTION_START>(PKT_S_ACTION_START).empty()) << "사망한 플레이어가 공격하는 모습이 다른 클라이언트에 보이면 안 된다";
}

TEST_F(CombatRequestTest, InvalidYawIsRejectedWithoutNotice)
{
    room->C_HandleAction(MakeAction(COMBO_1, std::numeric_limits<float>::quiet_NaN()), attacker);
    room->C_HandleAction(MakeAction(COMBO_1, 400.f), attacker);

    const auto responses = attackerSession->SentPackets<Protocol::S_ACTION>(PKT_S_ACTION);
    ASSERT_EQ(responses.size(), 2u);
    EXPECT_EQ(responses[0].result(), Protocol::COMBAT_RESULT_INVALID_YAW);
    EXPECT_EQ(responses[1].result(), Protocol::COMBAT_RESULT_INVALID_YAW);
    EXPECT_TRUE(observerSession->SentPackets<Protocol::S_ACTION_START>(PKT_S_ACTION_START).empty());
}

TEST_F(CombatRequestTest, ActionOutsideComboIsRejectedWithoutNotice)
{
    room->C_HandleAction(MakeAction(COMBO_2), attacker);

    EXPECT_EQ(LastActionResult(), Protocol::COMBAT_RESULT_INVALID_ACTION) << "콤보의 첫 단계를 건너뛸 수 없다";
    EXPECT_TRUE(observerSession->SentPackets<Protocol::S_ACTION_START>(PKT_S_ACTION_START).empty());
    EXPECT_FALSE(attacker->GetAction().IsActing());
}

TEST_F(CombatRequestTest, NextStepBeforeComboWindowIsRejectedAndFirstStepContinues)
{
    room->C_HandleAction(MakeAction(COMBO_1), attacker);
    ASSERT_EQ(LastActionResult(), Protocol::COMBAT_RESULT_SUCCESS);
    observerSession->ClearSent();

    TickFor(0.1f);
    room->C_HandleAction(MakeAction(COMBO_2), attacker);

    EXPECT_EQ(LastActionResult(), Protocol::COMBAT_RESULT_TOO_EARLY);
    EXPECT_TRUE(observerSession->SentPackets<Protocol::S_ACTION_START>(PKT_S_ACTION_START).empty());
    ASSERT_TRUE(attacker->GetAction().IsActing()) << "거절한 요청은 진행 중인 액션을 끊지 않는다";
    EXPECT_EQ(attacker->GetAction().GetLastAction()->actionId, COMBO_1);
}

TEST_F(CombatRequestTest, NextStepAtComboWindowIsAccepted)
{
    room->C_HandleAction(MakeAction(COMBO_1), attacker);
    TickFor(COMBO_1_WINDOW.start);
    observerSession->ClearSent();

    room->C_HandleAction(MakeAction(COMBO_2), attacker);

    EXPECT_EQ(LastActionResult(), Protocol::COMBAT_RESULT_SUCCESS);
    const auto notices = observerSession->SentPackets<Protocol::S_ACTION_START>(PKT_S_ACTION_START);
    ASSERT_EQ(notices.size(), 1u);
    EXPECT_EQ(notices[0].action_id(), COMBO_2);
}

TEST_F(CombatRequestTest, StartPositionFailingMoveRuleIsRejected)
{
    Protocol::C_ACTION pkt = MakeAction(COMBO_1);
    pkt.mutable_start_pos()->set_x(3000.f);

    room->C_HandleAction(pkt, attacker);

    EXPECT_EQ(LastActionResult(), Protocol::COMBAT_RESULT_INVALID_POSITION) << "공격 패킷으로 이동 판정을 건너뛰어 순간이동할 수 없다";
    EXPECT_FLOAT_EQ(attacker->GetPosInfo().pos().x(), 0.f) << "거절하면 서버의 위치를 그대로 둔다";
    EXPECT_TRUE(observerSession->SentPackets<Protocol::S_ACTION_START>(PKT_S_ACTION_START).empty());
    EXPECT_FALSE(attacker->GetAction().IsActing());
}

TEST_F(CombatRequestTest, PlayerHitDamagesMonsterInAttackAreaOnly)
{
    MonsterRef inside = SpawnMonster(FORWARD_AT_HIT + HIT_REACH - 5.f, 0.f);
    MonsterRef outside = SpawnMonster(FORWARD_AT_HIT, HIT_REACH + 5.f);
    ASSERT_NE(inside, nullptr);
    ASSERT_NE(outside, nullptr);

    room->C_HandleAction(MakeAction(COMBO_1), attacker);
    ASSERT_EQ(LastActionResult(), Protocol::COMBAT_RESULT_SUCCESS);
    TickFor(HIT_TIME - ROOM_TICK_TIME);
    ASSERT_EQ(HpOf(inside), MONSTER_MAX_HP) << "타격 시점 전에는 맞지 않는다";

    TickFor(ROOM_TICK_TIME);

    EXPECT_EQ(HpOf(inside), MONSTER_MAX_HP - static_cast<int64>(PLAYER_ATTACK * COMBO_1_COEFFICIENT)) <<"피해량은 플레이어의 물리 공격력 × 액션의 계수다";
    EXPECT_EQ(HpOf(outside), MONSTER_MAX_HP) << "공격 범위 밖의 몬스터는 맞지 않는다";

    const auto hits = observerSession->SentPackets<Protocol::S_HIT>(PKT_S_HIT);
    ASSERT_EQ(hits.size(), 1u);
    EXPECT_EQ(hits[0].entity_id(), inside->GetEntityId());
}

TEST_F(CombatRequestTest, HitIsJudgedWhereRootMotionCarriedAttackerInStartDirection)
{
    // 시작 방향 90도면 루트 모션의 앞이 월드 +Y다. 시작 위치에서는 닿지 않는 앞쪽 몬스터가 맞고, 시작 위치 바로 뒤의 몬스터는 맞지 않는다.
    MonsterRef ahead = SpawnMonster(0.f, FORWARD_AT_HIT + HIT_REACH - 5.f);
    MonsterRef behindStart = SpawnMonster(0.f, -40.f);
    ASSERT_NE(ahead, nullptr);
    ASSERT_NE(behindStart, nullptr);

    room->C_HandleAction(MakeAction(COMBO_1, 90.f), attacker);
    ASSERT_EQ(LastActionResult(), Protocol::COMBAT_RESULT_SUCCESS);
    TickFor(HIT_TIME);

    EXPECT_LT(HpOf(ahead), MONSTER_MAX_HP);
    EXPECT_EQ(HpOf(behindStart), MONSTER_MAX_HP);
}

TEST_F(CombatRequestTest, ComboKillingMonsterGivesRewardAndLevelUp)
{
    // 1단계는 루트 모션으로 나간 곳에서, 루트 모션이 없는 2단계는 같은 시작 위치에서 맞히므로 둘 다 닿는 자리에 둔다.
    MonsterRef monster = SpawnMonster(FORWARD_AT_HIT - 30.f, 0.f);
    ASSERT_NE(monster, nullptr);
    const int64 monsterId = monster->GetEntityId();

    room->C_HandleAction(MakeAction(COMBO_1), attacker);
    TickFor(COMBO_1_WINDOW.start);
    ASSERT_EQ(HpOf(monster), MONSTER_MAX_HP - static_cast<int64>(PLAYER_ATTACK * COMBO_1_COEFFICIENT));

    room->C_HandleAction(MakeAction(COMBO_2), attacker);
    ASSERT_EQ(LastActionResult(), Protocol::COMBAT_RESULT_SUCCESS);
    TickFor(HIT_TIME);

    EXPECT_TRUE(monster->IsDead());
    EXPECT_FALSE(room->Contains(monsterId)) << "처치한 몬스터는 룸에서 빠진다";

    const auto rewards = attackerSession->SentPackets<Protocol::S_REWARD>(PKT_S_REWARD);
    ASSERT_EQ(rewards.size(), 1u);
    EXPECT_EQ(rewards[0].gold(), MONSTER_GOLD);
    ASSERT_TRUE(rewards[0].has_level_up()) << "1레벨에 필요한 경험치만큼 주는 몬스터다";
    EXPECT_EQ(rewards[0].level_up().level(), 2);

    const auto deaths = observerSession->SentPackets<Protocol::S_DIE>(PKT_S_DIE);
    ASSERT_EQ(deaths.size(), 1u);
    EXPECT_EQ(deaths[0].entity_id(), monsterId);
}

TEST_F(CombatRequestTest, MoveNotifiesOthersWithSenderEntityId)
{
    const float startX = attacker->GetPosInfo().pos().x();

    Protocol::C_MOVE movePkt;
    movePkt.mutable_pos_info()->mutable_pos()->set_x(startX + 30.f);
    movePkt.mutable_pos_info()->set_state(Protocol::MOVE_STATE_RUN);
    room->C_HandleMove(movePkt, attacker);

    EXPECT_TRUE(attackerSession->SentPackets<Protocol::S_MOVE>(PKT_S_MOVE).empty()) << "이동한 플레이어는 자기 이동을 받지 않는다";

    const auto moves = observerSession->SentPackets<Protocol::S_MOVE>(PKT_S_MOVE);
    ASSERT_EQ(moves.size(), 1u);
    ASSERT_EQ(moves[0].moves_size(), 1);
    EXPECT_EQ(moves[0].moves(0).entity_id(), attacker->GetEntityId()) << "이동의 엔티티 번호는 보낸 플레이어의 것이다";
    EXPECT_FLOAT_EQ(moves[0].moves(0).pos_info().pos().x(), startX + 30.f);
    EXPECT_EQ(moves[0].moves(0).pos_info().state(), Protocol::MOVE_STATE_RUN);
}
