#pragma once
#include "JobQueue.h"

struct Pos
{
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

class Room : public JobQueue
{
public:
    Room();
	virtual ~Room();

public:
    void Init(const Json& roomData);
    void CacheRoomData();

	bool EnterRoom(ObjectRef object, bool moveRoom = false, bool randPos = false);
	bool LeaveRoom(ObjectRef object, bool moveRoom = false);

	bool HandleEnterPlayer(PlayerRef player, bool moveRoom = false);
	bool HandleLeavePlayer(PlayerRef player, bool moveRoom = false);

	void HandleMove(Protocol::C_MOVE pkt);
    void HandleEquipGear(Protocol::C_EQUIP_GEAR pkt, PlayerRef player);
    void HandleUnequipGear(Protocol::C_UNEQUIP_GEAR pkt, PlayerRef player);
    void HandleNormalAttack(Protocol::C_NORMAL_ATTACK pkt, PlayerRef player);

public:
	void UpdateTick();

    /* Room 정보 관련 */
	RoomRef GetRoomRef();
    int32 GetRoomId();
    optional<Json> GetPortalDataFromPortalId(int32 portalId);
    void SetupRandPos(Protocol::PosInfo* posInfo, bool randYaw = false);

private:
    /* Object 관리 관련*/
	bool RegisterObject(ObjectRef object);
	bool UnRegisterObject(uint64 objectId);

    void SpawnMonster(int32 templateId);

    /* 네트워크 관련 */
	void Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);

public:
    bool isValid = false;

private:
	unordered_map<uint64, ObjectRef> _objects;

    /** 해당 Room 관련 정보 */
    int32 _roomId;
    Json _roomData;

    Pos roomCenterPos;
    float widthHalfExtent;
    float heightHalfExtent;

    /** 몬스터 관련 정보 */
    int32 maxMonsterCount;
    float monsterRespawnTime;
    vector<int32> monsterIds;
};

