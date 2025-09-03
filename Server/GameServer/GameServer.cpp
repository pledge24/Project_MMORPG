#include "pch.h"
#include <thread>
#include "Service.h"
#include "IocpCore.h"
#include "GameSession.h"
#include "config.h"
#include "Global.h"
#include "EncodingConverter.h"

enum
{
	WORKER_TICK = 64
};

void DoDBJob(int dbQueueId)
{
    DBQueueRef dbQueue = GDBManager->GetDBQueue(dbQueueId);
    wcout << dbQueue->GetId() << L"번째 DBQueue가 작업을 시작함" << endl;

    while (dbQueue->IsStop() == false)
    {
        JobRef job = dbQueue->WaitForSingleJob();
        wcout << dbQueue->GetId() << L"번째 DBQueue가 작업을 받음" << endl;
        job->Execute();
    }
}

void DoWorkerJob(ServerServiceRef& service)
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + WORKER_TICK;

		// 네트워크 입출력 처리 -> 인게임 로직까지 (패킷 핸들러에 의해)
		service->GetIocpCore()->Dispatch(10);

		// 시간이 된 TimerJob 각 JQ에 밀어넣기(하나만 통과)
		ThreadManager::DistributeReservedJobs();

		// 글로벌 큐(떠넘겨진 JQ 들어있음)
		ThreadManager::DoGlobalQueueWork();
	}
}

int main(void)
{
    // Init
	ServerPacketHandler::Init();
    ASSERT_CRASH(Gamedata::LoadAllGamedata());

	const int maxSessionCount = 30;
	ServerServiceRef service = make_shared<ServerService>(
		NetAddress("127.0.0.1"s, 7777),
		make_shared<IocpCore>(),
		[=]() { return make_shared<GameSession>(); }, // TODO : SessionManager 등
		maxSessionCount
	);

	ASSERT_CRASH(service->Start());

    // DB 연결
    {
        // SQL Server
        int32 maxDBConnections = 1;
        const WCHAR* connectionString = ENV_DB_CONNECTION_STRING;
        ASSERT_CRASH(GDBConnectionPool->Connect(maxDBConnections, connectionString));

        // Redis
        ASSERT_CRASH(GRedisManager->Connect(ENV_REDIS_URI));
    }

	// worker thread
	const int workerThreadN = 5;
	for (int32 i = 0; i < workerThreadN; i++)
	{
		GThreadManager->Launch([&service]()
			{
				DoWorkerJob(service);
			});
	}

    // DB thread
    const int DBThreadN = 5;
    GDBManager->Init(DBThreadN);
    for (int32 i = 0; i < DBThreadN; i++)
    {
        GThreadManager->Launch([i]()
            {
                DoDBJob(i);
            });
    }

    // DB에서 서버 메모리에 올릴거 가져오기
    DBRequestFunctions::GetMaxItemUID();

    // Main Thread
    DoWorkerJob(service);

	GThreadManager->Join();

	return 0;
}