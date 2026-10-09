#include "Core/pch.h"
#include "ServerCore/Network/Service.h"
#include "ServerCore/Network/IocpCore.h"
#include "Core/Config.h"
#include "DB/ItemDAO.h"
#include "DB/DAOCommon.h"
#include "DB/DBWorker.h"
#include "Core/ServerContext.h"
#include "Game/Room/Room.h"

enum
{
	WORKER_TICK = 64
};

namespace
{
    // 접속 중인 세션이 모두 끊기기를 기다리는 상한(ms). 창 닫기 신호는 약 5초 뒤에 프로세스를 끝내므로 그 안에 저장까지 마친다.
    constexpr uint64 SESSION_CLOSE_TIMEOUT_MS = 2000;
    // 룸 큐 하나가 비기를 기다리는 상한(ms).
    constexpr uint64 ROOM_DRAIN_TIMEOUT_MS = 1000;

    // 콘솔 종료 신호를 받으면 세운다. 메인 스레드가 워커 루프를 빠져나와 종료 절차를 밟는다.
    atomic<bool> s_shutdownRequested = false;
    // 종료 절차의 마지막에 세운다. 나머지 워커 스레드가 루프를 빠져나온다.
    atomic<bool> s_workersStopped = false;
    // 종료 절차가 끝나면 신호를 준다. 콘솔 처리기가 이것을 기다린다.
    HANDLE s_shutdownCompleted = nullptr;

    // 콘솔 처리기는 시스템이 만든 별도 스레드에서 돈다. 창 닫기와 로그오프, 시스템 종료는 처리기가 돌아가는 즉시
    // 프로세스를 끝내므로, 종료 절차가 끝날 때까지 돌아가지 않는다.
    BOOL WINAPI HandleConsoleCtrl(DWORD ctrlType)
    {
        switch (ctrlType)
        {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            s_shutdownRequested.store(true);
            ::WaitForSingleObject(s_shutdownCompleted, INFINITE);
            return TRUE;
        default:
            return FALSE;
        }
    }

