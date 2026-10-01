#pragma once
#include "JobQueue.h"
#include "Utils.h"
#include "Entity.h"
#include "PlayerSaveData.h"
#include "RoomTransfer.h"
#include "CellMatrix.h"

class Room : public JobQueue
{
public:
    Room() = default;
	virtual ~Room() = default;

public:
    static RoomRef Create(const Json& roomData);
    bool Init(const Json& roomData);
    bool Start();

protected:
    void Update();

public:
    void TickEntity(EntityRef entity);

    /** 플레이어 관련 함수 */
	bool EnterPlayer(PlayerRef enterPlayer, RoomEnterData roomEnterData);
    bool LeavePlayer(PlayerRef leavePlayer, bool transferRoom);
    bool TransferPlayer(PlayerRef player, RoomEnterData roomEnterData);
    optional<PlayerSaveData> HandleDisconnect(PlayerRef player);

    /** 핸들 함수(Client Only) */
    void C_HandleEnterMap(Protocol::C_ENTER_MAP pkt, PlayerRef player);
    void C_HandleEnterRoom(Protocol::C_ENTER_ROOM pkt, PlayerRef player);
    void C_HandleMove(Protocol::C_MOVE pkt);
    void C_HandleBuyItem(Protocol::C_BUY_ITEM pkt, PlayerRef player);
    void C_HandleSellItem(Protocol::C_SELL_ITEM pkt, PlayerRef player);
    void C_HandleUseItem(Protocol::C_USE_ITEM pkt, PlayerRef player);
    void C_HandleEquipGear(Protocol::C_EQUIP_GEAR pkt, PlayerRef player);
    void C_HandleUnequipGear(Protocol::C_UNEQUIP_GEAR pkt, PlayerRef player);
    void C_HandleNormalAttack(Protocol::C_NORMAL_ATTACK pkt, PlayerRef player);
    void C_HandleRespawn(Protocol::C_RESPAWN pkt, PlayerRef player);
    void C_HandleChat(Protocol::C_CHAT pkt, PlayerRef player);

    void HandleNormalAttack(int32 combo, CreatureRef creature);
    void HandleHit(EntityRef attacker, Protocol::AttackInfo attackInfo);
    void HandleMonsterKill(PlayerRef player, const Protocol::Reward& reward);
    void HandleDie(CreatureRef creature);
    // 리스폰에 성공하면 true. 실패 응답은 이 함수가 보낸다.
    bool HandleRespawn(PlayerRef player, Protocol::RespawnType respawnType, Protocol::PosInfo respawnPos);

    // includeThisPlayer: 자기 자신의 EntityInfo도 S_SPAWN에 포함할지.
    // 클라 월드가 비어 있는 최초 입장·맵 간 이동에서는 true, 액터가 살아 있는 경우 false.
    void ReplicateRoomData(PlayerRef player, bool includeThisPlayer);

    /** Getter 함수(Public) */
    vector2D            GetRandomLocation(bool usePadding = true);
    float               GetRandomYaw() { return Utils::GetRandom(-180.f, 180.f); }
    RoomRef             GetRoomRef() { return static_pointer_cast<Room>(shared_from_this()); }
    int32               GetRoomId() const { return _roomId; }
    optional<Json>      GetPortalDataFromPortalId(int32 portalId);
    shared_ptr<Protocol::PosInfo> GetRespawnPoint() { return _hasRespawnPoint ? _respawnPoint : nullptr; }
    const vector3D&     GetCenterPoint() const { return _roomCenterPos; }

    /** Setter 함수 */
    void SetRandomPos(IN Protocol::PosInfo* posInfo, bool usePadding = true, bool randYaw = false);
    void SetValid(bool isValid) { _isValid = isValid; }

    /** Bool 함수 */
    bool IsValid() const { return _isValid; }
    bool Contains(int64 entityId) { return _entities.contains(entityId); }

    /** Room 위치 관련 */
    pair<PlayerRef, float> FindClosestPlayer(Protocol::PosInfo* posInfo, float range);  // pair<플레이어 참조, 거리^2> 

    /** 스폰 관련 */
    MonsterRef SpawnMonster(int32 templateId);
    PlayerRef SpawnPlayer(int64 entityId);
    PlayerRef SpawnPlayer(PlayerRef targetPlayer);

protected:
    /** 네트워크 함수 */
    void Broadcast(SendBufferRef sendBuffer, int64 exceptId = 0);

    /** 엔티티 관련 함수 */
    bool AddEntity(EntityRef entity);
    bool RemoveEntity(int64 entityId);

    /** 엔티티를 찾아 T로 내린다. 없거나 T가 아니면 nullptr */
    template<typename T>
    shared_ptr<T> FindEntityAs(int64 entityId)
    {
        auto it = _entities.find(entityId);
        if (it == _entities.end())
            return nullptr;

        return dynamic_pointer_cast<T>(it->second);
    }

    /** Room 관련 */
    void CacheRoomData();
    // 엔티티 위치로 셀 행렬을 다시 채운다.
    void UpdateCellMatrix();

private:
    /** Room 관련 */
	unordered_map<int64, EntityRef> _entities;
    CellMatrix _cellMatrix;

    int32 _roomId;
    Json _roomData;
    bool _isValid = false;

    vector3D _roomCenterPos;
    float _widthHalfExtent;
    float _heightHalfExtent;

    float _roomMinX;
    float _roomMaxX;
    float _roomMinY;
    float _roomMaxY;

    bool _hasRespawnPoint = false;
    shared_ptr<Protocol::PosInfo> _respawnPoint;

    /** Config */
    const float LOCATION_PADDING_X = 1000.f;
    const float LOCATION_PADDING_Y = 1000.f;
    const float LOCATION_PADDING_Z = 100.f;

    const float CELL_SIZE = 1000.f;    // 10M
    const uint64 ROOM_UPDATE_INTERVAL_MS = 200;

    /** 몬스터 관련 정보 */
    int32 _maxMonsterCount;
    float _monsterRespawnTime;
    vector<int32> _monsterIds;

};

