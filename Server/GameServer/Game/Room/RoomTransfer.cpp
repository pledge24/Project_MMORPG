#include "Core/pch.h"
#include "Game/Room/RoomTransfer.h"

namespace
{
    // 그 맵에서 좌표가 드는 지역을 찾는다. 높이는 보지 않는다.
    // 경계 위의 점은 안쪽이다. 이동 판정(MoveValidation)이 경계 위를 룸 안으로 받으므로, 경계에서 저장한
    // 플레이어를 밖으로 판정하면 다음 입장에서 마을로 밀려난다. 두 지역이 경계를 함께 쓰면 지역 표를 지역 번호
    // 순서로 돌기 때문에 번호가 작은 쪽이 잡힌다.
    const DesignData::AreaRow* FindAreaContaining(int32 mapId, const Protocol::Vector& pos)
    {
        for (const auto& [areaId, area] : Gamedata::GetAreas())
        {
            if (area.MapId != mapId)
                continue;

            if (pos.x() < area.CenterX - area.HalfExtentX || pos.x() > area.CenterX + area.HalfExtentX)
                continue;
            if (pos.y() < area.CenterY - area.HalfExtentY || pos.y() > area.CenterY + area.HalfExtentY)
                continue;

            return &area;
        }

        return nullptr;
    }
}

int32 RoomTransfer::SelectRoomForArea(int32 areaId)
{
    // 지금은 지역 하나가 룸 하나라서 지역 번호가 룸 번호다. 룸을 만들 때도 이 함수로 룸 번호를 정한다.
    // 지역 여럿을 룸 하나로 묶거나 지역 하나를 여러 룸으로 나눌 때, 입장할 룸을 고르는 규칙은 여기만 바꾼다.
    return areaId;
}

RoomEnterData RoomTransfer::MakePortalEnterData(const DesignData::PortalRow& portal)
{
    RoomEnterData enterData;
    enterData.nextRoomId = SelectRoomForArea(portal.DestAreaId);

    Protocol::PosInfo enterPos;
    Protocol::Vector& pos = *enterPos.mutable_pos();
    pos.set_x(portal.DestX);
    pos.set_y(portal.DestY);
    pos.set_z(portal.DestZ);
    enterPos.set_yaw(portal.DestYaw);
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
    const DesignData::AreaRow* town = Gamedata::FindArea(Gamedata::GetTownAreaId());
    if (town == nullptr || town->HasRespawnPoint == false
        || town->RespawnX.has_value() == false || town->RespawnY.has_value() == false || town->RespawnZ.has_value() == false)
        return nullopt;

    TownRespawn respawn;
    respawn.mapId = town->MapId;
    respawn.roomId = SelectRoomForArea(town->Id);
    respawn.pos.mutable_pos()->set_x(town->RespawnX.value());
    respawn.pos.mutable_pos()->set_y(town->RespawnY.value());
    respawn.pos.mutable_pos()->set_z(town->RespawnZ.value());
    respawn.pos.set_yaw(0.f);
    respawn.pos.set_state(Protocol::MoveState::MOVE_STATE_IDLE);

    return respawn;
}

optional<RoomAssignment> RoomTransfer::AssignRoom(int32 mapId, const Protocol::PosInfo& pos)
{
    if (const DesignData::AreaRow* area = FindAreaContaining(mapId, pos.pos()))
        return RoomAssignment{ area->MapId, SelectRoomForArea(area->Id), pos };

    // 어느 지역에도 들지 않는 좌표는 마을 리스폰 지점에서 시작한다. 지역 표가 바뀌어 저장된 좌표가 지역 밖이 된 경우다.
    optional<TownRespawn> town = FindTownRespawn();
    if (town.has_value() == false)
        return nullopt;

    return RoomAssignment{ town->mapId, town->roomId, town->pos };
}

optional<string> RoomTransfer::ValidatePortalUse(const DesignData::PortalRow& portal, const Protocol::PosInfo& playerPos)
{
    // 높이는 보지 않는다. 포털 좌표의 z는 바닥이고 플레이어의 z는 캡슐 중심이다.
    const float dx = playerPos.pos().x() - portal.SrcX;
    const float dy = playerPos.pos().y() - portal.SrcY;
    if (dx * dx + dy * dy > portal.Radius * portal.Radius)
        return format("포털 {}의 반경 {} 밖에서 온 포털 이동 요청", portal.Id, portal.Radius);

    return nullopt;
}
