#include "Core/pch.h"
#include <gtest/gtest.h>
#include "RecordingSession.h"
#include "Network/GameEntry.h"
#include "Lobby/LobbyReturn.h"
#include "Game/Entities/Player.h"

/*--------------------------------------------------------------
    세션 단계와 단계별 허용표 테스트

    세션은 로비 접속 전, 로비 접속 중, 로비, 게임 입장 중, 게임, 로비 복귀 중 여섯 단계를 거치고, 단계마다 받는 패킷이 다르다.
    단계를 보지 않던 때는 게임에 입장한 세션이 플레이 중인 캐릭터를 지울 수 있었고, 다른 계정의
    인증 티켓으로 로비 접속을 다시 보내면 세션의 계정 번호만 바뀌었다.

    아래 PHASES_BY_PACKET이 허용표의 계약 선언이다. 표를 바꾸면 이 표도 의식적으로 고친다.
    표가 받는 패킷 전부를 덮는지는 생성된 핸들러 테이블과 대조한다.
---------------------------------------------------------------*/

namespace
{
    constexpr SessionPhase ALL_PHASES[] =
    {
        SessionPhase::BEFORE_LOBBY, SessionPhase::CONNECTING_LOBBY, SessionPhase::LOBBY, SessionPhase::ENTERING_GAME, SessionPhase::IN_GAME,
        SessionPhase::RETURNING_TO_LOBBY,
    };

    struct ExpectedPhases
    {
        uint16 packetId;
        const char* name;
        vector<SessionPhase> phases;
    };

    const vector<SessionPhase> BEFORE_LOBBY_ONLY = { SessionPhase::BEFORE_LOBBY };
    const vector<SessionPhase> LOBBY_ONLY = { SessionPhase::LOBBY };
    const vector<SessionPhase> IN_GAME_ONLY = { SessionPhase::IN_GAME };

    const vector<ExpectedPhases> PHASES_BY_PACKET =
    {
        { PKT_C_CONNECT_LOBBY, "C_CONNECT_LOBBY", BEFORE_LOBBY_ONLY },
        { PKT_C_CREATE_CHARACTER, "C_CREATE_CHARACTER", LOBBY_ONLY },
        { PKT_C_DELETE_CHARACTER, "C_DELETE_CHARACTER", LOBBY_ONLY },
        { PKT_C_ENTER_GAME, "C_ENTER_GAME", LOBBY_ONLY },
        { PKT_C_MAP_LOAD_COMPLETE, "C_MAP_LOAD_COMPLETE", IN_GAME_ONLY },
        { PKT_C_USE_PORTAL, "C_USE_PORTAL", IN_GAME_ONLY },
        { PKT_C_MOVE, "C_MOVE", IN_GAME_ONLY },
        { PKT_C_NORMAL_ATTACK, "C_NORMAL_ATTACK", IN_GAME_ONLY },
        { PKT_C_BUY_ITEM, "C_BUY_ITEM", IN_GAME_ONLY },
        { PKT_C_SELL_ITEM, "C_SELL_ITEM", IN_GAME_ONLY },
        { PKT_C_EQUIP_GEAR, "C_EQUIP_GEAR", IN_GAME_ONLY },
        { PKT_C_UNEQUIP_GEAR, "C_UNEQUIP_GEAR", IN_GAME_ONLY },
        { PKT_C_USE_ITEM, "C_USE_ITEM", IN_GAME_ONLY },
        { PKT_C_SET_QUICK_SLOT, "C_SET_QUICK_SLOT", IN_GAME_ONLY },
        { PKT_C_USE_QUICK_SLOT, "C_USE_QUICK_SLOT", IN_GAME_ONLY },
        { PKT_C_RESPAWN, "C_RESPAWN", IN_GAME_ONLY },
        { PKT_C_CHANNEL_LIST, "C_CHANNEL_LIST", IN_GAME_ONLY },
        { PKT_C_CHANGE_CHANNEL, "C_CHANGE_CHANNEL", IN_GAME_ONLY },
        { PKT_C_CHAT, "C_CHAT", IN_GAME_ONLY },
        { PKT_C_RETURN_TO_LOBBY, "C_RETURN_TO_LOBBY", IN_GAME_ONLY },
    };

