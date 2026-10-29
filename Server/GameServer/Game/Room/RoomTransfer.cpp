#include "Core/pch.h"
#include "Game/Room/RoomTransfer.h"

namespace
{
    // 그 맵에서 좌표가 드는 지역을 찾는다. 높이는 보지 않는다.
    // 경계 위의 점은 안쪽이다. 추출이 지역끼리 겹치지 않음을 검사하지만 경계는 함께 쓸 수 있고, 그때는
    // 지역 표를 지역 번호 순서로 돌기 때문에 번호가 작은 쪽이 잡힌다.
    const DesignData::AreaRow* FindAreaContaining(int32 mapId, const Protocol::Vector& pos)
    {
        for (const auto& [areaId, area] : Gamedata::GetAreas())
        {
            if (area.MapId != mapId)
                continue;

            const AreaPlacement* placement = Gamedata::FindAreaPlacement(areaId);
            if (placement != nullptr && placement->bounds.Contains(pos.x(), pos.y()))
                return &area;
        }

        return nullptr;
    }

    Protocol::PosInfo MakeIdlePos(const vector3D& pos, float yaw)
    {
        Protocol::PosInfo posInfo;
        posInfo.mutable_pos()->set_x(pos.x);
        posInfo.mutable_pos()->set_y(pos.y);
        posInfo.mutable_pos()->set_z(pos.z);
        posInfo.set_yaw(yaw);
        posInfo.set_state(Protocol::MoveState::MOVE_STATE_IDLE);
        return posInfo;
    }
}

optional<string> RoomTransfer::ValidatePortalUse(const PortalPlacement& portal, const Protocol::PosInfo& playerPos)
{
    // 높이는 보지 않는다. 포털 좌표의 z는 바닥이고 플레이어의 z는 캡슐 중심이다.
    const float dx = playerPos.pos().x() - portal.src.x;
    const float dy = playerPos.pos().y() - portal.src.y;
    if (dx * dx + dy * dy > portal.radius * portal.radius)
        return format("포털 {}의 반경 {} 밖에서 온 포털 이동 요청", portal.id, portal.radius);

    return nullopt;
}

optional<MapPosition> RoomTransfer::FindPortalDestination(const DesignData::PortalRow& portal, const PortalPlacement& placement)
{
    const DesignData::AreaRow* destArea = Gamedata::FindArea(portal.DestAreaId);
    if (destArea == nullptr)
        return nullopt;

    return MapPosition{ destArea->MapId, MakeIdlePos(placement.dest, placement.destYaw) };
}

optional<Protocol::EntryResult> RoomTransfer::ValidateRespawn(bool isDead, Protocol::RespawnType respawnType)
{
    if (isDead == false)
        return Protocol::ENTRY_RESULT_NOT_DEAD;

    // 다른 유형은 목적지 규칙이 아직 없다. 지원하는 날 그 유형의 목적지와 함께 연다.
    if (respawnType != Protocol::RESPAWN_TYPE_TOWN)
        return Protocol::ENTRY_RESULT_UNSUPPORTED_RESPAWN_TYPE;

    return nullopt;
}

optional<MapPosition> RoomTransfer::FindTownRespawn()
{
    const DesignData::AreaRow* town = Gamedata::FindArea(Gamedata::GetTownAreaId());
    if (town == nullptr || town->HasRespawnPoint == false)
        return nullopt;

    const RespawnPointPlacement* point = Gamedata::FindRespawnPoint(town->Id);
    if (point == nullptr)
        return nullopt;

    return MapPosition{ town->MapId, MakeIdlePos(point->pos, point->yaw) };
}

optional<MapPosition> RoomTransfer::ResolveEnterPosition(int32 mapId, const Protocol::PosInfo& pos)
{
    // 지역 사이의 빈 공간에는 바닥이 없다. 저장된 좌표가 어느 지역에도 들지 않으면 그 자리에 세우지 않는다.
    if (const DesignData::AreaRow* area = FindAreaContaining(mapId, pos.pos()))
        return MapPosition{ area->MapId, pos };

    // 어느 지역에도 들지 않는 좌표는 마을 리스폰 지점에서 시작한다. 레벨이 바뀌어 저장된 좌표가 지역 밖이 된 경우다.
    return FindTownRespawn();
}

int32 RoomTransfer::SelectLeastPopulatedChannel(const vector<int32>& playerCounts)
{
    if (playerCounts.empty())
        return 0;

    // min_element는 같은 값이면 앞의 것을 고른다. 인원이 같으면 1채널부터 채운다.
    const auto least = std::min_element(playerCounts.begin(), playerCounts.end());
    return static_cast<int32>(least - playerCounts.begin()) + 1;
}

optional<Protocol::EntryResult> RoomTransfer::ValidateChannelChange(bool isDead, bool isLoadingMap, int32 currentChannel,
    int32 requestedChannel, int32 channelCount)
{
    if (requestedChannel < 1 || requestedChannel > channelCount)
        return Protocol::ENTRY_RESULT_INVALID_CHANNEL;

    // 맵 로드를 기다리는 플레이어는 아직 룸에 없으므로 옮길 룸이 없다.
    if (isLoadingMap)
        return Protocol::ENTRY_RESULT_MAP_LOADING;

    // 사망한 플레이어는 리스폰할 때까지 사망한 룸에 남는다. 채널을 바꾸면 리스폰을 거치지 않고 다른 룸으로 빠져나간다.
    if (isDead)
        return Protocol::ENTRY_RESULT_DEAD;

    if (requestedChannel == currentChannel)
        return Protocol::ENTRY_RESULT_SAME_CHANNEL;

    return nullopt;
}
