#include "Core/pch.h"
#include "Game/Room/RoomManager.h"
#include "Game/Room/Room.h"

RoomManager::RoomManager()
{
}

RoomManager::~RoomManager()
{
    Clear();
}

RoomRef RoomManager::CreateRoom(int32 templateId)
{
    const MapTemplate* mapTemplate = Gamedata::FindMap(templateId);
    if (mapTemplate == nullptr)
    {
        GLogger->Error("맵 표에 룸 {}이 없다", templateId);
        return nullptr;
    }

    RoomRef room = Room::Create(*mapTemplate);
    
    if (room == nullptr)
        return nullptr;

    if (room->Start() == false)
        return nullptr;

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
