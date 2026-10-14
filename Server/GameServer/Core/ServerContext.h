#pragma once

class RoomManager;
class GameSessionManager;
class SaveGate;
class ProgressCoordinator;

/**
 * 게임 서버 전역 객체의 수명 관리자.
 *
 * 서버가 쓰는 전역 객체를 정해진 순서로 만들고, 끝날 때 그 역순으로 지운다.
 * main이 맨 처음에 지역 변수로 하나 만든다. 정적 객체로 두면 다른 정적 객체와의 생성·소멸 순서가 정해지지 않는다.
 */
class ServerContext
{
public:
    ServerContext();
    ~ServerContext();

    ServerContext(const ServerContext&) = delete;
    ServerContext& operator=(const ServerContext&) = delete;

private:
    /** 선언 순서가 생성 순서다. 게임 서버 전역이 모두 사라진 뒤에 사라지도록 맨 앞에 둔다. */
    CoreGlobal _core;
    unique_ptr<RoomManager> _roomManager;
    JobQueueRef _sessionJobQueue;
    unique_ptr<GameSessionManager> _sessionManager;
    unique_ptr<SaveGate> _saveGate;
    unique_ptr<ProgressCoordinator> _progressCoordinator;
};
