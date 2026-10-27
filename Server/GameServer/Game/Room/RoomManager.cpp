#include "Core/pch.h"
#include "Game/Room/RoomManager.h"
#include "Game/Room/Room.h"

bool RoomManager::CreateAllRooms()
{
    for (const auto& [areaId, area] : Gamedata::GetAreas())
    {
        RoomRef room = Room::Create(area);
        const int32 roomId = room->GetRoomId();
        if (room->Start() == false)
        {
            // 룸 데이터가 틀렸다는 뜻이다. 룸 하나가 빠진 채로 뜨면 그 룸으로 가는 요청이 모두 깨진다.
            GLogger->Error("Room {} 생성에 실패했다", roomId);
            return false;
        }

        _rooms.emplace(roomId, room);
    }

    return true;
}

RoomRef RoomManager::FindRoom(int32 roomId) const
{
    // 락이 없다. 부팅 뒤로 _rooms를 바꾸지 않기 때문이다. 실행 중에 룸을 더하거나 빼려면 조회에 락부터 둬야 한다.
    auto it = _rooms.find(roomId);
    if (it == _rooms.end())
        return nullptr;

    return it->second;
}

vector<RoomRef> RoomManager::GetAllRooms() const
{
    vector<RoomRef> rooms;
    rooms.reserve(_rooms.size());
    for (const auto& [roomId, room] : _rooms)
        rooms.push_back(room);

    return rooms;
}
