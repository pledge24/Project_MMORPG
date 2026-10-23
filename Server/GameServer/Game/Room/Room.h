#pragma once
#include "Game/Entities/Entity.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Entities/PlayerSaveData.h"
#include "Game/Room/RoomTransfer.h"
#include "Game/Room/CellMatrix.h"

/**
 * 게임 서버가 동기화를 처리하는 최소 공간 단위.
 *
 * 공간 안의 엔티티를 소유하고, 그 엔티티에 대한 작업을 순서대로 하나씩 처리한다.
 * 룸은 작업을 나눠 주기만 하고, 게임 로직은 각 엔티티와 컴포넌트가 맡는다.
 */
class Room : public JobQueue
{
public:
    Room() = default;
	virtual ~Room() = default;

    //~ 생성
public:
    /** 룸을 만드는 유일한 길이다. 맵 표는 부팅 때 검증했으므로 실패하지 않는다. */
    static RoomRef Create(const MapTemplate& mapTemplate);

protected:
    /** Create만 부른다. */
    void Init(const MapTemplate& mapTemplate);
    void CacheRoomData();

    //~ 시작
public:
    /** 몬스터를 스폰하고 첫 룸 틱을 예약한다. 스폰에 실패하면 false. */
    bool Start();

private:
    /** Start가 스폰할 몬스터의 수와 종류. */
    int32 _maxMonsterCount;
    vector<int32> _monsterIds;

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
    /** 몬스터의 위치를 S_MOVE 하나로 모아 룸 전체에 보낸다. */
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
     * 플레이어를 룸에 넣고 S_ENTER_ROOM을 보낸 뒤, 다른 플레이어와 본인에게 서로의 스폰을 알린다. 실패 응답도 이 함수가 보낸다.
     * 그사이 접속이 끊겼으면 퇴장과 저장을 이어 받고 false.
     */
	bool EnterPlayer(PlayerRef enterPlayer, RoomEnterData roomEnterData);
    /** 다른 플레이어에게 S_DESPAWN을 알린다. transferRoom이 false면 떠나는 본인에게도 보낸다. */
    bool LeavePlayer(PlayerRef leavePlayer, bool transferRoom);
    /**
     * 끊긴 플레이어를 룸에서 빼고 저장 사본을 돌려준다. 이 룸에서 맵 로드를 기다리던 플레이어도 저장한다.
     * 저장은 부른 쪽이 한다. 이 룸에 없거나 소속 룸이 아니면 nullopt.
     */
    optional<PlayerSaveData> HandleDisconnect(PlayerRef player);

private:
    /**
     * 이 룸에 있는 player를 빼고 destination의 enterPos로 보낸다. 맵이 바뀌면 클라이언트의 맵 로드 완료를 기다리게 한다.
     * 응답은 부르기 전에 보낸다. 같은 맵이면 목적지 룸 큐가 곧바로 S_ENTER_ROOM을 보낸다.
     */
    void MovePlayer(const PlayerRef& player, const RoomRef& destination, const Protocol::PosInfo& enterPos);

    //~ 클라이언트 패킷 핸들러
public:
    /**
     * 이 룸에서 맵 로드를 기다리던 플레이어를 룸에 넣는다. 기다리지 않던 플레이어의 로드 완료는 버린다.
     * 이 섹션의 핸들러는 ServerPacketHandler가 DoAsync로 넣으므로, 잡이 돌 때 세션이 끊겼을 수 있다.
     */
    void C_HandleMapLoadComplete(PlayerRef player);
    /** 목적지가 다른 맵이면 응답에 그 맵 번호를 싣고 맵 로드를 기다리게 한다. 같은 맵이면 바로 옮긴다. */
    void C_HandleUsePortal(Protocol::C_USE_PORTAL pkt, PlayerRef player);
    /** 보낸 사람(player)의 위치만 바꾼다. 패킷의 엔티티 번호는 보지 않는다. */
    void C_HandleMove(Protocol::C_MOVE pkt, PlayerRef player);
    void C_HandleNormalAttack(Protocol::C_NORMAL_ATTACK pkt, PlayerRef player);
    /** 이 룸에서 되살린 뒤 마을로 보낸다. 사망한 룸이 마을이어도 퇴장과 입장을 거친다. */
    void C_HandleRespawn(Protocol::C_RESPAWN pkt, PlayerRef player);

