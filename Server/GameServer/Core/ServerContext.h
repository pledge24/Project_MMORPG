#pragma once

class RoomManager;
class GameSessionManager;
class SaveGate;
class ProgressCoordinator;
class SnowflakeIdGenerator;

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

    /**
     * ID 발급기를 만들어 GIdGenerator에 건다. 노드 번호는 설정에서 오므로 설정 검사를 통과한 뒤에 한 번만 부른다.
     * nodeId는 검증하지 않는다.
     */
    void CreateIdGenerator(int32 nodeId);

private:
    /** 선언 순서가 생성 순서다. 게임 서버 전역이 모두 사라진 뒤에 사라지도록 맨 앞에 둔다. */
    CoreGlobal _core;
    unique_ptr<RoomManager> _roomManager;
    unique_ptr<SnowflakeIdGenerator> _idGenerator;
    JobQueueRef _sessionJobQueue;
    unique_ptr<GameSessionManager> _sessionManager;
    unique_ptr<SaveGate> _saveGate;
    unique_ptr<ProgressCoordinator> _progressCoordinator;
};
