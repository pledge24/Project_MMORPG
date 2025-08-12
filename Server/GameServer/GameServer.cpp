#include "pch.h"
#include <thread>
#include "Service.h"
#include "IocpCore.h"
#include "GameSession.h"
#include "config.h"
#include "EncodingConverter.h"

#include <fstream>

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
    // 방법 2: json::parse()를 사용한 방법
    //try
    //{
    //    std::ifstream file2("Quest.json");
    //    if (file2.is_open())
    //    {
    //        Json j2 = Json::parse(file2);
    //        wcout << L"방법 2로 읽은 JSON: " << EncodingConverter::StringToWString(j2.dump(4)) << endl;
    //    }
    //}
    //catch (const std::exception& e)
    //{
    //    std::cerr << "방법 2 오류: " << e.what() << std::endl;
    //}

    //return 0;
	ServerPacketHandler::Init();

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

  //  // Create Table
  //  {
  //      auto query = L"									\
		//DROP TABLE IF EXISTS [dbo].[Gold];			\
		//CREATE TABLE [dbo].[Gold]					\
		//(											\
		//	[id] INT NOT NULL PRIMARY KEY IDENTITY, \
		//	[gold] INT NULL,						\
		//	[name] NVARCHAR(50) NULL,				\
		//	[createDate] DATETIME NULL				\
		//);";

  //      DBConnection* dbConn = GDBConnectionPool->Pop();
  //      ASSERT_CRASH(dbConn->Execute(query));
  //      GDBConnectionPool->Push(dbConn);
  //  }

  //  // Add Data
  //  for (int32 i = 0; i < 3; i++)
  //  {
  //      DBConnection* dbConn = GDBConnectionPool->Pop();

  //      DBBind<3, 0> dbBind(*dbConn, L"INSERT INTO [dbo].[Gold]([gold], [name], [createDate]) VALUES(?, ?, ?)");

  //      int32 gold = 100;
  //      dbBind.BindParam(0, gold);
  //      WCHAR name[100] = L"루키스";
  //      dbBind.BindParam(1, name);
  //      TIMESTAMP_STRUCT ts = { 2021, 6, 5 };
  //      dbBind.BindParam(2, ts);

  //      ASSERT_CRASH(dbBind.Execute());

  //      GDBConnectionPool->Push(dbConn);
  //  }

 /*   return 0;*/

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

    //for (int32 i = 0; i < 100; i++)
    //{
    //    int32 dbQueueCount = GDBManager->GetDBQueueCount();
    //    int32 queueId = Utils::GetRandom(0, dbQueueCount);
    //    DBQueueRef dbQueue = GDBManager->GetDBQueue(queueId);

    //    JobRef job = make_shared<Job>(
    //        []()
    //        {
    //            cout << "Handle_C_Login!" << endl;
    //        }
    //    );

    //    dbQueue->Push(std::move(job));
    //}

    //// Main Thread
    //DoWorkerJob(service);

	GThreadManager->Join();

	return 0;
}