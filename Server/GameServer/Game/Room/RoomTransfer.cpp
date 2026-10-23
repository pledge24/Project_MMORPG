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

RoomEnterData RoomTransfer::MakePortalEnterData(const PortalTemplate& portal)
{
    RoomEnterData enterData;
    enterData.nextRoomId = portal.dstRoomId;

    Protocol::PosInfo enterPos;
    Protocol::Vector& pos = *enterPos.mutable_pos();
    pos.set_x(portal.dstPos.x);
    pos.set_y(portal.dstPos.y);
    pos.set_z(portal.dstPos.z);
    enterPos.set_yaw(portal.dstYaw);
    enterPos.set_state(Protocol::MoveState::MOVE_STATE_IDLE);

    // 룸 입장 응답이 내 엔티티의 위치로 이 값을 실으므로 반드시 채워야 한다.
    enterData.enterPos = std::move(enterPos);

    return enterData;
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

optional<TownRespawn> RoomTransfer::FindTownRespawn()
{
    const int32 townRoomId = Gamedata::GetTownRoomId();
    const MapTemplate* town = Gamedata::FindMap(townRoomId);
    if (town == nullptr || town->respawnPoint.has_value() == false)
        return nullopt;

    const TemplatePos& point = town->respawnPoint.value();

    TownRespawn respawn;
    respawn.mapId = town->mapId;
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
        return RoomAssignment{ range->mapId, SelectRoomForRange(*range), pos };

    // 어느 룸에도 들지 않는 좌표는 마을 리스폰 지점에서 시작한다. 맵 표가 바뀌어 저장된 좌표가 범위 밖이 된 경우다.
    optional<TownRespawn> town = FindTownRespawn();
    if (town.has_value() == false)
        return nullopt;

    return RoomAssignment{ town->mapId, town->roomId, town->pos };
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
