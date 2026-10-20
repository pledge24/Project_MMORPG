#include "Core/pch.h"
#include "Game/Room/RoomTransfer.h"

namespace
{
    // 좌표가 범위 안에 드는 맵 표 행을 찾는다. 높이는 보지 않는다.
    // 경계 위의 점은 안쪽이다. 이동 판정(MoveValidation)이 경계 위를 룸 안으로 받으므로, 경계에서 저장한
    // 플레이어를 밖으로 판정하면 다음 입장에서 마을로 밀려난다. 두 범위가 경계를 함께 쓰면 맵 표를 행 번호
    // 순서로 돌기 때문에 행 번호가 작은 쪽이 잡힌다.
    const MapTemplate* FindRangeContaining(int32 mapId, const Protocol::Vector& pos)
    {
        for (const auto& [templateId, range] : Gamedata::GetMaps())
        {
            if (range.mapId != mapId)
                continue;

            if (pos.x() < range.center.x - range.depthHalfExtent || pos.x() > range.center.x + range.depthHalfExtent)
                continue;
            if (pos.y() < range.center.y - range.widthHalfExtent || pos.y() > range.center.y + range.widthHalfExtent)
                continue;

            return &range;
        }

        return nullptr;
    }

    // 범위를 맡을 룸을 정한다. 지금은 맵 표 행 하나가 룸 하나다.
    // 범위 여럿을 룸 하나로 묶거나 범위 하나를 여러 룸으로 나눌 때, 입장할 룸을 고르는 규칙은 여기만 바꾼다.
    int32 SelectRoomForRange(const MapTemplate& range)
    {
        return range.templateId;
    }
}

optional<string> RoomTransfer::ValidateEnterRequest(const Protocol::C_ENTER_ROOM& pkt, int32 enteringRoomId)
{
    switch (pkt.enter_type())
    {
    case Protocol::ENTER_TYPE_INITIAL:
    case Protocol::ENTER_TYPE_CROSS_MAP_TRANSFER:
        if (pkt.has_room_id() == false)
            return string("룸 번호가 없는 입장 요청");
        if (pkt.room_id() != enteringRoomId)
            return string("C_ENTER_MAP으로 받아 둔 룸 번호와 다른 입장 요청");
        return nullopt;
    case Protocol::ENTER_TYPE_SAME_MAP_TRANSFER:
        if (pkt.has_portal_id() == false)
            return string("포털 번호가 없는 포털 이동 요청");
        return nullopt;
    case Protocol::ENTER_TYPE_RESPAWN:
        // 리스폰 입장은 서버만 만드는 값이라 요청으로 받지 않는다.
        return string("클라이언트가 리스폰 입장을 요청함");
    default:
        return string("알 수 없는 입장 유형");
    }
}

RoomEnterData RoomTransfer::MakePortalEnterData(const PortalTemplate& portal, int64 entityId)
{
    RoomEnterData enterData;
    enterData.nextRoomId = portal.dstRoomId;
    enterData.enterType = Protocol::ENTER_TYPE_SAME_MAP_TRANSFER;

    Protocol::PosInfo enterPos;
    Protocol::Vector& pos = *enterPos.mutable_pos();
    enterPos.set_entity_id(entityId);
    pos.set_x(portal.dstPos.x);
    pos.set_y(portal.dstPos.y);
    pos.set_z(portal.dstPos.z);
    enterPos.set_yaw(portal.dstYaw);
    enterPos.set_state(Protocol::MoveState::MOVE_STATE_IDLE);

    // 클라이언트는 내 플레이어를 이 enter_pos로 옮기므로 반드시 채워야 한다.
    enterData.enterPos = std::move(enterPos);

    return enterData;
}

optional<string> RoomTransfer::ValidateRespawn(bool isDead, Protocol::RespawnType respawnType)
{
    if (isDead == false)
        return string("사망하지 않은 플레이어는 리스폰할 수 없습니다.");

    // 다른 유형은 목적지 규칙이 아직 없다. 지원하는 날 그 유형의 목적지와 함께 연다.
    if (respawnType != Protocol::RESPAWN_TYPE_TOWN)
        return string("지원하지 않는 리스폰 유형입니다.");

    return nullopt;
}

