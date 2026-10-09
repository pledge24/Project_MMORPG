#include "Core/pch.h"
#include "DB/DBWorker.h"

void DBWorker::Run(DBQueueRef dbQueue)
{
    GLogger->Info("{}번째 DBQueue가 작업을 시작함", dbQueue->GetId());

    while (dbQueue->IsStop() == false)
    {
        JobRef job = dbQueue->WaitForSingleJob();
        if (job == nullptr)
            continue;

        RunJob(job);
    }
}

bool DBWorker::RunJob(const JobRef& job)
{
    // 잡 밖으로 나간 예외는 DB 스레드를 끝내고 std::terminate로 서버 전체를 내린다.
    // 한 계정의 잘못된 데이터가 서버를 내리지 않도록 여기서 모두 받는다.
    try
    {
        job->Execute();
        return true;
    }
    catch (const exception& error)
    {
        GLogger->Error("DB 잡이 예외로 끝났습니다: {}", error.what());
    }
    catch (...)
    {
        GLogger->Error("DB 잡이 표준 예외가 아닌 값을 던지고 끝났습니다");
    }

    return false;
}
