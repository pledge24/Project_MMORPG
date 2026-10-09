#pragma once

USING_SHARED_PTR(Room);

/**
 * GameServer에 존재하는 모든 Room을 관리하는 Manager 클래스. 전역 객체 GRoomManager 하나만 존재한다.
 * 부팅 때 main이 CreateAllRooms로 모든 룸을 한 번 만들고, 그 뒤로는 조회만 한다. 등록된 룸이 바뀌지 않으므로
 * 조회에 락이 없고 어느 스레드에서 불러도 된다.
 */
class RoomManager
{
public:
    RoomManager() = default;
    ~RoomManager() = default;

    /**
     * 맵 표의 행마다 룸을 만들고 Start까지 불러 등록한다. 워커 스레드를 띄우기 전에 main이 한 번만 부른다.
     * 하나라도 실패하면 그 룸 번호를 로그에 남기고 false. 등록한 룸은 그대로 남지만 서버는 뜨지 않는다.
     */
    bool CreateAllRooms();

    /** 등록되지 않은 번호면 nullptr. */
    RoomRef FindRoom(int32 roomId) const;

private:
    unordered_map<int32, RoomRef> _rooms; // <RoomId, RoomRef>
};
