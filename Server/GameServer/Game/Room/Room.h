#pragma once
#include "Game/Entities/Entity.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Entities/PlayerSaveData.h"
#include "Game/Room/RoomTransfer.h"
#include "Game/Room/CellMatrix.h"

/**
 * GameServer가 동기화를 처리하는 최소 공간 단위.
 * 속한 Entity를 소유하며, 룸 틱이 ROOM_TICK_INTERVAL_MS마다 모든 엔티티의 Tick을 돌린다(ADR-0011).
 * 모든 작업(네트워크를 통한 작업, Room 내부 작업 등)은 Job 객체 단위로 처리되며, JobQueue에 꺼내 처리한다.
 * Room 클래스 자체는 중계 역할만 하며, 대부분의 작업은 각 컴포넌트에서 처리한다.
 */
class Room : public JobQueue
{
public:
    Room() = default;
	virtual ~Room() = default;

public:
    /** 룸을 만들어 Init까지 마친다. 룸을 만드는 길은 이것 하나다. Init에 실패하면 nullptr. */
    static RoomRef Create(const MapTemplate& mapTemplate);
    /** 몬스터를 스폰하고 첫 룸 틱을 예약한다. 스폰에 실패하면 false. */
    bool Start();

    //~ 룸 틱
    /**
     * 룸 틱 하나를 룸 큐 위에서 돈다. 순서는 셀 갱신, 엔티티 Tick, 이동 전송(MOVE_SEND_INTERVAL마다)이다.
     * deltaTime은 초 단위다. 다음 틱을 예약하지 않는다. 운영 코드는 예약된 틱(RunScheduledTick)이 부르고, 테스트는 직접 부른다.
     */
    void Tick(float deltaTime);

    //~ 플레이어 입장과 퇴장
    /**
     * 플레이어를 룸에 넣고 S_ENTER_ROOM을 보낸다. 실패해도 실패 응답을 이 함수가 보낸다.
     * 그사이 접속이 끊겼으면 퇴장과 저장을 이어 받고 false를 돌려준다.
     */
	bool EnterPlayer(PlayerRef enterPlayer, RoomEnterData roomEnterData);
    /** 다른 플레이어에게 S_DESPAWN을 알린다. transferRoom이 false면 떠나는 본인에게도 보낸다. */
    bool LeavePlayer(PlayerRef leavePlayer, bool transferRoom);
    /** 이 룸에서 퇴장시키고 목적지 룸 큐에 EnterPlayer를 넣는다. */
    bool TransferPlayer(PlayerRef player, RoomEnterData roomEnterData);
    /** 연결이 끊인 플레이어를 처리한다. 해당 플레이어 Entity를 제거하고, DB에 SaveData 저장을 요청한다. */
    optional<PlayerSaveData> HandleDisconnect(PlayerRef player);

    //~ 클라이언트 패킷 핸들러
    /**
     * 아래 핸들러는 모두 ServerPacketHandler가 DoAsync로 넣는다. 잡이 도는 시점에 세션이 끊겼을 수 있다.
     * 룸에는 룸 상태를 쓰는 요청(입장, 이동, 전투, 리스폰)만 둔다. 아이템 요청은 ItemRequests에 있다.
     */
    /**
     * 판정을 통과하면 들어갈 맵과 룸을 플레이어에 기록하고 S_ENTER_MAP을 보낸다. 룸 입장은 뒤이은 C_ENTER_ROOM이 한다.
     * 이 룸에 있는 플레이어는 이 룸의 포털로, 아직 룸에 들어간 적이 없는 플레이어는 불러온 룸으로 판정한다.
     */
    void C_HandleEnterMap(Protocol::C_ENTER_MAP pkt, PlayerRef player);
    void C_HandleEnterRoom(Protocol::C_ENTER_ROOM pkt, PlayerRef player);
    /** 보낸 사람(player)의 위치만 바꾼다. 패킷의 엔티티 번호는 보지 않는다. */
    void C_HandleMove(Protocol::C_MOVE pkt, PlayerRef player);
    void C_HandleNormalAttack(Protocol::C_NORMAL_ATTACK pkt, PlayerRef player);
    void C_HandleRespawn(Protocol::C_RESPAWN pkt, PlayerRef player);

    //~ 전투와 리스폰
    /** 공격 사실만 브로드캐스트한다. 피격 판정은 HandleHit이 따로 한다. */
    void HandleNormalAttack(int32 combo, CreatureRef creature);
    /** 공격자가 그사이 룸을 떠났으면 공격을 버린다. 대상이 죽으면 보상과 사망까지 처리한다. */
    void HandleHit(EntityRef attacker, Protocol::AttackInfo attackInfo);
    void HandleMonsterKill(PlayerRef player, const Protocol::Reward& reward);
    /** 몬스터는 룸에서 뺀다. 플레이어는 리스폰할 때까지 룸에 남긴다. */
    void HandleDie(CreatureRef creature);
    /** 리스폰에 성공하면 true. 실패 응답은 이 함수가 보낸다. 세션이 끊겼으면 아무것도 보내지 않고 false. */
    bool HandleRespawn(PlayerRef player, Protocol::RespawnType respawnType, Protocol::PosInfo respawnPos);

    /**
     * 현재 Room 정보를 S_SPAWN으로 플레이어에게 보낸다.
     * includeThisPlayer가 false면 요청한 플레이어 정보를 제외하고 보낸다.
     */
    void ReplicateRoomData(PlayerRef player, bool includeThisPlayer);

