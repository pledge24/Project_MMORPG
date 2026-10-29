#pragma once
#include "Game/Entities/Entity.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Entities/PlayerSaveData.h"
#include "Game/Room/RoomTransfer.h"
#include "Game/Room/CellMatrix.h"

class NavQuery;

/**
 * 맵 하나 전체를 시뮬레이션하는 사본. 같은 맵의 룸들은 플레이어에게 채널로 보인다.
 *
 * 맵 안의 엔티티를 소유하고, 그 엔티티에 대한 작업을 순서대로 하나씩 처리한다.
 * 룸은 작업을 나눠 주고 누가 누구를 보는지 정할 뿐, 게임 로직은 각 엔티티와 컴포넌트가 맡는다.
 */
class Room : public JobQueue
{
public:
    Room() = default;
    /** 질의 객체의 정의가 보이는 Room.cpp에서 지운다. */
    virtual ~Room();

    //~ 생성
public:
    /**
     * 룸을 만드는 유일한 길이다. 룸의 경계와 셀 크기는 그 맵의 배치 데이터다.
     * 맵의 지역은 기획 데이터에서 찾아 두므로 Gamedata::Install 뒤에 부른다.
     */
    static RoomRef Create(int32 roomId, int32 channel, const MapPlacement& map);

protected:
    /** Create만 부른다. */
    void Init(int32 roomId, int32 channel, const MapPlacement& map);

    //~ 시작
public:
    /** 맵에 배치한 NPC와 지역마다의 몬스터를 스폰하고 첫 룸 틱을 예약한다. 스폰에 실패하면 false. */
    bool Start();

private:
    /** 이 룸의 맵에 배치한 NPC를 마커 자리에 둔다. 스폰에 실패하면 false. */
    bool SpawnNpcs();

    //~ 룸 틱
public:
    /**
     * 룸 틱 하나를 룸 큐 위에서 돈다. deltaTime은 초 단위다.
     * 다음 틱을 예약하지 않으므로, 운영 코드는 RunScheduledTick이 부르고 테스트는 직접 부른다.
     */
    void Tick(float deltaTime);

protected:
    /** 지난 틱부터 흐른 시간으로 Tick을 부르고 다음 틱을 예약한다. */
    void RunScheduledTick();
    /** 플레이어마다 그 플레이어가 보는 몬스터의 위치를 S_MOVE 하나로 모아 보낸다. 움직이지 않는 NPC는 싣지 않는다. */
    void BroadcastMonsterMoves();

private:
    const uint64 ROOM_TICK_INTERVAL_MS = 50;
    /** 몬스터 위치를 보내는 주기(초). */
    const float MOVE_SEND_INTERVAL = 0.2f;

    /** 마지막 예약 틱의 시각(ms, GetTickCount64). */
    uint64 _lastTickTime = 0;
    /** 마지막 이동 전송 뒤 흐른 시간(초). */
    float _timeSinceMoveSend = 0.f;

    //~ 플레이어 입장과 퇴장
public:
    /**
     * 플레이어를 룸에 넣고 S_ENTER_ROOM을 보낸 뒤, 주변 플레이어와 본인에게 서로의 스폰을 알린다. 실패 응답도 이 함수가 보낸다.
     * 그사이 접속이 끊겼으면 퇴장과 저장을 이어 받고 false.
     */
	bool EnterPlayer(PlayerRef enterPlayer, RoomEnterData roomEnterData);
    /** 주변 플레이어에게 S_DESPAWN을 알린다. transferRoom이 false면 떠나는 본인에게도 보낸다. */
    bool LeavePlayer(PlayerRef leavePlayer, bool transferRoom);
    /**
     * 끊긴 플레이어를 룸에서 빼고 저장 사본을 돌려준다. 이 룸에서 맵 로드를 기다리던 플레이어도 저장한다.
     * 저장은 부른 쪽이 한다. 이 룸에 없거나 소속 룸이 아니면 nullopt.
     */
    optional<PlayerSaveData> HandleDisconnect(PlayerRef player);

private:
    /**
     * 이 룸에 있는 player를 destination의 enterPos로 보낸다. destination이 이 룸이면 룸 안의 순간이동이고,
     * 맵이 바뀌면 클라이언트의 맵 로드 완료를 기다리게 한다. 응답은 부르기 전에 보낸다.
     */
    void MovePlayer(const PlayerRef& player, const RoomRef& destination, const Protocol::PosInfo& enterPos);
    /** 룸을 떠나지 않고 pos로 옮긴다. 본인은 룸 입장 응답과 주변 스폰을 다시 받고, 주변 플레이어는 디스폰과 스폰을 받는다. */
    void TeleportPlayer(const PlayerRef& player, const Protocol::PosInfo& pos);
    /** 본인에게 룸 입장 응답과 주변 엔티티의 스폰을 보내고, 주변 플레이어에게 본인의 스폰을 알린다. */
    void AnnounceEnter(const PlayerRef& player);
    /** 포털 이동과 리스폰이 들어갈 룸. 이 룸의 맵이면 이 룸, 다른 맵이면 그 맵에서 인원이 가장 적은 채널이다. 맵이 없으면 nullptr. */
    RoomRef SelectDestinationRoom(int32 mapId);

