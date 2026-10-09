#include "Core/pch.h"
#include "ServerCore/Network/Service.h"
#include "ServerCore/Network/IocpCore.h"
#include "Core/Config.h"
#include "DB/ItemDAO.h"
#include "DB/DAOCommon.h"
#include "DB/DBWorker.h"

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

    // DB thread 수. 큐마다 DB 스레드가 하나씩 붙는다.
    const int DBThreadN = 5;

    // DB 연결
    {
        // SQL Server. DB 스레드마다 하나, 시작할 때 main이 아이템 UID를 읽는 데 하나를 쓴다.
        // 모자라면 동시에 돈 잡 하나가 연결을 빌리지 못한다.
        const int32 dbConnectionCount = DBThreadN + 1;
        ASSERT_CRASH(GDBConnectionPool->Connect(dbConnectionCount, config.dbConnectionString.c_str()));

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
    GDBManager->Init(DBThreadN);
    for (int32 i = 0; i < DBThreadN; i++)
    {
        GThreadManager->Launch([i]()
            {
                DBWorker::Run(GDBManager->GetDBQueue(i));
            });
    }

    // DB에서 서버 메모리에 올릴거 가져오기
    try
    {
        DBConnectionGuard conn;
        ItemDAO::GetMaxItemUID(*conn);
    }
    catch (const DBError& error)
    {
        // 다음 아이템 UID를 모르면 새 아이템이 기존 아이템과 UID가 겹친다.
        GLogger->Error("아이템 UID의 최댓값을 읽지 못해 서버를 종료합니다: {}", error.what());
        return 1;
    }

    // Main Thread
    DoWorkerJob(service);

	GThreadManager->Join();

	return 0;
}