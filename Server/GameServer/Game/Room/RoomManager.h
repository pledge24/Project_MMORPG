#pragma once

USING_SHARED_PTR(Room);

/**
 * 룸 번호(맵 데이터의 템플릿 번호)로 룸을 찾는 목록. 전역 객체 GRoomManager 하나만 있다.
 * 목록이 룸의 shared_ptr를 붙잡으므로 등록된 룸은 프로세스가 끝날 때까지 산다.
 * 락이 없다. 서버 시작 때 main 스레드가 모두 등록하고, 그 뒤로는 여러 스레드가 조회만 한다.
 */
class RoomManager
{
public:
    RoomManager();
    ~RoomManager();

    //~ 룸 관리
    /** 맵 데이터로 룸을 만들고 Start까지 부른다. 등록은 하지 않는다. 데이터가 없거나 시작에 실패하면 nullptr. */
    RoomRef CreateRoom(int32 templateId);
    /** 이미 같은 번호가 있으면 아무것도 하지 않는다. 등록한 룸은 IsValid가 true가 된다. */
    void AddRoom(int32 templateId, RoomRef room);
    void RemoveRoom(int32 templateId);
    void Clear();

    //~ 룸 조회
    /** 등록되지 않은 번호면 nullptr. */
    RoomRef GetRoomRefFromRoomId(int32 templateId);

private:
    unordered_map<int32, RoomRef> _rooms; // <RoomId, RoomRef>
};