    //~ 클라이언트 패킷 핸들러
public:
    /**
     * 이 룸에서 맵 로드를 기다리던 플레이어를 룸에 넣는다. 기다리지 않던 플레이어의 로드 완료는 버린다.
     * 이 섹션의 핸들러는 ServerPacketHandler가 DoAsync로 넣으므로, 잡이 돌 때 세션이 끊겼을 수 있다.
     */
    void C_HandleMapLoadComplete(PlayerRef player);
    /** 목적지가 다른 맵이면 응답에 그 맵 번호를 싣고 맵 로드를 기다리게 한다. 같은 맵이면 룸 안에서 옮긴다. */
    void C_HandleUsePortal(Protocol::C_USE_PORTAL pkt, PlayerRef player);
    /** 보낸 사람(player)의 위치를 바꾼다. */
    void C_HandleMove(Protocol::C_MOVE pkt, PlayerRef player);
    /** 사망한 플레이어의 공격은 응답으로 거절하고 다른 플레이어에게 알리지 않는다. */
    void C_HandleNormalAttack(Protocol::C_NORMAL_ATTACK pkt, PlayerRef player);
    /** 이 룸에서 되살린 뒤 마을로 보낸다. 마을이 이 룸의 맵에 있으면 룸 안에서 옮긴다. */
    void C_HandleRespawn(Protocol::C_RESPAWN pkt, PlayerRef player);
    /** 이 룸이 속한 맵의 채널마다 인원을 응답한다. */
    void C_HandleChannelList(PlayerRef player);
    /** 같은 맵의 다른 채널 룸으로 옮긴다. 맵을 다시 열지 않는 룸 이동이다. */
    void C_HandleChangeChannel(Protocol::C_CHANGE_CHANNEL pkt, PlayerRef player);

    //~ 전투와 리스폰
public:
    /** 공격 사실만 브로드캐스트한다. 피격 판정은 HandleHit이 따로 한다. */
    void HandleNormalAttack(int32 combo, CreatureRef creature);
    /** 공격 하나의 피격을 판정한다. 대상이 죽으면 보상과 사망까지 처리한다. */
    void HandleHit(EntityRef attacker, AttackInfo attackInfo);
    void HandleMonsterKill(PlayerRef player, const Reward& reward);
    /** 몬스터는 룸에서 뺀다. 플레이어는 리스폰할 때까지 룸에 남긴다. */
    void HandleDie(CreatureRef creature);

private:
    /** 공격한 엔티티를 뺀, 공격한 엔티티를 보는 플레이어에게 공격을 알린다. */
    void BroadcastAttack(int64 attackerId, int32 combo, float yaw);

    //~ Room 정보 관련
public:
    /**
     * bounds 안의 무작위 평면 위치. usePadding이면 경계에서 안쪽으로 떨어진 곳에서 고른다.
     * 여백을 빼고 남는 폭이 없는 축은 그 축의 가운데다.
     */
    static vector2D     GetRandomLocation(const PlaneBounds& bounds, bool usePadding = true);
    /** -180도에서 180도 사이. */
    static float        GetRandomYaw() { return Utils::GetRandom(-180.f, 180.f); }
    RoomRef             GetRoomRef() { return static_pointer_cast<Room>(shared_from_this()); }
    /** 아래 정보는 Init 뒤로 바뀌지 않으므로 룸 큐 밖에서 읽어도 된다. */
    int32               GetRoomId() const { return _roomId; }
    int32               GetMapId() const { return _map.mapId; }
    /** 1부터다. */
    int32               GetChannel() const { return _channel; }
    /** 이 룸의 맵에 그 번호의 포털이 없으면 nullptr. */
    const DesignData::PortalRow* FindPortal(int32 portalId) const;
    /** 맵 경계의 중심. 높이는 0이다. */
    vector3D            GetCenterPoint() const;

private:
    /** 몬스터를 스폰할 지역 하나. */
    struct RoomArea
    {
        DesignData::AreaRow row;
        AreaPlacement placement;
    };

    /** 지역 안의 무작위 위치. 높이는 그 몬스터의 캡슐이 바닥에 서는 높이이고, navmesh가 없으면 지역 바닥보다 조금 위다. */
    Protocol::PosInfo MakeRandomSpawnPos(const AreaPlacement& area, int32 monsterTemplateId);
    /** posInfo의 높이를 바닥 위 캡슐 중심(바닥 + halfHeight)으로 바꾼다. navmesh가 없거나 바닥이 없으면 그대로 두고 false. */
    bool PlaceOnFloor(Protocol::PosInfo& posInfo, float halfHeight);
    /** 플레이어 캡슐의 반높이. navmesh의 에이전트가 플레이어 캡슐이다. navmesh가 없으면 0. */
    float PlayerHalfHeight() const;

