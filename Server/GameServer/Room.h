#pragma once
#include "JobQueue.h"
#include "Utils.h"

class Room : public JobQueue
{
public:
    Room() = default;
	virtual ~Room() = default;

protected:
    void UpdateTick();

public:
    void Init(const Json& roomData);
    void CacheRoomData();

    /** 핸들 함수 */
	void HandleEnterPlayer(PlayerRef enterPlayer, shared_ptr<Protocol::PosInfo> enterPos, bool moveRoom = false);
    void HandleLeavePlayer(PlayerRef leavePlayer, bool moveRoom = false);

	void HandleMove(Protocol::C_MOVE pkt);
    void HandleEquipGear(Protocol::C_EQUIP_GEAR pkt, PlayerRef player);
    void HandleUnequipGear(Protocol::C_UNEQUIP_GEAR pkt, PlayerRef player);
    void HandleNormalAttack(Protocol::C_NORMAL_ATTACK pkt, PlayerRef player);

    /** Getter 함수 */
	RoomRef GetRoomRef() { return static_pointer_cast<Room>(shared_from_this()); }
    int32 GetRoomId() const { return _roomId; }
    optional<Json> GetPortalDataFromPortalId(int32 portalId);

    /** Setter 함수 */
    void SetRandomPos(Protocol::PosInfo* posInfo, bool usePadding = true, bool randYaw = false);
    void SetValid(bool isValid) { _isValid = isValid; }

    bool IsValid() const { return _isValid; }

    /** Room 위치 관련 */
    vector2D ClampLocation(float posX, float posY, bool usePadding = true);

protected:
    /* Object 관리 관련*/
	bool RegisterObject(ObjectRef object);
	bool UnRegisterObject(uint64 objectId);

    MonsterRef SpawnMonster(int32 templateId);

    /* 네트워크 관련 */
	void Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);

private:
	unordered_map<uint64, ObjectRef> _objects;

    /** 해당 Room 관련 정보 */
    int32 _roomId;
    Json _roomData;
    bool _isValid = false;

    vector3D roomCenterPos;
    float widthHalfExtent;
    float heightHalfExtent;

    /** 몬스터 관련 정보 */
    int32 maxMonsterCount;
    float monsterRespawnTime;
    vector<int32> monsterIds;

    const float LOCATION_PADDING_X = 1000.f;
    const float LOCATION_PADDING_Y = 1000.f;
    const float LOCATION_PADDING_Z = 100.f;

    /** 기타 */
    uint64 prevTickTime = GetTickCount64();
    const uint64 ROOM_TICK = 500;
    const float SEND_MOVE_PACKET_TIME = 1.f;
    float elapsedTime = 0.f;
};

