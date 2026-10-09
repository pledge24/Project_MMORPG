#pragma once

/**
 * 룸에 들어갈 때 목적지 룸이 넘겨받는 정보. 룸 이동과 리스폰이 Room::EnterPlayer에 값으로 넘긴다.
 * enterPos가 비어 있으면 클라이언트에 입장 위치를 보내지 않는다.
 */
struct RoomEnterData
{
    int32 nextRoomId = -1;
    Protocol::EnterType enterType = Protocol::ENTER_TYPE_NONE;
    optional<Protocol::PosInfo> enterPos;
};

/**
 * Room 이동과 리스폰 요청을 판정하는 자유 함수.
 */
namespace RoomTransfer
{
    /**
     * 통과하면 nullopt, 거절하면 로그에 남길 사유를 돌려준다.
     * 최초 입장과 맵 간 이동은 C_ENTER_MAP으로 받아 둔 룸 번호와 같아야 하고, 포털 이동은 포털 번호가 있어야 한다.
     * 리스폰 입장은 서버만 만드는 값이라 요청으로 받지 않는다.
     */
    optional<string> ValidateEnterRequest(const Protocol::C_ENTER_ROOM& pkt, int32 enteringRoomId);

    /** 포털의 목적지로 포털 이동의 입장 정보를 만든다. 포털이 현재 룸에 있는지는 호출자가 확인한다. */
    RoomEnterData MakePortalEnterData(const PortalTemplate& portal, int64 entityId);

    /**
     * 통과하면 nullopt, 거절하면 S_RESPAWN에 실을 사유를 돌려준다.
     * 사망한 플레이어의, 서버가 지원하는 유형(지금은 마을 리스폰뿐)만 받는다.
     */
    optional<string> ValidateRespawn(bool isDead, Protocol::RespawnType respawnType);
}
