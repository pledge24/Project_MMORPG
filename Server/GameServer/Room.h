#pragma once
#include "JobQueue.h"

class Room : public JobQueue
{
public:
	Room();
	virtual ~Room();

public:
	bool EnterRoom(ObjectRef object, bool randPos = true);
	bool LeaveRoom(ObjectRef object);

	bool HandleEnterPlayer(PlayerRef player);
	bool HandleLeavePlayer(PlayerRef player);
	void HandleMove(Protocol::C_MOVE pkt);
    void HandleEquipGear(Protocol::C_EQUIP_GEAR pkt, PlayerRef player);
    void HandleUnequipGear(Protocol::C_UNEQUIP_GEAR pkt, PlayerRef player);


public:
	void UpdateTick();

	RoomRef GetRoomRef();

private:
	bool AddObject(ObjectRef object);
	bool RemoveObject(uint64 objectId);

private:
	void Broadcast(SendBufferRef sendBuffer, uint64 exceptId = 0);

private:
	unordered_map<uint64, ObjectRef> _objects;
};

extern RoomRef GRoom;