optional<TownRespawn> RoomTransfer::FindTownRespawn()
{
    const int32 townRoomId = Gamedata::GetTownRoomId();
    const MapTemplate* town = Gamedata::FindMap(townRoomId);
    if (town == nullptr || town->respawnPoint.has_value() == false)
        return nullopt;

    const TemplatePos& point = town->respawnPoint.value();

    TownRespawn respawn;
    respawn.roomId = townRoomId;
    respawn.pos.mutable_pos()->set_x(point.x);
    respawn.pos.mutable_pos()->set_y(point.y);
    respawn.pos.mutable_pos()->set_z(point.z);
    respawn.pos.set_yaw(0.f);
    respawn.pos.set_state(Protocol::MoveState::MOVE_STATE_IDLE);

    return respawn;
}

optional<RoomAssignment> RoomTransfer::AssignRoom(int32 mapId, const Protocol::PosInfo& pos)
{
    if (const MapTemplate* range = FindRangeContaining(mapId, pos.pos()))
        return RoomAssignment{ SelectRoomForRange(*range), pos };

    // 어느 룸에도 들지 않는 좌표는 마을 리스폰 지점에서 시작한다. 맵 표가 바뀌어 저장된 좌표가 범위 밖이 된 경우다.
    optional<TownRespawn> town = FindTownRespawn();
    if (town.has_value() == false)
        return nullopt;

    return RoomAssignment{ town->roomId, town->pos };
}

optional<string> RoomTransfer::ValidatePortalUse(const PortalTemplate& portal, float portalRadius, const Protocol::PosInfo& playerPos)
{
    // 높이는 보지 않는다. 포털 좌표의 z는 바닥이고 플레이어의 z는 캡슐 중심이다.
    const float dx = playerPos.pos().x() - portal.srcPos.x;
    const float dy = playerPos.pos().y() - portal.srcPos.y;
    if (dx * dx + dy * dy > portalRadius * portalRadius)
        return format("포털 {}의 반경 {} 밖에서 온 포털 이동 요청", portal.portalId, portalRadius);

    return nullopt;
}

optional<string> RoomTransfer::ValidateFirstEnterMap(const Protocol::C_ENTER_MAP& pkt, int32 enteringRoomId)
{
    // 첫 입장은 불러온 진행의 룸으로만 할 수 있다.
    if (pkt.room_id() != enteringRoomId)
        return format("불러온 룸 {}이 아닌 룸 {}으로 온 첫 맵 입장 요청", enteringRoomId, pkt.room_id());

    const MapTemplate* room = Gamedata::FindMap(pkt.room_id());
    if (room == nullptr)
        return format("맵 표에 없는 룸 {}으로 온 맵 입장 요청", pkt.room_id());
    if (pkt.map_id() != room->mapId)
        return format("룸 {}의 맵 {}과 다른 맵 {}으로 온 맵 입장 요청", pkt.room_id(), room->mapId, pkt.map_id());

    return nullopt;
}

optional<string> RoomTransfer::ValidateCrossMapEnter(const Protocol::C_ENTER_MAP& pkt, const MapTemplate& currentRoom, const Protocol::PosInfo& playerPos)
{
    const MapTemplate* destination = Gamedata::FindMap(pkt.room_id());
    if (destination == nullptr)
        return format("맵 표에 없는 룸 {}으로 온 맵 입장 요청", pkt.room_id());
    if (pkt.map_id() != destination->mapId)
        return format("룸 {}의 맵 {}과 다른 맵 {}으로 온 맵 입장 요청", pkt.room_id(), destination->mapId, pkt.map_id());
    if (destination->mapId == currentRoom.mapId)
        return format("같은 맵의 룸 {}으로 온 맵 입장 요청. 같은 맵 안의 이동은 포털 이동이다", pkt.room_id());

    // 맵 간 이동도 포털을 거친다. 목적지로 가는 포털 중 하나의 반경 안에만 있으면 통과한다.
    for (const PortalTemplate& portal : currentRoom.portals)
    {
        if (portal.dstRoomId != pkt.room_id())
            continue;

        if (ValidatePortalUse(portal, currentRoom.portalRadius, playerPos).has_value() == false)
            return nullopt;
    }

    return format("룸 {}에서 룸 {}으로 가는 포털의 반경 밖에서 온 맵 입장 요청", currentRoom.templateId, pkt.room_id());
}
