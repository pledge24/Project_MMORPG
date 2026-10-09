#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Network/GameSessionManager.h"

/*--------------------------------------------------------------
    계정 세션 교체 테스트

    한 계정은 게임 서버에 동시에 하나만 접속한다. 같은 계정의 새 로그인이 오면
    관리자는 기존 세션을 돌려주고 새 세션으로 바꾼다. 기존 세션을 끊는 일은
    호출자(Handle_C_LOGIN)가 한다. 밀려난 세션이 나중에 접속 종료로 등록 해제되어도
    새 세션의 등록은 남아야 한다.

    픽스처 결합도: 전역 GSessionManager 대신 지역 관리자를 쓴다. 세션은 연결하지 않는다.
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
    EXPECT_EQ(session->_userId, USER_ID);
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

    // 한 세션이 다른 계정의 토큰으로 C_LOGIN을 다시 보냈다.
    manager.RegisterUser(OTHER_USER_ID, session);

    // 이전 계정의 새 로그인이 이 세션을 밀어내면 안 된다.
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
