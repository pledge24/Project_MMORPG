#pragma once

/** 룸에 들어갈 때 목적지 룸이 넘겨받는 정보. */
struct RoomEnterData
{
    int32 nextRoomId = -1;
    /** 비어 있으면 룸 중심에 둔다. */
    optional<Protocol::PosInfo> enterPos;
};

/** 마을 리스폰의 목적지. 지역 표에서 읽은 값이다. */
struct TownRespawn
{
    int32 mapId = 0;
    int32 roomId = 0;
    /** 리스폰 지점. 이동 상태는 Idle이다. */
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
    /**
     * 플레이어가 포털 반경 안에 있는지 판정한다. 통과하면 nullopt, 거절하면 로그에 남길 사유.
     * 포털이 현재 룸에 있는지는 호출자가 확인한다.
     */
    optional<string> ValidatePortalUse(const DesignData::PortalRow& portal, const Protocol::PosInfo& playerPos);

    /** 그 지역을 맡은 룸의 번호. 어느 스레드에서 불러도 된다. */
    int32 SelectRoomForArea(int32 areaId);

    /** 포털의 도착 지역을 맡은 룸과 도착 좌표로 포털 이동의 입장 정보를 만든다. 포털이 현재 룸에 있는지는 호출자가 확인한다. */
    RoomEnterData MakePortalEnterData(const DesignData::PortalRow& portal);

    /** 리스폰 요청을 판정한다. 통과하면 nullopt, 거절하면 S_RESPAWN에 실을 결과. */
    optional<Protocol::EntryResult> ValidateRespawn(bool isDead, Protocol::RespawnType respawnType);

    /**
     * 마을과 그 리스폰 지점을 지역 표에서 찾는다. 어느 스레드에서 불러도 된다.
     * 마을이 지역 표에 없거나 리스폰 지점이 없으면 nullopt.
     */
    optional<TownRespawn> FindTownRespawn();

    /**
     * 맵과 좌표로 입장할 룸을 고른다. 그 맵의 어느 지역에도 들지 않으면 마을 리스폰 지점으로 보낸다.
     * 마을 리스폰 지점도 없으면 nullopt. 어느 스레드에서 불러도 된다.
     */
    optional<RoomAssignment> AssignRoom(int32 mapId, const Protocol::PosInfo& pos);
}