    //~ 전투와 리스폰
public:
    /** 공격 사실만 브로드캐스트한다. 피격 판정은 HandleHit이 따로 한다. */
    void HandleNormalAttack(int32 combo, CreatureRef creature);
    /** 공격 하나의 피격을 판정한다. 대상이 죽으면 보상과 사망까지 처리한다. */
    void HandleHit(EntityRef attacker, Protocol::AttackInfo attackInfo);
    void HandleMonsterKill(PlayerRef player, const Protocol::Reward& reward);
    /** 몬스터는 룸에서 뺀다. 플레이어는 리스폰할 때까지 룸에 남긴다. */
    void HandleDie(CreatureRef creature);

    //~ Room 정보 관련
public:
    /** usePadding이면 룸 경계에서 안쪽으로 떨어진 곳에서 고른다. */
    vector2D            GetRandomLocation(bool usePadding = true);
    /** -180도에서 180도 사이. */
    float               GetRandomYaw() { return Utils::GetRandom(-180.f, 180.f); }
    RoomRef             GetRoomRef() { return static_pointer_cast<Room>(shared_from_this()); }
    /** Init 뒤로는 바뀌지 않으므로 룸 큐 밖에서 읽어도 된다. */
    int32               GetRoomId() const { return _roomId; }
    /** 이 룸이 속한 맵. Init 뒤로는 바뀌지 않으므로 룸 큐 밖에서 읽어도 된다. */
    int32               GetMapId() const { return _mapTemplate.mapId; }
    /** 이 룸에 그 번호의 포털이 없으면 nullptr. */
    const PortalTemplate* FindPortal(int32 portalId) const;
    const vector3D&     GetCenterPoint() const { return _roomCenterPos; }

    /** posInfo를 룸 안의 무작위 위치로 바꾼다. randYaw가 false면 yaw는 그대로 둔다. */
    void SetRandomPos(IN Protocol::PosInfo* posInfo, bool usePadding = true, bool randYaw = false);

private:
    const float LOCATION_PADDING_X = 1000.f;
    const float LOCATION_PADDING_Y = 1000.f;
    const float LOCATION_PADDING_Z = 100.f;

    int32 _roomId;
    /** 맵 표의 행 사본. */
    MapTemplate _mapTemplate;

    /** 언리얼 좌표를 따른다. depth가 x 방향, width가 y 방향의 반폭이다. */
    vector3D _roomCenterPos;
    float _depthHalfExtent;
    float _widthHalfExtent;

    float _roomMinX;
    float _roomMaxX;
    float _roomMinY;
    float _roomMaxY;

    //~ 엔티티
public:
    bool Contains(int64 entityId) { return _entities.contains(entityId); }

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
    /** 다른 플레이어에게만 S_SPAWN을 알린다(본인 제외). 이 룸에 없는 플레이어면 알리지 않고 nullptr. */
    PlayerRef SpawnPlayer(PlayerRef targetPlayer);
    /** 룸의 엔티티 중 player 자신을 뺀 나머지를 S_SPAWN으로 player에게 보낸다. */
    void ReplicateRoomData(PlayerRef player);

protected:
    /** 엔티티를 이 룸에 넣고, 처음 룸에 들어가는 엔티티면 Start를 부른다. 이미 있는 id면 false. */
    bool AddEntity(EntityRef entity);
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

    //~ 위치 탐색
public:
    /**
     * range 안에서 가장 가까운 살아 있는 플레이어와 거리의 제곱. 없으면 (nullptr, -1).
     * 위치는 이번 룸 틱이 시작할 때의 것이다.
     */
    pair<PlayerRef, float> FindClosestPlayer(const Protocol::PosInfo* posInfo, float range);

protected:
    /** 엔티티 위치로 셀 행렬을 다시 채운다. */
    void UpdateCellMatrix();

private:
    const float CELL_SIZE = 1000.f;    // 10M
    CellMatrix _cellMatrix;

    //~ 네트워크
public:
    /** 룸의 플레이어 전원에게 보낸다. exceptId가 0이 아니면 그 엔티티는 뺀다. 룸 큐 위에서만 부른다. */
    void Broadcast(SendBufferRef sendBuffer, int64 exceptId = 0);
};

