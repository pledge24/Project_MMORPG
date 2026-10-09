#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Network/SaveGate.h"

/*--------------------------------------------------------------
    저장 대기 테스트

    접속 종료 저장이 끝나기 전에는 같은 계정의 입장 불러오기를 하지 않는다.
    밀려난 세션의 저장이 userId DB 큐에 들어가기 전에 새 세션의 불러오기가 먼저
    들어가면 저장 전의 진행을 불러오고, 새 세션이 끊길 때 그 낡은 진행으로 덮어쓴다.

    픽스처 결합도: 전역 GSaveGate 대신 지역 객체를 쓴다. 맡기는 불러오기는 호출 횟수만 센다.
---------------------------------------------------------------*/

namespace
{
    constexpr int64 USER_ID = 7;
    constexpr int64 OTHER_USER_ID = 8;
}

class SaveGateTest : public ::testing::Test
{
protected:
    SaveGate::ParkedLoad MakeLoad()
    {
        return SaveGate::ParkedLoad{ [this]() { runCount++; }, [this]() { rejectCount++; } };
    }

    SaveGate gate;
    int32 runCount = 0;
    int32 rejectCount = 0;
};

TEST_F(SaveGateTest, NotHeldAccountDoesNotPark)
{
    EXPECT_EQ(gate.Park(USER_ID, MakeLoad()).result, SaveGate::ParkResult::NOT_HELD);
    EXPECT_FALSE(gate.Release(USER_ID).has_value());
}

TEST_F(SaveGateTest, ReleaseHandsBackParkedLoad)
{
    gate.Hold(USER_ID);

    EXPECT_EQ(gate.Park(USER_ID, MakeLoad()).result, SaveGate::ParkResult::PARKED);

    optional<SaveGate::ParkedLoad> released = gate.Release(USER_ID);
    ASSERT_TRUE(released.has_value());
    released->run();
    EXPECT_EQ(runCount, 1);
    EXPECT_EQ(rejectCount, 0);
}

TEST_F(SaveGateTest, ReleaseEndsHold)
{
    gate.Hold(USER_ID);
    EXPECT_FALSE(gate.Release(USER_ID).has_value());

    EXPECT_EQ(gate.Park(USER_ID, MakeLoad()).result, SaveGate::ParkResult::NOT_HELD);
}

TEST_F(SaveGateTest, SecondParkIsBusy)
{
    gate.Hold(USER_ID);
    gate.Park(USER_ID, MakeLoad());

    EXPECT_EQ(gate.Park(USER_ID, MakeLoad()).result, SaveGate::ParkResult::BUSY);

    // 먼저 맡긴 불러오기가 그대로 남아 있다.
    EXPECT_TRUE(gate.Release(USER_ID).has_value());
}

// 밀어낼 때와 접속 종료 때 두 번 걸어도 저장은 한 번이므로 한 번의 해제로 풀린다.
TEST_F(SaveGateTest, HoldTwiceKeepsParkedLoad)
{
    gate.Hold(USER_ID);
    gate.Park(USER_ID, MakeLoad());
    gate.Hold(USER_ID);

    EXPECT_TRUE(gate.Release(USER_ID).has_value());
    EXPECT_EQ(gate.Park(USER_ID, MakeLoad()).result, SaveGate::ParkResult::NOT_HELD);
}

// TD-003: 만료는 기다리던 입장만 돌려준다. 저장은 아직 끝나지 않았으므로 대기는 남아 다음 입장도 기다린다.
TEST_F(SaveGateTest, ExpireHandsBackParkedLoadButKeepsHold)
{
    gate.Hold(USER_ID);
    SaveGate::ParkTicket ticket = gate.Park(USER_ID, MakeLoad());

    optional<SaveGate::ParkedLoad> expired = gate.Expire(USER_ID, ticket.token);
    ASSERT_TRUE(expired.has_value());
    expired->reject();
    EXPECT_EQ(rejectCount, 1);

    SaveGate::ParkTicket next = gate.Park(USER_ID, MakeLoad());
    EXPECT_EQ(next.result, SaveGate::ParkResult::PARKED);
    EXPECT_NE(next.token, ticket.token);
    EXPECT_FALSE(gate.Expire(USER_ID, ticket.token).has_value()) << "지난 만료는 새로 맡긴 입장을 거절하지 않는다";
    EXPECT_TRUE(gate.Release(USER_ID).has_value());
}

// 저장이 먼저 끝나 풀렸으면 늦게 온 만료 타이머는 아무것도 하지 않는다.
TEST_F(SaveGateTest, ExpireAfterReleaseDoesNothing)
{
    gate.Hold(USER_ID);
    SaveGate::ParkTicket ticket = gate.Park(USER_ID, MakeLoad());
    gate.Release(USER_ID);

    EXPECT_FALSE(gate.Expire(USER_ID, ticket.token).has_value());
}

// 앞선 불러오기의 만료 타이머가 다음 대기에 맡긴 불러오기를 거절하지 않는다.
TEST_F(SaveGateTest, StaleTokenDoesNotExpireNewerPark)
{
    gate.Hold(USER_ID);
    SaveGate::ParkTicket oldTicket = gate.Park(USER_ID, MakeLoad());
    gate.Release(USER_ID);

    gate.Hold(USER_ID);
    SaveGate::ParkTicket newTicket = gate.Park(USER_ID, MakeLoad());
    ASSERT_EQ(newTicket.result, SaveGate::ParkResult::PARKED);
    EXPECT_NE(oldTicket.token, newTicket.token);

    EXPECT_FALSE(gate.Expire(USER_ID, oldTicket.token).has_value());
    EXPECT_TRUE(gate.Release(USER_ID).has_value());
}

TEST_F(SaveGateTest, HoldIsPerAccount)
{
    gate.Hold(USER_ID);

    EXPECT_EQ(gate.Park(OTHER_USER_ID, MakeLoad()).result, SaveGate::ParkResult::NOT_HELD);
}
