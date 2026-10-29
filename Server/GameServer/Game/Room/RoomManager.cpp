#include "Core/pch.h"
#include "Game/Room/RoomManager.h"
#include "Game/Room/Room.h"

bool RoomManager::CreateAllRooms(int32 channelCount)
{
    vector<RoomRef> rooms = MakeRooms(channelCount);
    if (rooms.empty())
    {
        GLogger->Error("룸을 하나도 만들지 못했다");
        return false;
    }

    for (const RoomRef& room : rooms)
    {
        if (room->Start() == false)
        {
            // 룸 데이터가 틀렸다는 뜻이다. 룸 하나가 빠진 채로 뜨면 그 채널로 가는 요청이 모두 깨진다.
            GLogger->Error("맵 {}의 {}채널 룸 생성에 실패했다", room->GetMapId(), room->GetChannel());
            return false;
        }

        AddRoom(room);
    }

    return true;
}

vector<RoomRef> RoomManager::MakeRooms(int32 channelCount)
{
    vector<RoomRef> rooms;

    // 룸 번호는 서버 안에서만 쓰는 번호라 만든 순서로 매긴다. DB와 클라이언트는 맵과 채널만 안다.
    int32 nextRoomId = 1;
    for (const auto& [mapId, map] : Gamedata::GetMaps())
    {
        // 기동할 때 맵마다 배치 데이터가 있는지 대조했으므로 없으면 서버의 결함이다.
        const MapPlacement* placement = Gamedata::FindMapPlacement(mapId);
        if (placement == nullptr)
        {
            GLogger->Error("맵 {}의 배치 데이터가 없어 룸을 만들지 못했다", mapId);
            return {};
        }

        for (int32 channel = 1; channel <= channelCount; channel++)
            rooms.push_back(Room::Create(nextRoomId++, channel, *placement));
    }

    return rooms;
}

RoomRef RoomManager::FindRoom(int32 roomId) const
{
    // 락이 없다. 부팅 뒤로 룸 목록을 바꾸지 않기 때문이다. 실행 중에 룸을 더하거나 빼려면 조회에 락부터 둬야 한다.
    auto it = _rooms.find(roomId);
    if (it == _rooms.end())
        return nullptr;

    return it->second;
}

RoomRef RoomManager::FindRoom(int32 mapId, int32 channel) const
{
    auto it = _channelRooms.find(mapId);
    if (it == _channelRooms.end() || channel < 1 || channel > static_cast<int32>(it->second.size()))
        return nullptr;

    return it->second[channel - 1];
}

vector<RoomRef> RoomManager::GetChannelRooms(int32 mapId) const
{
    auto it = _channelRooms.find(mapId);
    if (it == _channelRooms.end())
        return {};

    return it->second;
}

RoomRef RoomManager::SelectRoom(int32 mapId) const
{
    auto it = _channelRooms.find(mapId);
    if (it == _channelRooms.end())
        return nullptr;

    vector<int32> playerCounts;
    playerCounts.reserve(it->second.size());
    for (const RoomRef& room : it->second)
        playerCounts.push_back(room->GetPlayerCount());

    return FindRoom(mapId, RoomTransfer::SelectLeastPopulatedChannel(playerCounts));
}

vector<RoomRef> RoomManager::GetAllRooms() const
{
    vector<RoomRef> rooms;
    rooms.reserve(_rooms.size());
    for (const auto& [roomId, room] : _rooms)
        rooms.push_back(room);

    return rooms;
}

void RoomManager::AddRoom(const RoomRef& room)
{
    _rooms.emplace(room->GetRoomId(), room);
    _channelRooms[room->GetMapId()].push_back(room);
}
