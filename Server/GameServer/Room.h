#pragma once
#include "JobQueue.h"

class Room : public JobQueue
{
public:
    Room();
	virtual ~Room();

public:
    void Init(const Json& roomData);

	bool EnterRoom(ObjectRef object, bool moveRoom, bool randPos);
	bool LeaveRoom(ObjectRef object, bool moveRoom);

	bool HandleEnterPlayer(PlayerRef player, bool moveRoom = false);
	bool HandleLeavePlayer(PlayerRef player, bool moveRoom = false);

	void HandleMove(Protocol::C_MOVE pkt);
    void HandleEquipGear(Protocol::C_EQUIP_GEAR pkt, PlayerRef player);
    void HandleUnequipGear(Protocol::C_UNEQUIP_GEAR pkt, PlayerRef player);
    void HandleNormalAttack(Protocol::C_NORMAL_ATTACK pkt);

public:
	void UpdateTick();

    /* Room 정보 관련 */
	RoomRef GetRoomRef();
    int32 GetRoomId();
    optional<Json> GetPortalDataFromPortalId(int32 portalId);
    void SetupRandPos(Protocol::PosInfo* posInfo, bool randYaw = false);

private:
    /* Object 관리 관련 */
	bool AddObject(ObjectRef object);
	bool RemoveObject(uint64 objectId);

    /* 네트워크 관련 */
	void Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);

public:
    bool isValid = false;

private:
	unordered_map<uint64, ObjectRef> _objects;

    int32 _roomId;
    Json _roomData;
};

