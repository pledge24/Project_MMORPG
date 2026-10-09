#pragma once

USING_SHARED_PTR(Room);

/**
 * GameServer에 존재하는 모든 Room을 관리하는 Manager 클래스. 전역 객체 GRoomManager 하나만 존재한다.
 * GameServer 시작 시 모든 Room을 생성하고, 그 뒤로는 주로 Room 조회 용도로 사용된다. 
 */
class RoomManager
{
public:
    RoomManager();
    ~RoomManager();

    /** 맵 데이터로 룸을 만들고 Start까지 부른다. 등록은 하지 않는다. 데이터가 없거나 시작에 실패하면 nullptr. */
    RoomRef CreateRoom(int32 templateId);
    /** 이미 같은 번호가 있으면 아무것도 하지 않는다. 등록한 룸은 IsValid가 true가 된다. */
    void AddRoom(int32 templateId, RoomRef room);
    void RemoveRoom(int32 templateId);
    void Clear();

    /** 등록되지 않은 번호면 nullptr. */
    RoomRef GetRoomRefFromRoomId(int32 templateId);

private:
    unordered_map<int32, RoomRef> _rooms; // <RoomId, RoomRef>
};

