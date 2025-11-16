#include "pch.h"
#include "RoomManager.h"
#include "Room.h"

/*----------------
    RoomManager
-----------------*/

RoomManager::RoomManager()
{
}

RoomManager::~RoomManager()
{
    Clear();
}

RoomRef RoomManager::CreateRoom(int32 templateId)
{
    RoomRef room = nullptr;
    try
    {
        if (Gamedata::MapDataTable.find(templateId) == Gamedata::MapDataTable.end())
            throw wstring(L"Gamedata에 해당 room에 대한 정보가 없음");

        const Json& roomData = Gamedata::MapDataTable[templateId];
        room = make_shared<Room>();
        room->Init(roomData);
    }
    catch (const wstring cause)
    {
        wcout << L"CreateRoom 중 문제 발생: " << cause << endl;
        return nullptr;
    }

    return room;
}

void RoomManager::AddRoom(int32 templateId, RoomRef room)
{
    if (_rooms.find(templateId) != _rooms.end())
        return;

    room->SetValid(true);
    _rooms.insert(make_pair(templateId, room));
}

void RoomManager::RemoveRoom(int32 templateId)
{
    if (_rooms.find(templateId) == _rooms.end())
        return;

    RoomRef room = _rooms[templateId];
    room->SetValid(false);

    _rooms.erase(templateId);
}

void RoomManager::Clear()
{
    for (auto pair : _rooms)
    {
        RoomRef room = pair.second;
        room->SetValid(false);
    }

    _rooms.clear();
}

RoomRef RoomManager::GetRoomRefFromRoomId(int32 templateId)
{
    if (_rooms.contains(templateId) == false)
        return nullptr;

    return _rooms[templateId];
}