    // 생성된 Init은 핸들러가 없는 칸에 Handle_INVALID 함수 포인터를 넣고, 받는 패킷의 칸에는 람다를 넣는다.
    bool HasHandler(uint16 packetId)
    {
        using HandlerPointer = bool(*)(PacketSessionRef&, BYTE*, int32);
        const HandlerPointer* pointer = GPacketHandler[packetId].target<HandlerPointer>();
        return pointer == nullptr || *pointer != &Handle_INVALID;
    }
}

TEST(SessionPhaseTableTest, EachPacketIsAcceptedOnlyInItsPhases)
{
    for (const ExpectedPhases& expected : PHASES_BY_PACKET)
    {
        for (SessionPhase phase : ALL_PHASES)
        {
            const bool accepted = ranges::find(expected.phases, phase) != expected.phases.end();
            EXPECT_EQ(SessionPhaseTable::Accepts(expected.packetId, phase), accepted)
                << expected.name << "를 " << ToString(phase) << " 단계에서 " << (accepted ? "받아야" : "버려야") << " 한다";
        }
    }
}

// 핸들러가 있는 패킷은 모두 표에 있고, 표에 있는 패킷은 모두 핸들러가 있다.
// 표에 없는 패킷은 어느 단계에서도 버리므로, 받는 패킷을 더하고 표를 잊으면 그 패킷이 늘 버려진다.
TEST(SessionPhaseTableTest, TableCoversExactlyPacketsWithHandler)
{
    ServerPacketHandler::Init();

    for (int32 id = 0; id < UINT16_MAX; id++)
    {
        const uint16 packetId = static_cast<uint16>(id);
        const bool inTable = ranges::any_of(ALL_PHASES, [packetId](SessionPhase phase) { return SessionPhaseTable::Accepts(packetId, phase); });
        EXPECT_EQ(inTable, HasHandler(packetId)) << "패킷 " << packetId;

        const bool inExpected = ranges::any_of(PHASES_BY_PACKET, [packetId](const ExpectedPhases& expected) { return expected.packetId == packetId; });
        EXPECT_EQ(inExpected, HasHandler(packetId)) << "이 테스트의 표가 핸들러가 있는 패킷 " << packetId << "을 덮지 않는다";
    }
}

TEST(SessionPhaseTableTest, IdOutsideHandlerTableIsRejected)
{
    for (SessionPhase phase : ALL_PHASES)
        EXPECT_FALSE(SessionPhaseTable::Accepts(UINT16_MAX, phase));
}

TEST(SessionPhaseTest, NewSessionIsBeforeLobby)
{
    RecordingSession session;
    EXPECT_EQ(session.GetPhase(), SessionPhase::BEFORE_LOBBY);
}

