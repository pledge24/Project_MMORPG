#pragma once

/** 룸에 들어갈 때 목적지 룸이 넘겨받는 정보. */
struct RoomEnterData
{
    int32 nextRoomId = -1;
    /** 비어 있으면 맵 경계의 중심에 둔다. */
    optional<Protocol::PosInfo> enterPos;
};

/** 맵 하나 안의 위치. 들어갈 룸(채널)은 이 맵의 룸 중에서 따로 고른다. */
struct MapPosition
{
    int32 mapId = 0;
    /** 이동 상태는 Idle이다. */
    Protocol::PosInfo pos;
};

/**
 * 룸 이동과 리스폰의 규칙.
 *
 * 클라이언트의 룸 이동, 리스폰, 채널 변경 요청이 규칙에 맞는지 판정하고, 들어갈 맵과 위치, 채널을 정한다.
 * 룸 큐와 세션을 모르므로 어느 스레드에서 불러도 된다.
 */
namespace RoomTransfer
{
    /**
     * 플레이어가 포털 반경 안에 있는지 판정한다. 통과하면 nullopt, 거절하면 로그에 남길 사유.
     * 포털이 현재 룸의 맵에 있는지는 호출자가 확인한다.
     */
    optional<string> ValidatePortalUse(const PortalPlacement& portal, const Protocol::PosInfo& playerPos);

    /** 포털의 도착 지역이 속한 맵과 배치한 도착 지점. 도착 지역이 지역 표에 없으면 nullopt. */
    optional<MapPosition> FindPortalDestination(const DesignData::PortalRow& portal, const PortalPlacement& placement);

    /** 리스폰 요청을 판정한다. 통과하면 nullopt, 거절하면 S_RESPAWN에 실을 결과. */
    optional<Protocol::EntryResult> ValidateRespawn(bool isDead, Protocol::RespawnType respawnType);

    /**
     * 마을 리스폰의 목적지. 마을을 지역 표에서, 그 리스폰 지점을 배치 데이터에서 찾는다.
     * 마을이 지역 표에 없거나 리스폰 지점이 없으면 nullopt.
     */
    optional<MapPosition> FindTownRespawn();

    /**
     * 맵과 좌표로 입장할 위치를 정한다. 그 맵의 어느 지역에도 들지 않으면 마을 리스폰 지점으로 보낸다.
     * 마을 리스폰 지점도 없으면 nullopt.
     */
    optional<MapPosition> ResolveEnterPosition(int32 mapId, const Protocol::PosInfo& pos);

    /** 채널마다의 인원(채널 1부터 차례로)에서 인원이 가장 적은 채널. 같으면 번호가 작은 채널이고, 채널이 없으면 0. */
    int32 SelectLeastPopulatedChannel(const vector<int32>& playerCounts);

    /** 채널 변경 요청을 판정한다. 채널은 1부터 channelCount까지다. 통과하면 nullopt, 거절하면 S_CHANGE_CHANNEL에 실을 결과. */
    optional<Protocol::EntryResult> ValidateChannelChange(bool isDead, bool isLoadingMap, int32 currentChannel,
        int32 requestedChannel, int32 channelCount);
}