    // stop이 설 때까지 IOCP 처리, 예약 잡 분배, 글로벌 큐 소비를 반복한다.
    void DoWorkerJob(ServerServiceRef& service, const atomic<bool>& stop)
    {
        while (stop.load() == false)
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

    // done이 참이 될 때까지 기다린다. 상한을 넘기면 false. 그동안 나머지 워커가 IOCP와 잡을 처리한다.
    template<typename Predicate>
    bool WaitUntil(Predicate done, uint64 timeoutMs)
    {
        const uint64 deadline = ::GetTickCount64() + timeoutMs;
        while (done() == false)
        {
            if (::GetTickCount64() >= deadline)
                return false;

            this_thread::sleep_for(10ms);
        }
        return true;
    }

    // 지금 룸 큐마다 들어 있는 잡이 모두 돌 때까지 기다린다. 룸 큐는 넣은 순서대로 돌므로 끝에 넣은 잡이 돌면 앞의 잡도 돌았다.
    bool DrainRoomQueues()
    {
        vector<RoomRef> rooms = GRoomManager->GetAllRooms();
        auto remaining = make_shared<atomic<int32>>(static_cast<int32>(rooms.size()));

        for (const RoomRef& room : rooms)
            room->DoAsync([remaining]() { remaining->fetch_sub(1); });

        return WaitUntil([&remaining]() { return remaining->load() == 0; }, ROOM_DRAIN_TIMEOUT_MS);
    }

    // 새 접속을 막고, 접속 중인 세션을 모두 끊어 접속 종료 저장을 돌리고, DB 큐가 빌 때까지 기다린 뒤 스레드를 모두 끝낸다.
    // 저장은 평소의 접속 종료 경로(ProgressCoordinator)를 그대로 탄다.
    void Shutdown(ServerServiceRef& service)
    {
        GLogger->Info("종료 신호를 받았습니다. 새 접속을 막고 접속 중인 플레이어를 저장합니다");

        service->CloseService();
        service->DisconnectAllSessions("Server Shutdown");

        if (WaitUntil([&service]() { return service->GetCurrentSessionCount() == 0; }, SESSION_CLOSE_TIMEOUT_MS) == false)
            GLogger->Warning("세션 {}개가 끊기지 않아 그 플레이어는 저장하지 못할 수 있습니다", service->GetCurrentSessionCount());

        // 접속 종료가 룸 큐에 넣은 퇴장과 저장을 마저 돌린다. 룸 이동 중에 끊긴 플레이어는 첫 바퀴에서 떠난 룸이
        // 들어갈 룸 큐로 넘기므로 두 바퀴 돌린다.
        for (int32 round = 0; round < 2; round++)
        {
            if (DrainRoomQueues() == false)
                GLogger->Warning("룸 큐가 비지 않아 일부 플레이어는 저장하지 못할 수 있습니다");
        }

        // 멈춘 DB 큐는 남은 잡을 다 돌린 뒤에 DB 스레드를 끝낸다. 그래서 접속 종료 저장이 모두 끝난다.
        for (int32 i = 0; i < GDBManager->GetDBQueueCount(); i++)
            GDBManager->GetDBQueue(i)->Stop();

        s_workersStopped.store(true);
        GThreadManager->Join();

        GLogger->Info("접속 중이던 플레이어를 저장하고 서버를 종료합니다");
    }
}

int main(void)
{
    // 전역 객체를 여기서 만들고 main이 끝날 때 역순으로 지운다. 아래 모든 코드가 이 객체에 기댄다.
    ServerContext context;

    // 종료 신호를 받으면 접속 중인 플레이어를 저장한 뒤 끝낸다. 처리기가 기다릴 신호부터 만든다.
    s_shutdownCompleted = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    ASSERT_CRASH(s_shutdownCompleted != nullptr);
    ASSERT_CRASH(::SetConsoleCtrlHandler(HandleConsoleCtrl, TRUE));

    // Init
	ServerPacketHandler::Init();

    // 기획표가 틀리면 게임 중에 드러나지 않도록 여기서 멈춘다. 틀린 행은 LoadAllGamedata가 로그에 남긴다.
    if (Gamedata::LoadAllGamedata() == false)
    {
        GLogger->Error("기획 데이터가 틀려 서버를 종료합니다. 위 로그에서 원인을 확인하세요");
        return 1;
    }

    const Config config = Config::Load(&Config::ReadProcessEnv);

    // 룸은 여기서 한 번만 만든다. 워커 스레드가 뜬 뒤로는 룸 목록을 읽기만 한다.
    if (GRoomManager->CreateAllRooms() == false)
    {
        GLogger->Error("룸 생성에 실패해 서버를 종료합니다. 위 로그에서 원인을 확인하세요");
        return 1;
    }

	const int maxSessionCount = 30;
	ServerServiceRef service = make_shared<ServerService>(
		NetAddress("127.0.0.1"s, config.port),
		make_shared<IocpCore>(),
		[=]() { return make_shared<GameSession>(); }, // TODO: SessionManager 등
		maxSessionCount
	);

	ASSERT_CRASH(service->Start())

    // DB 스레드 수. 큐마다 DB 스레드가 하나씩 붙는다.
    constexpr int32 DB_THREAD_COUNT = 5;

    // DB 연결
    {
        // SQL Server. DB 스레드마다 하나, 시작할 때 main이 아이템 UID를 읽는 데 하나를 쓴다.
        // 모자라면 동시에 돈 잡 하나가 연결을 빌리지 못한다.
        const int32 dbConnectionCount = DB_THREAD_COUNT + 1;
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
				DoWorkerJob(service, s_workersStopped);
			});
	}

    // DB thread
    GDBManager->Init(DB_THREAD_COUNT);
    for (int32 i = 0; i < DB_THREAD_COUNT; i++)
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

        // 띄운 스레드를 끝내야 ServerContext가 합류하고 지운다. 아직 아무도 접속하지 않았으므로 저장할 것이 없다.
        for (int32 i = 0; i < GDBManager->GetDBQueueCount(); i++)
            GDBManager->GetDBQueue(i)->Stop();
        s_workersStopped.store(true);
        return 1;
    }

    // 메인 스레드도 워커로 일하다가 종료 신호를 받으면 빠져나와 종료 절차를 밟는다.
    DoWorkerJob(service, s_shutdownRequested);
    Shutdown(service);

    ::SetEvent(s_shutdownCompleted);

	return 0;
}
