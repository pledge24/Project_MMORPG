#include "Core/pch.h"
#include <gtest/gtest.h>
#include "RecordingSession.h"
#include "Network/GameSessionManager.h"

/*--------------------------------------------------------------
    계정 세션 교체 테스트

    한 계정은 게임 서버에 동시에 하나만 접속한다. 같은 계정의 새 로비 접속이 오면
    관리자는 기존 세션을 돌려주고 새 세션으로 바꾼다. 기존 세션을 끊는 일은
    BindAccount가 한다. 밀려난 세션이 나중에 접속 종료로 등록 해제되어도
    새 세션의 등록은 남아야 한다.

    로비 접속 잡은 BindAccount 하나로 계정을 묶는다. 앞 세션이 있으면 진행 조율자에 알리고 끊긴 사유를 보낸 뒤 끊는다.
    로비가 진행 조율자와 플레이어를 모르도록 이 일을 세션 관리자가 맡는다.

    픽스처 결합도: 전역 GSessionManager 대신 지역 관리자를 쓴다. 세션은 연결하지 않는다.
    BindAccount는 TestMain의 ServerContext가 만든 전역 진행 조율자와 세션 잡 큐를 쓴다.
---------------------------------------------------------------*/

namespace
{
    constexpr int64 USER_ID = 7;
    constexpr int64 OTHER_USER_ID = 8;
}

class GameSessionManagerTest : public ::testing::Test
{
protected:
    GameSessionManager manager;
};

TEST_F(GameSessionManagerTest, FirstRegisterReplacesNothing)
{
    GameSessionRef session = make_shared<GameSession>();

    EXPECT_EQ(manager.RegisterUser(USER_ID, session), nullptr);
    EXPECT_EQ(session->GetUserId(), USER_ID);
}

TEST_F(GameSessionManagerTest, SameUserReturnsReplacedSession)
{
    GameSessionRef oldSession = make_shared<GameSession>();
    GameSessionRef newSession = make_shared<GameSession>();
    manager.RegisterUser(USER_ID, oldSession);

    EXPECT_EQ(manager.RegisterUser(USER_ID, newSession), oldSession);
}

TEST_F(GameSessionManagerTest, UnregisteringReplacedSessionKeepsNewRegistration)
{
    GameSessionRef oldSession = make_shared<GameSession>();
    GameSessionRef newSession = make_shared<GameSession>();
    manager.RegisterUser(USER_ID, oldSession);
    manager.RegisterUser(USER_ID, newSession);

    // 밀려난 세션의 접속 종료는 교체보다 늦게 온다.
    manager.UnregisterUser(oldSession);

    GameSessionRef thirdSession = make_shared<GameSession>();
    EXPECT_EQ(manager.RegisterUser(USER_ID, thirdSession), newSession);
}

TEST_F(GameSessionManagerTest, UnregisteringCurrentSessionReleasesUser)
{
    GameSessionRef session = make_shared<GameSession>();
    manager.RegisterUser(USER_ID, session);

    manager.UnregisterUser(session);

    GameSessionRef nextSession = make_shared<GameSession>();
    EXPECT_EQ(manager.RegisterUser(USER_ID, nextSession), nullptr);
}

TEST_F(GameSessionManagerTest, RebindingSessionToOtherUserReleasesPreviousUser)
{
    GameSessionRef session = make_shared<GameSession>();
    manager.RegisterUser(USER_ID, session);

    // 한 세션이 다른 계정의 인증 티켓으로 로비 접속을 다시 보냈다.
    manager.RegisterUser(OTHER_USER_ID, session);

    // 이전 계정의 새 로비 접속이 이 세션을 밀어내면 안 된다.
    EXPECT_EQ(manager.RegisterUser(USER_ID, make_shared<GameSession>()), nullptr);
}

TEST_F(GameSessionManagerTest, DifferentUsersDoNotReplaceEachOther)
{
    GameSessionRef session = make_shared<GameSession>();
    GameSessionRef otherSession = make_shared<GameSession>();
    manager.RegisterUser(USER_ID, session);

    EXPECT_EQ(manager.RegisterUser(OTHER_USER_ID, otherSession), nullptr);
    EXPECT_EQ(manager.RegisterUser(USER_ID, make_shared<GameSession>()), session);
}

// 앞 세션의 클라이언트는 끊긴 이유를 받아야 로그인 화면에 문구를 띄운다.
TEST_F(GameSessionManagerTest, BindAccountSendsDisconnectReasonToReplacedSession)
{
    auto oldSession = make_shared<RecordingSession>();
    manager.RegisterUser(USER_ID, oldSession);

    manager.BindAccount(USER_ID, make_shared<RecordingSession>());

    const vector<Protocol::S_DISCONNECT> sent = oldSession->SentPackets<Protocol::S_DISCONNECT>(PKT_S_DISCONNECT);
    ASSERT_EQ(sent.size(), 1u);
    EXPECT_EQ(sent[0].reason(), Protocol::DISCONNECT_REASON_DUPLICATE_CONNECTION);
}

TEST_F(GameSessionManagerTest, BindAccountWithoutPreviousSessionSendsNothing)
{
    auto session = make_shared<RecordingSession>();

    manager.BindAccount(USER_ID, session);

    EXPECT_TRUE(session->SentPackets<Protocol::S_DISCONNECT>(PKT_S_DISCONNECT).empty());
}

// 등록하기 전에 끊긴 세션은 접속 종료의 등록 해제를 이미 지나쳤다. 등록이 남으면 다음 로비 접속이 끊긴 세션을 밀어낸다.
TEST_F(GameSessionManagerTest, BindAccountOfDisconnectedSessionWithdrawsRegistration)
{
    auto session = make_shared<RecordingSession>();

    EXPECT_FALSE(manager.BindAccount(USER_ID, session)) << "연결하지 않은 세션은 끊긴 세션이다";

    EXPECT_EQ(manager.RegisterUser(USER_ID, make_shared<GameSession>()), nullptr);
}
