#include "pch.h"
#include <thread>
#include "Service.h"
#include "IocpCore.h"
#include "GameSession.h"
#include "ServerPacketHandler.h"
#include "DBConnectionPool.h"
#include "DBBind.h"

enum
{
	WORKER_TICK = 64
};

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