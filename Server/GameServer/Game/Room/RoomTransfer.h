#pragma once

/** 룸에 들어갈 때 목적지 룸이 넘겨받는 정보. */
struct RoomEnterData
{
    int32 nextRoomId = -1;
    Protocol::EnterType enterType = Protocol::ENTER_TYPE_NONE;
    /** 비어 있으면 클라이언트에 입장 위치를 보내지 않는다. */
    optional<Protocol::PosInfo> enterPos;
};

/** 마을 리스폰의 목적지. 맵 표에서 읽은 값이다. */
struct TownRespawn
{
    int32 mapId = 0;
    int32 roomId = 0;
    /** 리스폰 지점. 엔티티 id는 비어 있고 이동 상태는 Idle이다. */
    Protocol::PosInfo pos;
};

/** 입장할 룸과 그 룸에 넣을 위치. 마을로 보내면 맵도 바뀌므로 룸이 속한 맵을 함께 싣는다. */
struct RoomAssignment
{
    int32 mapId = 0;
    int32 roomId = 0;
    Protocol::PosInfo pos;
};

/**
 * 룸 이동과 리스폰의 규칙.
 *
 * 클라이언트의 룸 이동과 리스폰 요청이 규칙에 맞는지 판정하고, 들어갈 곳을 정한다.
 */
namespace RoomTransfer
{
    /** C_ENTER_ROOM을 판정한다. 통과하면 nullopt, 거절하면 로그에 남길 사유. */
    optional<string> ValidateEnterRequest(const Protocol::C_ENTER_ROOM& pkt, int32 enteringRoomId);

    /**
     * 플레이어가 포털 반경 안에 있는지 판정한다. 통과하면 nullopt, 거절하면 로그에 남길 사유.
     * 포털이 현재 룸에 있는지는 호출자가 확인한다.
     */
    optional<string> ValidatePortalUse(const PortalTemplate& portal, float portalRadius, const Protocol::PosInfo& playerPos);

    /** 포털의 목적지로 포털 이동의 입장 정보를 만든다. 포털이 현재 룸에 있는지는 호출자가 확인한다. */
    RoomEnterData MakePortalEnterData(const PortalTemplate& portal, int64 entityId);

    /**
     * 아직 어느 룸에도 들어가지 않은 플레이어의 C_ENTER_MAP을 판정한다.
     * 통과하면 nullopt, 거절하면 로그에 남길 사유.
     */
    optional<string> ValidateFirstEnterMap(const Protocol::C_ENTER_MAP& pkt, int32 enteringRoomId);

    /** 룸에 있는 플레이어의 C_ENTER_MAP을 판정한다. 통과하면 nullopt, 거절하면 로그에 남길 사유. */
    optional<string> ValidateCrossMapEnter(const Protocol::C_ENTER_MAP& pkt, const MapTemplate& currentRoom, const Protocol::PosInfo& playerPos);

    /** 리스폰 요청을 판정한다. 통과하면 nullopt, 거절하면 S_RESPAWN에 실을 사유. */
    optional<string> ValidateRespawn(bool isDead, Protocol::RespawnType respawnType);

    /**
     * 마을과 그 리스폰 지점을 맵 표에서 찾는다. 어느 스레드에서 불러도 된다.
     * 마을이 맵 표에 없거나 리스폰 지점이 없으면 nullopt.
     */
    optional<TownRespawn> FindTownRespawn();

    /**
     * 맵과 좌표로 입장할 룸을 고른다. 어느 룸의 범위에도 들지 않으면 마을 리스폰 지점으로 보낸다.
     * 마을 리스폰 지점도 없으면 nullopt. 어느 스레드에서 불러도 된다.
     */
    optional<RoomAssignment> AssignRoom(int32 mapId, const Protocol::PosInfo& pos);
}