TEST(SessionPhaseTest, PhasesAdvanceInOrder)
{
    RecordingSession session;

    EXPECT_FALSE(session.BeginGameEntry()) << "로비 접속 전에는 게임 입장을 시작하지 않는다";
    EXPECT_FALSE(session.CompleteGameEntry());
    EXPECT_FALSE(session.CancelGameEntry());
    EXPECT_EQ(session.GetPhase(), SessionPhase::BEFORE_LOBBY);

    EXPECT_FALSE(session.CompleteLobbyConnect()) << "로비 접속을 요청하지 않은 세션은 로비 접속을 마치지 않는다";

    ASSERT_TRUE(session.BeginLobbyConnect());
    EXPECT_EQ(session.GetPhase(), SessionPhase::CONNECTING_LOBBY);
    EXPECT_FALSE(session.BeginLobbyConnect()) << "로비 접속 중인 세션은 두 번째 로비 접속을 시작하지 않는다";
    EXPECT_FALSE(session.BeginGameEntry()) << "로비 접속 중에는 게임 입장을 시작하지 않는다";

    ASSERT_TRUE(session.CompleteLobbyConnect());
    EXPECT_EQ(session.GetPhase(), SessionPhase::LOBBY);
    EXPECT_FALSE(session.CompleteLobbyConnect()) << "로비 접속은 한 번만 마친다";
    EXPECT_FALSE(session.BeginLobbyConnect());
    EXPECT_FALSE(session.CompleteGameEntry()) << "입장을 시작하지 않은 세션은 입장을 마치지 않는다";

    ASSERT_TRUE(session.BeginGameEntry());
    EXPECT_EQ(session.GetPhase(), SessionPhase::ENTERING_GAME);
    EXPECT_FALSE(session.BeginGameEntry()) << "입장 중인 세션은 두 번째 입장을 시작하지 않는다";

    ASSERT_TRUE(session.CompleteGameEntry());
    EXPECT_EQ(session.GetPhase(), SessionPhase::IN_GAME);
    EXPECT_FALSE(session.CancelGameEntry()) << "게임에 입장한 세션은 로비로 되돌아가지 않는다";
    EXPECT_FALSE(session.CompleteLobbyConnect());
    EXPECT_EQ(session.GetPhase(), SessionPhase::IN_GAME);
}

TEST(SessionPhaseTest, CancelledEntryReturnsToLobby)
{
    RecordingSession session;
    session.BeginLobbyConnect();
    session.CompleteLobbyConnect();
    session.BeginGameEntry();

    ASSERT_TRUE(session.CancelGameEntry());
    EXPECT_EQ(session.GetPhase(), SessionPhase::LOBBY);
    EXPECT_TRUE(session.BeginGameEntry()) << "거절된 뒤에는 다시 캐릭터를 골라 입장할 수 있다";
}

// 입장 결과의 응답을 받은 클라이언트는 곧바로 다음 요청(맵 로드 완료, 다른 캐릭터의 입장)을 보낸다.
// 응답 뒤에 단계를 바꾸면 그 요청이 앞 단계에서 버려진다.
TEST(GameEntryFinishTest, SuccessMovesSessionToGameBeforeResponse)
{
    shared_ptr<RecordingSession> session = make_shared<RecordingSession>();
    session->BeginLobbyConnect();
    session->CompleteLobbyConnect();
    session->BeginGameEntry();

    Protocol::S_ENTER_GAME enterGamePkt;
    enterGamePkt.set_result(Protocol::ENTRY_RESULT_SUCCESS);
    GameEntry::FinishEnter(session, enterGamePkt);

    EXPECT_EQ(session->GetPhase(), SessionPhase::IN_GAME);
    const vector<Protocol::S_ENTER_GAME> responses = session->SentPackets<Protocol::S_ENTER_GAME>(PKT_S_ENTER_GAME);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::ENTRY_RESULT_SUCCESS);
}

TEST(GameEntryFinishTest, FailureReturnsSessionToLobby)
{
    shared_ptr<RecordingSession> session = make_shared<RecordingSession>();
    session->BeginLobbyConnect();
    session->CompleteLobbyConnect();
    session->BeginGameEntry();

    Protocol::S_ENTER_GAME enterGamePkt;
    enterGamePkt.set_result(Protocol::ENTRY_RESULT_PREVIOUS_SAVE_PENDING);
    GameEntry::FinishEnter(session, enterGamePkt);

    EXPECT_EQ(session->GetPhase(), SessionPhase::LOBBY);
    const vector<Protocol::S_ENTER_GAME> responses = session->SentPackets<Protocol::S_ENTER_GAME>(PKT_S_ENTER_GAME);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::ENTRY_RESULT_PREVIOUS_SAVE_PENDING);
}

namespace
{
    // 게임에 입장해 플레이어를 등록한 세션.
    shared_ptr<RecordingSession> MakeInGameSession()
    {
        shared_ptr<RecordingSession> session = make_shared<RecordingSession>();
        session->BeginLobbyConnect();
        session->CompleteLobbyConnect();
        session->BeginGameEntry();
        session->CompleteGameEntry();
        EXPECT_TRUE(session->TryRegisterPlayer(make_shared<Player>()));
        return session;
    }
}

