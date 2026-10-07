#include "Core/pch.h"
#include "Game/Room/RoomTransfer.h"

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
        return string("클라이언트가 리스폰 입장을 요청함");
    default:
        return string("알 수 없는 입장 유형");
    }
}

RoomEnterData RoomTransfer::MakePortalEnterData(const Json& portal, int64 entityId)
{
    using namespace JsonProperty::Map;
    const Json& dst = portal[Dst];

    RoomEnterData enterData;
    enterData.nextRoomId = dst[TemplateId];
    enterData.enterType = Protocol::ENTER_TYPE_SAME_MAP_TRANSFER;

    Protocol::PosInfo enterPos;
    Protocol::Vector& pos = *enterPos.mutable_pos();
    enterPos.set_entity_id(entityId);
    pos.set_x(dst[PosX]);
    pos.set_y(dst[PosY]);
    pos.set_z(dst[PosZ]);
    enterPos.set_yaw(dst[Yaw]);
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