    static constexpr float LOCATION_PADDING_X = 1000.f;
    static constexpr float LOCATION_PADDING_Y = 1000.f;
    static constexpr float LOCATION_PADDING_Z = 100.f;

    int32 _roomId = 0;
    int32 _channel = 0;
    /** 이 룸이 맡은 맵의 배치 데이터 사본. 경계가 룸의 경계다. */
    MapPlacement _map;
    /** 이 맵의 지역. 지역 번호 순서다. */
    vector<RoomArea> _areas;
    /** 맵의 navmesh에 묻는 이 룸의 질의 객체. navmesh 없이 만든 룸(테스트)이면 nullptr이고 지형을 보지 않는다. */
    unique_ptr<NavQuery> _navQuery;

    //~ 엔티티
public:
    bool Contains(int64 entityId) { return _entities.contains(entityId); }
    /** 룸에 있는 플레이어 수. 룸 큐 밖에서 읽으면 읽는 순간의 값이다. */
    int32 GetPlayerCount() const { return _playerCount.load(); }
    /** 이 룸의 NPC. 없거나 NPC가 아니면 nullptr. 룸 큐 위에서 부른다. */
    NpcRef FindNpc(int64 entityId);

    /**
     * 엔티티를 만들어 이 룸에 넣고 Start까지 부른다. UWorld::SpawnActor에 대응한다.
     * T의 정의가 보이는 곳에서 부른다. 실패하면 nullptr.
     */
    template<typename T>
    shared_ptr<T> SpawnEntity(const typename T::SpawnParams& params)
    {
        shared_ptr<T> entity = EntityFactory::Create<T>(params);
        if (entity == nullptr)
            return nullptr;

        if (AddEntity(entity) == false)
        {
            GLogger->Error("룸 {}: 엔티티 {}를 등록하지 못했다", _roomId, entity->GetEntityId());
            return nullptr;
        }

        return entity;
    }

protected:
    /** 엔티티를 이 룸과 그 위치의 셀에 넣고, 처음 룸에 들어가는 엔티티면 Start를 부른다. 이미 있는 id면 false. */
    bool AddEntity(EntityRef entity);
    /** 엔티티를 룸과 셀에서 뺀다. 알리지 않으므로 알릴 것은 부르기 전에 보낸다. */
    bool RemoveEntity(int64 entityId);

    /** 엔티티를 찾아 T로 내린다. 없거나 T가 아니면 nullptr. */
    template<typename T>
    shared_ptr<T> FindEntityAs(int64 entityId)
    {
        auto it = _entities.find(entityId);
        if (it == _entities.end())
            return nullptr;

        return dynamic_pointer_cast<T>(it->second);
    }

private:
    /** 엔티티의 소유권을 가진다. 여기서 빠지면 다른 곳이 들고 있지 않는 한 엔티티가 사라진다. */
	unordered_map<int64, EntityRef> _entities;
    /** 룸 큐만 쓰고, 채널을 고르는 쪽이 룸 큐 밖에서 읽는다. */
    atomic<int32> _playerCount = 0;

    //~ 시야
public:
    /**
     * 엔티티가 지금 위치로 다른 셀에 들어갔으면 셀을 옮기고, 새로 보게 된 쪽과 안 보게 된 쪽에 스폰과 디스폰을 알린다.
     * 엔티티가 플레이어면 본인에게도 새로 보이는 엔티티의 스폰과 안 보이게 된 엔티티의 디스폰을 보낸다.
     */
    void UpdateVisibility(const EntityRef& entity);
    /** sourceId를 보는 플레이어 전원에게 보낸다. exceptId가 0이 아니면 그 엔티티는 뺀다. 룸에 없는 엔티티면 보내지 않는다. */
    void BroadcastNearby(int64 sourceId, SendBufferRef sendBuffer, int64 exceptId = 0);
    /** 룸의 플레이어 전원에게 보낸다. 채팅처럼 시야와 무관하게 룸 전체에 가는 것만 쓴다. */
    void Broadcast(SendBufferRef sendBuffer, int64 exceptId = 0);

protected:
    /** 모든 엔티티의 셀을 지금 위치로 맞춘다. 룸 틱이 엔티티 Tick 전에 부른다. */
    void RefreshVisibility();
    /** entityId가 든 셀과 이웃 셀의 엔티티. 본인도 들어 있다. 셀이 없으면 빈 목록. */
    vector<EntityRef> FindNearbyEntities(int64 entityId) const;

private:
    /** 시야는 셀 단위다. 셀 크기는 맵의 배치 데이터에서 온다. */
    CellMatrix _cellMatrix;

    //~ 위치 탐색
public:
    /**
     * range 안에서 가장 가까운 살아 있는 플레이어와 거리의 제곱. 없으면 (nullptr, -1).
     * 셀은 이번 룸 틱이 시작할 때나 그 플레이어가 마지막으로 움직였을 때의 것이다.
     */
    pair<PlayerRef, float> FindClosestPlayer(const Protocol::PosInfo* posInfo, float range);
};
