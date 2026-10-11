#include "Core/pch.h"
#include "Core/ServerContext.h"
#include "Network/GameSessionManager.h"
#include "Network/SaveGate.h"
#include "Network/ProgressCoordinator.h"

ServerContext::ServerContext()
{
    _roomManager = make_unique<RoomManager>();
    GRoomManager = _roomManager.get();

    _sessionJobQueue = make_shared<JobQueue>();
    GSessionJobQueue = _sessionJobQueue;

    _sessionManager = make_unique<GameSessionManager>();
    GSessionManager = _sessionManager.get();

    _saveGate = make_unique<SaveGate>();
    _progressCoordinator = ProgressCoordinator::CreateForServer(*_saveGate);
    GProgressCoordinator = _progressCoordinator.get();
}

ServerContext::~ServerContext()
{
    // 아래 객체를 쓰는 워커와 DB 스레드가 남아 있으면 지우는 도중에 읽는다.
    GThreadManager->Join();

    // 전역 포인터를 먼저 비우고 객체를 지운다. 지우는 중에 불린 코드가 지운 객체를 읽지 않게 한다.
    GProgressCoordinator = nullptr;
    _progressCoordinator.reset();
    _saveGate.reset();

    GSessionManager = nullptr;
    _sessionManager.reset();

    GSessionJobQueue = nullptr;
    _sessionJobQueue.reset();

    GRoomManager = nullptr;
    _roomManager.reset();
}