TEST(SessionPhaseTest, LobbyReturnStartsOnlyInGame)
{
    RecordingSession session;
    EXPECT_FALSE(session.BeginLobbyReturn()) << "게임에 입장하지 않은 세션은 로비로 복귀하지 않는다";

    session.BeginLobbyConnect();
    session.CompleteLobbyConnect();
    EXPECT_FALSE(session.BeginLobbyReturn());
    EXPECT_FALSE(session.CompleteLobbyReturn()) << "복귀를 시작하지 않은 세션은 복귀를 마치지 않는다";
    EXPECT_EQ(session.GetPhase(), SessionPhase::LOBBY);
}

TEST(SessionPhaseTest, LobbyReturnGoesBackToLobbyWithoutPlayer)
{
    shared_ptr<RecordingSession> session = MakeInGameSession();

    ASSERT_TRUE(session->BeginLobbyReturn());
    EXPECT_EQ(session->GetPhase(), SessionPhase::RETURNING_TO_LOBBY);
    EXPECT_FALSE(session->BeginLobbyReturn()) << "복귀 중인 세션은 두 번째 복귀를 시작하지 않는다";
    EXPECT_NE(session->GetPlayer(), nullptr) << "저장이 끝날 때까지 플레이어를 남긴다. 그사이 끊기면 접속 종료가 그 플레이어를 본다";

    ASSERT_TRUE(session->CompleteLobbyReturn());
    EXPECT_EQ(session->GetPhase(), SessionPhase::LOBBY);
    EXPECT_EQ(session->GetPlayer(), nullptr);
    EXPECT_TRUE(session->TryRegisterPlayer(make_shared<Player>())) << "복귀한 세션은 같은 연결로 다시 입장한다";
}

// 목록을 받은 클라이언트는 곧바로 로비 요청을 보낸다. 응답 뒤에 단계를 바꾸면 그 요청이 복귀 중 단계에서 버려진다.
TEST(LobbyReturnFinishTest, SuccessMovesSessionToLobbyThenSendsResponseAndList)
{
    shared_ptr<RecordingSession> session = MakeInGameSession();
    session->BeginLobbyReturn();

    Protocol::S_CHARACTER_LIST characterListPkt;
    characterListPkt.set_character_slot_count(3);
    LobbyReturn::Finish(session, characterListPkt);

    EXPECT_EQ(session->GetPhase(), SessionPhase::LOBBY);
    const vector<Protocol::S_RETURN_TO_LOBBY> responses = session->SentPackets<Protocol::S_RETURN_TO_LOBBY>(PKT_S_RETURN_TO_LOBBY);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::ENTRY_RESULT_SUCCESS);
    const vector<Protocol::S_CHARACTER_LIST> lists = session->SentPackets<Protocol::S_CHARACTER_LIST>(PKT_S_CHARACTER_LIST);
    ASSERT_EQ(lists.size(), 1u);
    EXPECT_EQ(lists[0].character_slot_count(), 3);
}

TEST(LobbyReturnFinishTest, ListLoadFailureSendsInternalErrorWithoutList)
{
    shared_ptr<RecordingSession> session = MakeInGameSession();
    session->BeginLobbyReturn();

    LobbyReturn::Finish(session, nullopt);

    const vector<Protocol::S_RETURN_TO_LOBBY> responses = session->SentPackets<Protocol::S_RETURN_TO_LOBBY>(PKT_S_RETURN_TO_LOBBY);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].result(), Protocol::ENTRY_RESULT_INTERNAL_ERROR);
    EXPECT_TRUE(session->SentPackets<Protocol::S_CHARACTER_LIST>(PKT_S_CHARACTER_LIST).empty());
    EXPECT_NE(session->GetPhase(), SessionPhase::LOBBY) << "목록이 없는 세션을 로비로 받지 않는다. 결과를 알린 뒤 끊는다";
}
