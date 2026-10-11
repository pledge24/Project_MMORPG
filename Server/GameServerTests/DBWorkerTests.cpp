#include "Core/pch.h"
#include <gtest/gtest.h>
#include "DB/DBWorker.h"
#include "DB/DAOCommon.h"

/*--------------------------------------------------------------
    DB 스레드 테스트

    DB 스레드는 잡이 던진 예외를 받지 않으면 std::terminate로 서버 전체가 끝난다. 한 계정의
    잘못된 데이터가 Json 예외나 bad_alloc을 낼 수 있으므로, 잡 경계에서 모든 예외를 받아 로그를
    남기고 다음 잡을 계속 돌려야 한다(TD-039).

    픽스처 결합도: 없음. 잡을 직접 만들어 RunJob에 넘긴다.
---------------------------------------------------------------*/

TEST(DBWorkerTest, CompletedJobReportsSuccess)
{
    bool ran = false;
    JobRef job = make_shared<Job>([&ran]() { ran = true; });

    EXPECT_TRUE(DBWorker::RunJob(job));
    EXPECT_TRUE(ran);
}

TEST(DBWorkerTest, StandardExceptionDoesNotEscapeJob)
{
    JobRef job = make_shared<Job>([]() { throw runtime_error("잡 안의 예외"); });

    bool succeeded = true;
    EXPECT_NO_THROW(succeeded = DBWorker::RunJob(job)) << "잡 밖으로 나간 예외는 DB 스레드를 끝내고 서버를 종료시킨다";
    EXPECT_FALSE(succeeded);
}

TEST(DBWorkerTest, DBErrorDoesNotEscapeJob)
{
    JobRef job = make_shared<Job>([]() { throw DBError("Test", "실패"); });

    EXPECT_NO_THROW(DBWorker::RunJob(job));
}

TEST(DBWorkerTest, NonStandardExceptionDoesNotEscapeJob)
{
    JobRef job = make_shared<Job>([]() { throw 42; });

    EXPECT_NO_THROW(DBWorker::RunJob(job));
}

// TD-016: DB 큐를 멈출 수단이 없으면 DB 스레드가 끝나지 않아 정상 종료를 만들 수 없다.
// 멈출 때 이미 들어온 잡(접속 종료 저장 등)은 버리지 않고 다 돌린 뒤 끝난다.
TEST(DBWorkerTest, StoppedQueueRunsPendingJobsThenEndsWorkerLoop)
{
    DBQueueRef dbQueue = make_shared<DBQueue>(0);
    bool ran = false;
    dbQueue->Push(make_shared<Job>([&ran]() { ran = true; }));

    dbQueue->Stop();
    DBWorker::Run(dbQueue);

    EXPECT_TRUE(ran) << "멈추기 전에 들어온 저장 잡을 버리면 진행이 사라진다";
}

TEST(DBWorkerTest, StoppedQueueIgnoresNewJobs)
{
    DBQueue dbQueue(0);
    dbQueue.Stop();

    dbQueue.Push(make_shared<Job>([]() {}));

    EXPECT_EQ(dbQueue.WaitForSingleJob(), nullptr) << "멈춘 큐에 넣은 잡은 실행되지 않는다";
}
