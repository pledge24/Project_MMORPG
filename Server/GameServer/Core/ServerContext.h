#pragma once

class RoomManager;
class GameSessionManager;
class SaveGate;
class ProgressCoordinator;

/**
 * 게임 서버의 전역 객체를 만들고 지운다. GameServer와 GameServerTests의 main이 맨 처음에 지역 변수로 하나 만든다.
 * ServerCore의 전역(CoreGlobal)을 먼저 만들고 게임 서버의 전역을 차례로 만든다. 지울 때는 남은 스레드를 합류시킨 뒤 역순으로 지운다.
 * 정적 객체로 두지 않는다. 번역 단위가 다른 정적 객체 사이에서는 만들고 지우는 순서가 정해지지 않는다.
 */
class ServerContext
{
public:
    ServerContext();
    ~ServerContext();

    ServerContext(const ServerContext&) = delete;
    ServerContext& operator=(const ServerContext&) = delete;

private:
    // 선언 순서가 생성 순서다. 멤버는 선언의 역순으로 소멸하므로 ServerCore 전역이 가장 나중에 사라진다.
    CoreGlobal _core;
    unique_ptr<RoomManager> _roomManager;
    JobQueueRef _sessionJobQueue;
    unique_ptr<GameSessionManager> _sessionManager;
    unique_ptr<SaveGate> _saveGate;
    unique_ptr<ProgressCoordinator> _progressCoordinator;
};
