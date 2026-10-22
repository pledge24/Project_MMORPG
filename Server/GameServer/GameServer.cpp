#include "Core/pch.h"
#include "ServerCore/Network/Service.h"
#include "ServerCore/Network/IocpCore.h"
#include "Core/Config.h"
#include "Core/DataCrossCheck.h"
#include "Lobby/Data/LobbyData.h"
#include "DB/IssuedIdDAO.h"
#include "DB/DAOCommon.h"
#include "DB/DBWorker.h"
#include "Core/ServerContext.h"
#include "Game/Room/Room.h"
#include "Utils/SnowflakeIdGenerator.h"

enum
{
	WORKER_TICK = 64
};

namespace
{
    // 접속 중인 세션이 모두 끊기기를 기다리는 상한(ms). 창 닫기 신호는 약 5초 뒤에 프로세스를 끝내므로 그 안에 저장까지 마친다.
    // 상대가 소켓을 닫지 않는 세션도 Session::DISCONNECT_TIMEOUT_MS(1초) 뒤에 끊기가 끝나므로 그보다 길게 둔다.
    constexpr uint64 SESSION_CLOSE_TIMEOUT_MS = 2000;
    // 룸 큐 하나가 비기를 기다리는 상한(ms).
    constexpr uint64 ROOM_DRAIN_TIMEOUT_MS = 1000;
    // 룸 큐를 비우는 횟수. 룸 이동 중에 끊긴 플레이어는 떠난 룸 큐가 들어갈 룸 큐로 넘긴 뒤에야 저장되므로 한 번 더 비운다.
    constexpr int32 ROOM_DRAIN_ROUNDS = 2;

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

            // 네트워크 입출력 처리. 패킷 핸들러가 여기서 돈다.
            service->GetIocpCore()->Dispatch(10);

            // 시간이 된 예약 잡을 각 잡 큐에 넣는다. 한 번에 한 스레드만 나눠 준다.
            ThreadManager::DistributeReservedJobs();

            // 다른 스레드가 떠넘긴 잡 큐 처리.
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

    // 접속 중인 플레이어를 저장하고 스레드를 모두 끝낸다. 저장은 평소의 접속 종료 경로를 그대로 탄다.
    void Shutdown(ServerServiceRef& service)
    {
        GLogger->Info("종료 신호를 받았습니다. 새 접속을 막고 접속 중인 플레이어를 저장합니다");

        // 새 접속을 막고 접속 중인 세션을 모두 끊는다.
        service->CloseService();
        service->DisconnectAllSessions("Server Shutdown");

        if (WaitUntil([&service]() { return service->GetCurrentSessionCount() == 0; }, SESSION_CLOSE_TIMEOUT_MS) == false)
            GLogger->Warning("세션 {}개가 끊기지 않아 그 플레이어는 저장하지 못할 수 있습니다", service->GetCurrentSessionCount());

        // 접속 종료가 룸 큐에 넣은 퇴장과 저장을 마저 돌린다.
        for (int32 round = 0; round < ROOM_DRAIN_ROUNDS; round++)
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

    // 패킷 핸들러 등록.
	ServerPacketHandler::Init();

    // 기획표가 틀리면 게임 중에 드러나지 않도록 여기서 멈춘다. 틀린 행은 LoadAllGamedata가 로그에 남긴다.
    if (Gamedata::LoadAllGamedata() == false)
    {
        GLogger->Error("기획 데이터가 틀려 서버를 종료합니다. 위 로그에서 원인을 확인하세요");
        return 1;
    }

    if (LobbyData::LoadAll() == false)
    {
        GLogger->Error("로비 기획 데이터가 틀려 서버를 종료합니다. 위 로그에서 원인을 확인하세요");
        return 1;
    }

    // 로비와 게임은 서로의 기획 데이터를 읽지 않는다. 둘이 맞아야 하는 값은 여기서 대조한다.
    if (optional<string> error = DataCrossCheck::Check())
    {
        GLogger->Error("로비와 게임의 기획 데이터가 맞지 않아 서버를 종료합니다: {}", error.value());
        return 1;
    }

    const Config config = Config::Load(&Config::ReadProcessEnv);
    if (optional<string> error = config.Validate())
    {
        GLogger->Error("설정이 틀려 서버를 종료합니다: {}", error.value());
        return 1;
    }

    // 노드 번호는 Validate가 확인했다.
    const int32 serverNodeId = config.serverNodeId.value();
    context.CreateIdGenerator(serverNodeId);

    // 룸은 여기서 한 번만 만든다. 워커 스레드가 뜬 뒤로는 룸 목록을 읽기만 한다.
    if (GRoomManager->CreateAllRooms() == false)
    {
        GLogger->Error("룸 생성에 실패해 서버를 종료합니다. 위 로그에서 원인을 확인하세요");
        return 1;
    }

    // 접속을 받기 전에 DB를 모두 준비한다. 리슨부터 열면 DB 큐가 생기기 전에 온 로그인이 없는 큐를 고르고,
    // 발급기의 시작점이 정해지기 전에 캐릭터나 아이템이 만들어질 수 있다.

    // DB 연결. 풀 크기가 DB 스레드 수보다 넉넉한지는 Config::Validate가 봤다.
    ASSERT_CRASH(GDBConnectionPool->Connect(config.dbConnectionCount, config.dbConnectionString.c_str()));
    ASSERT_CRASH(GRedisManager->Connect(config.redisUri));

    // DB에서 서버 메모리에 올릴 것을 가져온다. 스레드를 띄우기 전이라 실패하면 그대로 끝내도 된다.
    try
    {
        DBConnectionGuard conn;
        GIdGenerator->StartAfter(IssuedIdDAO::GetLastIssuedId(*conn, serverNodeId));
    }
    catch (const exception& error)
    {
        // 이 노드가 발급한 마지막 ID를 모르면, 시계가 뒤로 간 뒤 새 ID가 기존 캐릭터나 아이템과 겹칠 수 있다.
        GLogger->Error("이 노드가 발급한 ID의 최댓값을 읽지 못해 서버를 종료합니다: {}", error.what());
        return 1;
    }

    // DB 스레드 시작. 큐마다 DB 스레드가 하나씩 붙는다.
    GDBManager->Init(config.dbThreadCount);
    for (int32 i = 0; i < config.dbThreadCount; i++)
    {
        GThreadManager->Launch([i]()
            {
                DBWorker::Run(GDBManager->GetDBQueue(i));
            });
    }

	// 접속 받기 시작.
	ServerServiceRef service = make_shared<ServerService>(
		NetAddress(config.bindAddress, config.port),
		make_shared<IocpCore>(),
		[=]() { return make_shared<GameSession>(); },
		config.maxSessionCount
	);

	ASSERT_CRASH(service->Start())

	// 워커 스레드 시작. 메인 스레드가 마지막 하나로 합류한다.
	for (int32 i = 0; i < config.workerThreadCount; i++)
	{
		GThreadManager->Launch([&service]()
			{
				DoWorkerJob(service, s_workersStopped);
			});
	}

    // 메인 스레드도 워커로 일하다가 종료 신호를 받으면 빠져나와 종료 절차를 밟는다.
    DoWorkerJob(service, s_shutdownRequested);
    Shutdown(service);

    ::SetEvent(s_shutdownCompleted);

	return 0;
}
