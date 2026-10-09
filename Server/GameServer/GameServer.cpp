#include "Core/pch.h"
#include "ServerCore/Network/Service.h"
#include "ServerCore/Network/IocpCore.h"
#include "Core/Config.h"
#include "DB/ItemDAO.h"

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

    // 기획표가 틀리면 게임 중에 드러나지 않도록 여기서 멈춘다. 틀린 행은 LoadAllGamedata가 로그에 남긴다.
    if (Gamedata::LoadAllGamedata() == false)
    {
        GLogger->Error("기획 데이터가 틀려 서버를 종료합니다. 위 로그에서 원인을 확인하세요");
        return 1;
    }

    const Config config = Config::Load(&Config::ReadProcessEnv);

    // Room 추가
    for (const auto& [roomId, mapTemplate] : Gamedata::GetMaps())
    {
        RoomRef room = GRoomManager->CreateRoom(roomId);
        if (room == nullptr)
        {
            // 룸 데이터가 틀렸다는 뜻이다. 룸 하나가 빠진 채로 뜨면 그 룸으로 가는 요청이 모두 깨진다.
            GLogger->Error("Room {} 생성에 실패해 서버를 종료합니다. 위 로그에서 원인을 확인하세요", roomId);
            return 1;
        }

        GRoomManager->AddRoom(roomId, room);
    }

	const int maxSessionCount = 30;
	ServerServiceRef service = make_shared<ServerService>(
		NetAddress("127.0.0.1"s, config.port),
		make_shared<IocpCore>(),
		[=]() { return make_shared<GameSession>(); }, // TODO: SessionManager 등
		maxSessionCount
	);

	ASSERT_CRASH(service->Start())

    // DB 연결
    {
        // SQL Server
        int32 maxDBConnections = 1;
        ASSERT_CRASH(GDBConnectionPool->Connect(maxDBConnections, config.dbConnectionString.c_str()));

        // Redis
        ASSERT_CRASH(GRedisManager->Connect(config.redisUri));
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
    ItemDAO::GetMaxItemUID();

    // Main Thread
    DoWorkerJob(service);

	GThreadManager->Join();

	return 0;
}