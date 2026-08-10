#pragma once

/*----------------
    RoomManager
-----------------*/

USING_SHARED_PTR(Room);

class RoomManager
{
public:
    RoomManager();
    ~RoomManager();

    /** Room 관리 함수*/
    RoomRef CreateRoom(int32 templateId);
    void AddRoom(int32 templateId, RoomRef room);
    void RemoveRoom(int32 templateId);
    void Clear();

    /** Room 조회 함수*/
    RoomRef GetRoomRefFromRoomId(int32 templateId);

private:
    unordered_map<int32, RoomRef> _rooms; // <RoomId, RoomRef>
};

