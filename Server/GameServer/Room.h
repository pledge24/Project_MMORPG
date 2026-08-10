#pragma once
#include "JobQueue.h"
#include "Utils.h"
#include "object.h"

struct RoomEnterData
{
    int32 nextRoomId = -1;
    Protocol::EnterType enterType = Protocol::ENTER_TYPE_NONE;
    optional<Protocol::PosInfo> enterPos;
};

using Cell = set<int64>;   // 특정 영역에 있는 ObjectId

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
    void TickObject(ObjectRef object);

    /** 플레이어 관련 함수 */
	bool EnterPlayer(PlayerRef enterPlayer, RoomEnterData roomEnterData);
    bool LeavePlayer(PlayerRef leavePlayer, bool transferRoom);
    bool TransferPlayer(PlayerRef player, RoomEnterData roomEnterData);

    /** 핸들 함수(Client Only) */
    void C_HandleEnterRoom(Protocol::C_ENTER_ROOM pkt, PlayerRef player);
    void C_HandleMove(Protocol::C_MOVE pkt);
    void C_HandleBuyItem(Protocol::C_BUY_ITEM pkt, PlayerRef player);
    void C_HandleSellItem(Protocol::C_SELL_ITEM pkt, PlayerRef player);
    void C_HandleUseItem(Protocol::C_USE_ITEM pkt, PlayerRef player);
    void C_HandleEquipGear(Protocol::C_EQUIP_GEAR pkt, PlayerRef player);
    void C_HandleUnequipGear(Protocol::C_UNEQUIP_GEAR pkt, PlayerRef player);
    void C_HandleNormalAttack(Protocol::C_NORMAL_ATTACK pkt, PlayerRef player);
    void C_HandleRespawn(Protocol::C_RESPAWN pkt, PlayerRef player);
    
    void HandleNormalAttack(int32 combo, CreatureRef creature);
    void HandleHit(ObjectRef attacker, Protocol::AttackInfo attackInfo);
    void HandleMonsterKill(PlayerRef player, MonsterRef monster);
    void HandleDie(CreatureRef creature);
    void HandleRespawn(PlayerRef player, Protocol::RespawnType& respawnType, Protocol::PosInfo& respawnPos);

    void ReplicateRoomData(PlayerRef player, bool excludeThisPlayer);

    /** Getter 함수(Public) */
    vector2D            GetRandomLocation(bool usePadding = true);
    float               GetRandomYaw() { return Utils::GetRandom(-180.f, 180.f); }
    RoomRef             GetRoomRef() { return static_pointer_cast<Room>(shared_from_this()); }
    int32               GetRoomId() const { return _roomId; }
    optional<Json>      GetPortalDataFromPortalId(int32 portalId);
    shared_ptr<Protocol::PosInfo> GetRespawnPoint() { return hasRespawnPoint ? respawnPoint : nullptr; }
    const vector3D&     GetCenterPoint() const { return _roomCenterPos; }

    /** Setter 함수 */
    void SetRandomPos(IN Protocol::PosInfo* posInfo, bool usePadding = true, bool randYaw = false);
    void SetValid(bool isValid) { _isValid = isValid; }

    /** Bool 함수 */
    bool IsValid() const { return _isValid; }
    bool Contains(int64 objectId) { return _objects.contains(objectId); }

    /** Room 위치 관련 */
    vector2D ClampLocation(float posX, float posY, bool usePadding = true);
    pair<PlayerRef, float> FindClosestPlayer(Protocol::PosInfo* posInfo, float range);  // pair<플레이어 참조, 거리^2> 

    /** 스폰 관련 */
    MonsterRef SpawnMonster(int32 templateId);
    PlayerRef SpawnPlayer(int64 objectId);
    PlayerRef SpawnPlayer(PlayerRef targetPlayer);

protected:
    /** 네트워크 함수 */
    void Broadcast(SendBufferRef sendBuffer, int64 exceptId = 0);

    /* Object 관련 함수*/
    bool AddObject(ObjectRef object);
    bool RemoveObject(int64 objectId);

    /** Room 관련 */
    void CacheRoomData();
    void CreateCellMatrix();
    void ClearCellMatrix();
    void UpdateCellMatrix();

    pair<int32, int32>      GetCellIndicesFromPos(const vector2D& pos);
    pair<int32, int32>      GetCellIndicesFromPos(Protocol::PosInfo* posInfo);
    Cell*                   GetCellFromPos(const vector2D& pos);
    Cell*                   GetCellFromPos(Protocol::PosInfo* posInfo);

private:
    /** Room 관련 */
	unordered_map<int64, ObjectRef> _objects;
    vector<vector<Cell>> _cellMatrix;
    vector2D _cellOffset = vector2D::GetZeroVector();

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

    bool hasRespawnPoint = false;
    shared_ptr<Protocol::PosInfo> respawnPoint;

    /** Config */
    const float LOCATION_PADDING_X = 1000.f;
    const float LOCATION_PADDING_Y = 1000.f;
    const float LOCATION_PADDING_Z = 100.f;

    const float CELL_SIZE = 1000.f;    // 10M
    const uint64 ROOM_UPDATE_INTERVAL_MS = 200;

    /** 몬스터 관련 정보 */
    int32 maxMonsterCount;
    float monsterRespawnTime;
    vector<int32> monsterIds;

};

