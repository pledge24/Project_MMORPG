#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerProgress.h"
#include "RecordingSession.h"

/*--------------------------------------------------------------
    룸의 공격과 이동 요청 테스트

    요청한 플레이어는 응답을 받고, 같은 룸의 다른 플레이어는 통지를 받는다. 통지는 엔티티 번호를
    메시지의 한 곳에만 싣는다.

    픽스처 결합도: Room::Create로 룸을 만들고 Start()는 부르지 않는다. 두 플레이어를 RecordingSession에 묶어
    EnterPlayer로 넣는다. 1레벨 전사 레벨 표를 Gamedata::Install로 주입한다.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 ROOM_ID = 10;
    constexpr int32 PLAYER_MAX_HP = 100;
}

class CombatRequestTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        LevelTemplate level1;
        level1.level = 1;
        level1.maxHp = PLAYER_MAX_HP;

        GamedataTables tables;
        tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ClassLevelTable({ level1 });
        Gamedata::Install(std::move(tables));

        MapTemplate mapTemplate;
        mapTemplate.templateId = ROOM_ID;
        mapTemplate.depthHalfExtent = 5000.f;
        mapTemplate.widthHalfExtent = 5000.f;
        room = Room::Create(mapTemplate);
        ASSERT_NE(room, nullptr);

        attacker = EnterPlayer(attackerSession);
        observer = EnterPlayer(observerSession);
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

    PlayerRef EnterPlayer(shared_ptr<RecordingSession>& outSession)
    {
        outSession = make_shared<RecordingSession>();

        PlayerProgress progress;
        progress.classId = Protocol::CLASS_TYPE_WARRIOR;
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
        if (room->EnterPlayer(player, enterData) == false)
            return nullptr;

        return player;
    }

    Protocol::C_NORMAL_ATTACK MakeAttack()
    {
        Protocol::C_NORMAL_ATTACK pkt;
        pkt.set_combo(2);
        pkt.set_yaw(45.f);
        return pkt;
    }

    RoomRef room;
    PlayerRef attacker;
    PlayerRef observer;
    shared_ptr<RecordingSession> attackerSession;
    shared_ptr<RecordingSession> observerSession;
};

TEST_F(CombatRequestTest, NormalAttackRespondsToAttackerAndNotifiesOthers)
{
    room->C_HandleNormalAttack(MakeAttack(), attacker);

    const auto responses = attackerSession->SentPackets<Protocol::S_NORMAL_ATTACK>(PKT_S_NORMAL_ATTACK);
    ASSERT_EQ(responses.size(), 1u) << "요청한 플레이어는 언제나 응답을 받는다";
    EXPECT_EQ(responses[0].result(), Protocol::COMBAT_RESULT_SUCCESS);
    EXPECT_TRUE(attackerSession->SentPackets<Protocol::S_ATTACK>(PKT_S_ATTACK).empty()) << "공격한 플레이어는 자기 공격을 이미 재생했다";

    const auto notices = observerSession->SentPackets<Protocol::S_ATTACK>(PKT_S_ATTACK);
    ASSERT_EQ(notices.size(), 1u);
    EXPECT_EQ(notices[0].entity_id(), attacker->GetEntityId());
    EXPECT_EQ(notices[0].combo(), 2);
    EXPECT_FLOAT_EQ(notices[0].yaw(), 45.f) << "공격한 순간의 방향은 클라이언트가 보낸 값이다";
    EXPECT_TRUE(observerSession->SentPackets<Protocol::S_NORMAL_ATTACK>(PKT_S_NORMAL_ATTACK).empty()) << "응답은 요청한 플레이어에게만 간다";
}

TEST_F(CombatRequestTest, DeadPlayerAttackIsRejectedWithoutNotice)
{
    AttackInfo lethalAttack;
    lethalAttack.damage = PLAYER_MAX_HP * 10;
    attacker->OnHit(nullptr, lethalAttack);
    ASSERT_TRUE(attacker->IsDead());

    room->C_HandleNormalAttack(MakeAttack(), attacker);

    const auto responses = attackerSession->SentPackets<Protocol::S_NORMAL_ATTACK>(PKT_S_NORMAL_ATTACK);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::COMBAT_RESULT_DEAD);
    EXPECT_TRUE(observerSession->SentPackets<Protocol::S_ATTACK>(PKT_S_ATTACK).empty()) << "사망한 플레이어가 공격하는 모습이 다른 클라이언트에 보이면 안 된다";
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
