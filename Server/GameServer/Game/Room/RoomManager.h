#pragma once

USING_SHARED_PTR(Room);

/**
 * 게임 서버의 모든 룸을 가진 관리자.
 *
 * 서버가 뜰 때 룸을 모두 만들어 두고, 룸을 번호로 찾아 주는 창구가 된다.
 */
class RoomManager
{
public:
    RoomManager() = default;
    ~RoomManager() = default;

    /**
     * 맵 표의 행마다 룸을 만들어 시작하고 등록한다. 워커 스레드를 띄우기 전에 한 번만 부른다.
     * 하나라도 실패하면 false이고, 그때는 서버를 띄우지 않는다.
     */
    bool CreateAllRooms();

    /** 어느 스레드에서 불러도 된다. 등록되지 않은 번호면 nullptr. */
    RoomRef FindRoom(int32 roomId) const;
    /** 등록된 룸 전부. 서버 종료 절차가 룸 큐를 비울 때 쓴다. */
    vector<RoomRef> GetAllRooms() const;

private:
    unordered_map<int32, RoomRef> _rooms; // <RoomId, RoomRef>
};