    //~ Room 정보 관련
    /** usePadding이면 룸 경계에서 LOCATION_PADDING만큼 안쪽에서 고른다. */
    vector2D            GetRandomLocation(bool usePadding = true);
    /** -180도에서 180도 사이. */
    float               GetRandomYaw() { return Utils::GetRandom(-180.f, 180.f); }
    RoomRef             GetRoomRef() { return static_pointer_cast<Room>(shared_from_this()); }
    /** Init 뒤로는 바뀌지 않으므로 룸 큐 밖에서 읽어도 된다. */
    int32               GetRoomId() const { return _roomId; }
    /** 이 룸에 그 번호의 포털이 없으면 nullptr. */
    const PortalTemplate* FindPortal(int32 portalId) const;
    const vector3D&     GetCenterPoint() const { return _roomCenterPos; }

    /** z는 룸 중심 높이에 LOCATION_PADDING_Z를 더한 값이다. randYaw가 false면 yaw를 건드리지 않는다. */
    void SetRandomPos(IN Protocol::PosInfo* posInfo, bool usePadding = true, bool randYaw = false);

    //~ 상태 조회
    bool Contains(int64 entityId) { return _entities.contains(entityId); }

    //~ 위치 탐색
    /**
     * range 안에서 가장 가까운 살아 있는 플레이어와 그 거리의 제곱을 돌려준다.
     * 없으면 (nullptr, -1)이다. 셀 행렬은 룸 틱이 엔티티 Tick 전에 다시 채우므로 위치는 그 틱이 시작할 때의 것이다.
     */
    pair<PlayerRef, float> FindClosestPlayer(const Protocol::PosInfo* posInfo, float range);

    //~ 스폰
    /**
     * 엔티티를 만들어 이 룸에 넣는다. 언리얼의 UWorld::SpawnActor에 대응한다.
     * AddEntity가 Start까지 부른다. T의 정의가 보이는 곳에서 부른다. 생성이나 등록에 실패하면 nullptr.
     */
    template<typename T>
    shared_ptr<T> SpawnEntity(const typename T::SpawnParams& params)
    {
        shared_ptr<T> entity = EntityFactory::Create<T>(params);
        if (entity == nullptr)
            return nullptr;

        if (AddEntity(entity) == false)
        {
            wcout << L"Room " << _roomId << L": 엔티티 " << entity->GetEntityId() << L" 등록에 실패했습니다" << '\n';
            return nullptr;
        }

        return entity;
    }
    /** 다른 플레이어에게만 S_SPAWN을 알린다(본인 제외). 이 룸에 없는 플레이어면 알리지 않고 nullptr. */
    PlayerRef SpawnPlayer(int64 entityId);
    /** 다른 플레이어에게만 S_SPAWN을 알린다(본인 제외). 이 룸에 없는 플레이어면 알리지 않고 nullptr. */
    PlayerRef SpawnPlayer(PlayerRef targetPlayer);

    //~ 네트워크
    /** 룸의 플레이어 전원에게 보낸다. exceptId가 0이 아니면 그 엔티티는 뺀다. 룸 큐 위에서만 부른다. */
    void Broadcast(SendBufferRef sendBuffer, int64 exceptId = 0);

protected:

    //~ 엔티티
    /**
     * 이미 있는 id면 false. 셀 행렬에는 다음 룸 틱에서 들어간다.
     * 엔티티의 _room을 이 룸으로 바꾸고, 처음 룸에 들어가는 엔티티면 Start를 부른다.
     */
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

    //~ 룸 데이터
    /** Create만 부른다. 맵 표의 행을 읽어 두고 셀 행렬을 만든다. */
    bool Init(const MapTemplate& mapTemplate);
    /** 맵 데이터에서 자주 읽는 값을 멤버로 옮겨 둔다. Init에서 한 번 부른다. */
    void CacheRoomData();
    /** 엔티티 위치로 셀 행렬을 다시 채운다. */
    void UpdateCellMatrix();

    //~ 룸 틱
    /** 지난 틱부터 흐른 시간으로 Tick을 부르고 다음 틱을 예약한다. */
    void RunScheduledTick();
    /** 몬스터의 위치를 S_MOVE 하나로 모아 룸 전체에 보낸다. 몬스터가 없으면 보내지 않는다. */
    void BroadcastMonsterMoves();

private:
    //~ 룸 상태
    /** 룸이 엔티티의 shared_ptr를 붙잡는다. 여기서 빠지면 다른 곳이 들고 있지 않는 한 엔티티가 사라진다. */
	unordered_map<int64, EntityRef> _entities;
    CellMatrix _cellMatrix;

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

    /** 맵 데이터에 리스폰 지점이 없는 룸이면 nullptr. */
    shared_ptr<Protocol::PosInfo> _respawnPoint;

    //~ 설정값
    const float LOCATION_PADDING_X = 1000.f;
    const float LOCATION_PADDING_Y = 1000.f;
    const float LOCATION_PADDING_Z = 100.f;

    const float CELL_SIZE = 1000.f;    // 10M
    const uint64 ROOM_TICK_INTERVAL_MS = 50;
    /** 몬스터 위치를 보내는 주기. 단위는 초다. 룸 틱보다 길다. */
    const float MOVE_SEND_INTERVAL = 0.2f;

    //~ 룸 틱 상태
    /** 마지막 예약 틱의 시각(ms, GetTickCount64 기준). */
    uint64 _lastTickTime = 0;
    /** 마지막 이동 전송 뒤 흐른 시간. 단위는 초다. */
    float _timeSinceMoveSend = 0.f;

    //~ 몬스터 스폰 정보
    int32 _maxMonsterCount;
    float _monsterRespawnTime;
    vector<int32> _monsterIds;

};

