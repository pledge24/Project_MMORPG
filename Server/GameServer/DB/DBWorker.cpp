#include "Core/pch.h"
#include "DB/DBWorker.h"

void DBWorker::Run(DBQueueRef dbQueue)
{
    GLogger->Info("{}번째 DBQueue가 작업을 시작함", dbQueue->GetId());

    // 멈춘 큐는 남은 잡을 다 내준 뒤에야 nullptr를 돌려준다. 그 전에 끝내면 접속 종료 저장이 사라진다.
    while (JobRef job = dbQueue->WaitForSingleJob())
        RunJob(job);

    GLogger->Info("{}번째 DBQueue가 작업을 마침", dbQueue->GetId());
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
