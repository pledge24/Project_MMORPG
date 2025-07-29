#include "pch.h"
#include <thread>
#include "Service.h"
#include "IocpCore.h"
#include "GameSession.h"
#include "ServerPacketHandler.h"
#include "DBConnectionPool.h"
#include "DBBind.h"
#include "DBQueue.h"
#include "DBManager.h"
#include "hiredis\hiredis.h"
#include "sw/redis++/redis++.h"

enum
{
	WORKER_TICK = 64
};

void DoDBJob(DBQueueRef dbQueue)
{
    while (dbQueue->isStop() == false)
    {
        JobRef job = dbQueue->WaitForSingleJob();
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
    //// Redis 서버에 연결
    //redisContext* c = redisConnect("127.0.0.1", 6379);
    //if (c == nullptr || c->err)
    //{
    //    if (c)
    //    {
    //        std::cout << "연결 오류: " << c->errstr << std::endl;
    //        redisFree(c);
    //    }
    //    else
    //    {
    //        std::cout << "Redis 컨텍스트를 할당할 수 없습니다" << std::endl;
    //    }
    //    return 1;
    //}

    //std::cout << "Redis에 연결되었습니다." << std::endl;

    //// 테스트용 키 몇 개 생성
    //redisCommand(c, "SET test:key1 value1");
    //redisCommand(c, "SET test:key2 value2");
    //redisCommand(c, "SET user:100 john");
    //redisCommand(c, "SET user:101 jane");

    //// KEYS * 명령 실행
    //redisReply* reply = (redisReply*)redisCommand(c, "KEYS *");

    //if (reply == nullptr)
    //{
    //    std::cout << "명령 실행 실패" << std::endl;
    //    redisFree(c);
    //    return 1;
    //}

    //// 응답 타입 확인
    //if (reply->type == REDIS_REPLY_ARRAY)
    //{
    //    std::cout << "총 " << reply->elements << "개의 키를 찾았습니다:" << std::endl;

    //    // 모든 키 출력
    //    for (size_t i = 0; i < reply->elements; i++)
    //    {
    //        if (reply->element[i]->type == REDIS_REPLY_STRING)
    //        {
    //            std::cout << "  [" << i + 1 << "] " << reply->element[i]->str << std::endl;
    //        }
    //    }
    //}
    //else
    //{
    //    std::cout << "예상치 못한 응답 타입: " << reply->type << std::endl;
    //}

    //// 메모리 해제
    //freeReplyObject(reply);

    //// 특정 패턴으로 키 검색 예제
    //std::cout << "\n'user:*' 패턴으로 검색:" << std::endl;
    //reply = (redisReply*)redisCommand(c, "KEYS user:*");

    //if (reply != nullptr && reply->type == REDIS_REPLY_ARRAY)
    //{
    //    std::cout << "찾은 키: " << reply->elements << "개" << std::endl;
    //    for (size_t i = 0; i < reply->elements; i++)
    //    {
    //        if (reply->element[i]->type == REDIS_REPLY_STRING)
    //        {
    //            std::cout << "  " << reply->element[i]->str << std::endl;
    //        }
    //    }
    //}

    //freeReplyObject(reply);

    //// 연결 종료
    //redisFree(c);
    //std::cout << "\nRedis 연결을 종료했습니다." << std::endl;

    //return 0;

    try
    {
        sw::redis::Redis redis("tcp://127.0.0.1:6379");
        redis.ping();
        std::wcout << L"Redis++ 연결 성공!" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::wcout << L"오류: " << e.what() << std::endl;
    }
    return 0;

    // DB thread
    const int DBThreadN = 5;
    GDBManager->Init(DBThreadN);
    for (int32 i = 0; i < DBThreadN; i++)
    {
        DBQueueRef dbQueue = GDBManager->GetDBQueue(i);
        
        GThreadManager->Launch([&dbQueue]()
            {
                DoDBJob(dbQueue);
            });
    }

    return 0;

    // ===================
    int32 maxDBConnections = 1;
    const WCHAR* connectionString = L"Driver={ODBC Driver 17 for SQL Server};Server=(localdb)\\ProjectModels;Database=GameDB;Trusted_Connection=Yes;";
    ASSERT_CRASH(GDBConnectionPool->Connect(maxDBConnections, connectionString));

    // Create Table
    {
        auto query = L"									\
			DROP TABLE IF EXISTS [dbo].[Gold];			\
			CREATE TABLE [dbo].[Gold]					\
			(											\
				[id] INT NOT NULL PRIMARY KEY IDENTITY, \
				[gold] INT NULL,						\
				[name] NVARCHAR(50) NULL,				\
				[createDate] DATETIME NULL				\
			);";

        DBConnection* dbConn = GDBConnectionPool->Pop();
        ASSERT_CRASH(dbConn->Execute(query));
        GDBConnectionPool->Push(dbConn);
    }

    // Add Data
    for (int32 i = 0; i < 3; i++)
    {
        DBConnection* dbConn = GDBConnectionPool->Pop();

        DBBind<3, 0> dbBind(*dbConn, L"INSERT INTO [dbo].[Gold]([gold], [name], [createDate]) VALUES(?, ?, ?)");

        int32 gold = 100;
        dbBind.BindParam(0, gold);
        WCHAR name[100] = L"루키스";
        dbBind.BindParam(1, name);
        TIMESTAMP_STRUCT ts = { 2021, 6, 5 };
        dbBind.BindParam(2, ts);

        ASSERT_CRASH(dbBind.Execute());

        /*
        // 기존에 바인딩 된 정보 날림
        dbConn->Unbind();

        // 넘길 인자 바인딩
        int32 gold = 100;
        SQLLEN len = 0;

        WCHAR name[100] = L"루키스";
        SQLLEN nameLen = 0;

        TIMESTAMP_STRUCT ts = {};
        ts.year = 2021;
        ts.month = 6;
        ts.day = 5;
        SQLLEN tsLen = 0;

        // 넘길 인자 바인딩
        ASSERT_CRASH(dbConn->BindParam(1, &gold, &len));
        ASSERT_CRASH(dbConn->BindParam(2, name, &nameLen));
        ASSERT_CRASH(dbConn->BindParam(3, &ts, &tsLen));

        // SQL 실행
        ASSERT_CRASH(dbConn->Execute(L"INSERT INTO [dbo].[Gold]([gold], [name], [createDate]) VALUES(?, ?, ?)"));
        */

        GDBConnectionPool->Push(dbConn);
    }

    // Read
    {
        DBConnection* dbConn = GDBConnectionPool->Pop();

        DBBind<1, 4> dbBind(*dbConn, L"SELECT id, gold, name, createDate FROM [dbo].[Gold] WHERE gold = (?)");

        int32 gold = 100;
        dbBind.BindParam(0, gold);

        int32 outId = 0;
        int32 outGold = 0;
        WCHAR outName[100];
        TIMESTAMP_STRUCT outDate = {};
        dbBind.BindCol(0, OUT outId);
        dbBind.BindCol(1, OUT outGold);
        dbBind.BindCol(2, OUT outName);
        dbBind.BindCol(3, OUT outDate);

        ASSERT_CRASH(dbBind.Execute());

        /*
        // 기존에 바인딩 된 정보 날림
        dbConn->Unbind();

        int32 gold = 100;
        SQLLEN len = 0;
        // 넘길 인자 바인딩
        ASSERT_CRASH(dbConn->BindParam(1, &gold, &len));

        int32 outId = 0;
        SQLLEN outIdLen = 0;
        ASSERT_CRASH(dbConn->BindCol(1, &outId, &outIdLen));

        int32 outGold = 0;
        SQLLEN outGoldLen = 0;
        ASSERT_CRASH(dbConn->BindCol(2, &outGold, &outGoldLen));

        WCHAR outName[100];
        SQLLEN outNameLen = 0;
        ASSERT_CRASH(dbConn->BindCol(3, outName, len32(outName), &outNameLen));

        TIMESTAMP_STRUCT outDate = {};
        SQLLEN outDateLen = 0;
        ASSERT_CRASH(dbConn->BindCol(4, &outDate, &outDateLen));

        // SQL 실행
        ASSERT_CRASH(dbConn->Execute(L"SELECT id, gold, name, createDate FROM [dbo].[Gold] WHERE gold = (?)"));
        */


        wcout.imbue(locale("kor"));

        while (dbConn->Fetch())
        {
            wcout << "Id: " << outId << " Gold : " << outGold << " Name: " << outName << endl;
            wcout << "Date : " << outDate.year << "/" << outDate.month << "/" << outDate.day << endl;
        }

        GDBConnectionPool->Push(dbConn);
    }

    return 0;
    //========================================
	ServerPacketHandler::Init();

	const int maxSessionCount = 30;
	ServerServiceRef service = make_shared<ServerService>(
		NetAddress("127.0.0.1"s, 7777),
		make_shared<IocpCore>(),
		[=]() { return make_shared<GameSession>(); }, // TODO : SessionManager 등
		maxSessionCount
	);

	ASSERT_CRASH(service->Start());

	// worker thread
	const int workerThreadN = 5;
	for (int32 i = 0; i < workerThreadN; i++)
	{
		GThreadManager->Launch([&service]()
			{
				DoWorkerJob(service);
			});
	}

	// Main Thread
	DoWorkerJob(service);

	GThreadManager->Join();

	return 0;
}