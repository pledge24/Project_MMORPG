#include "Core/pch.h"
#include <gtest/gtest.h>
#include "ServerCore/Job/JobTimer.h"
#include "ServerCore/Job/JobQueue.h"
#include "ServerCore/Job/GlobalQueue.h"

/*--------------------------------------------------------------
    예약 잡 분배 테스트

    JobTimer::Distribute는 만기된 예약 잡을 각 큐에 넘기기만 한다. 분배하던 스레드가 잡을 그 자리에서
    실행하면 잡이 끝날 때까지 다른 워커가 분배를 건너뛰고, 같은 묶음의 뒤쪽 잡도 늦게 들어간다(TD-015).

    픽스처 결합도: 지역 JobTimer와 JobQueue를 쓴다. 넘겨진 큐는 ServerContext가 만든 전역 GGlobalQueue에서
    테스트가 직접 꺼내 실행한다. 다른 테스트가 남긴 큐가 섞이지 않도록 앞뒤로 글로벌 큐를 비운다.
---------------------------------------------------------------*/

class JobTimerTest : public ::testing::Test
{
protected:
    void SetUp() override { DrainGlobalQueue(); }
    void TearDown() override { DrainGlobalQueue(); }

    static void DrainGlobalQueue()
    {
        while (GGlobalQueue->Pop() != nullptr)
        {
        }
    }

    JobTimer timer;
};

TEST_F(JobTimerTest, DistributeHandsDueJobsToGlobalQueueWithoutRunningThem)
{
    JobQueueRef first = make_shared<JobQueue>();
    JobQueueRef second = make_shared<JobQueue>();
    vector<string> events;
    timer.Reserve(0, first, make_shared<Job>([&events]() { events.push_back("first"); }));
    timer.Reserve(0, second, make_shared<Job>([&events]() { events.push_back("second"); }));

    timer.Distribute(::GetTickCount64() + 1);
    EXPECT_TRUE(events.empty()) << "분배하는 스레드는 잡을 실행하지 않는다";

    // 넘겨진 큐는 워커가 글로벌 큐에서 꺼내 실행한다.
    while (JobQueueRef queue = GGlobalQueue->Pop())
        queue->Execute();

    EXPECT_EQ(events.size(), 2u);
}

TEST_F(JobTimerTest, JobNotYetDueStaysReserved)
{
    JobQueueRef queue = make_shared<JobQueue>();
    int32 runCount = 0;
    timer.Reserve(60'000, queue, make_shared<Job>([&runCount]() { runCount++; }));

    timer.Distribute(::GetTickCount64());

    EXPECT_EQ(GGlobalQueue->Pop(), nullptr);
    EXPECT_EQ(runCount, 0);
    timer.Clear();
}
