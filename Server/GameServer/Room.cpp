#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include "Monster.h"
#include "ObjectUtils.h"
#include "EquippedGear.h"

// TEMP: Room 하나만 운영(나중엔 RoomManager 사용해서 관리)
RoomRef GRoom = make_shared<Room>();

Room::Room()
{

}

Room::~Room()
{

}

bool Room::EnterRoom(ObjectRef object, bool randPos /*= true*/)
{
	bool success = AddObject(object);

	// 랜덤 위치
	if (randPos)
	{
		object->posInfo->set_x(Utils::GetRandom(0.f, 500.f));
		object->posInfo->set_y(Utils::GetRandom(0.f, 500.f));
		object->posInfo->set_z(100.f);
		object->posInfo->set_yaw(Utils::GetRandom(0.f, 100.f));
	}

	// 입장 사실을 신입 플레이어에게 알린다
	//if (auto player = dynamic_pointer_cast<Player>(object))
	//{
	//	Protocol::S_ENTER_GAME enterGamePkt;
	//	enterGamePkt.set_success(success);

	//	Protocol::ObjectInfo* playerInfo = new Protocol::ObjectInfo();
	//	playerInfo->CopyFrom(*object->objectInfo);
	//	enterGamePkt.set_allocated_player(playerInfo);

	//	SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(enterGamePkt);
	//	if (auto session = player->session.lock())
	//		session->Send(sendBuffer);
	//}

	// 다른 플레이어에게 현재 플레이어 Spawn Broadcast
	{
		Protocol::S_SPAWN spawnPkt;

		Protocol::ObjectInfo* objectInfo = spawnPkt.add_objects();
		objectInfo->CopyFrom(*object->objectInfo);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(spawnPkt);
		Broadcast(sendBuffer, object->objectInfo->object_id());
	}

	// 방에 있던 다른 플레이어 목록을 현재 플레이어한테 전송
	if (auto player = dynamic_pointer_cast<Player>(object))
	{
		Protocol::S_SPAWN spawnPkt;

		for (auto& item : _objects)
		{
			if (item.second->IsPlayer() == false)
				continue;

			Protocol::ObjectInfo* playerInfo = spawnPkt.add_objects();
			playerInfo->CopyFrom(*item.second->objectInfo);
		}

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(spawnPkt);
		if (auto session = player->session.lock())
			session->Send(sendBuffer);
	}

	return success;
}

bool Room::LeaveRoom(ObjectRef object)
{
	if (object == nullptr)
		return false;

	const uint64 objectId = object->objectInfo->object_id();
	bool success = RemoveObject(objectId);

	// 퇴장 사실을 퇴장하는 플레이어에게 알린다
	if (auto player = dynamic_pointer_cast<Player>(object))
	{
		Protocol::S_LEAVE_GAME leaveGamePkt;

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(leaveGamePkt);
		if (auto session = player->session.lock())
			session->Send(sendBuffer);
	}

	// 퇴장 사실을 다른 플레이어에게 알린다
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_object_ids(objectId);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(despawnPkt);
		Broadcast(sendBuffer, objectId);

		if (auto player = dynamic_pointer_cast<Player>(object))
			if (auto session = player->session.lock())
				session->Send(sendBuffer);
	}

	return success;
}

bool Room::HandleEnterPlayer(PlayerRef player)
{
	return EnterRoom(player, false);
}

bool Room::HandleLeavePlayer(PlayerRef player)
{
	return LeaveRoom(player);
}

void Room::HandleMove(Protocol::C_MOVE pkt)
{
	const uint64 objectId = pkt.info().object_id();
	if (_objects.find(objectId) == _objects.end())
		return;

	// 적용
	PlayerRef player = dynamic_pointer_cast<Player>(_objects[objectId]);
	player->posInfo->CopyFrom(pkt.info());

	// 이동 사실을 알린다 (본인 포함? 빼고?)
	{
		Protocol::S_MOVE movePkt;
		{
			Protocol::PosInfo* info = movePkt.mutable_info();
			info->CopyFrom(pkt.info());
		}
		SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(movePkt);
		Broadcast(sendBuffer);
	}
}

void Room::HandleEquipGear(Protocol::C_EQUIP_GEAR pkt, PlayerRef player)
{
    const uint64 objectId = player->objectInfo->object_id();
    if (_objects.find(objectId) == _objects.end())
        return;

    Protocol::S_EQUIP_GEAR rPkt;
    rPkt.set_object_id(objectId);

    if (player->EquipGear(OUT rPkt, pkt.mutable_slot()) == false)
        return;

    // 장착한 유저에게만 그대로 전송.
    {
        SessionRef session = player->session.lock();
        SEND_PACKET(rPkt);
    }

    // 다른 유저들한테는 변경된 stat을 보내지 않는다.
    {
        rPkt.clear_updated_stat_info();
        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(rPkt);
        Broadcast(sendBuffer, objectId);
    }
}

void Room::HandleUnequipGear(Protocol::C_UNEQUIP_GEAR pkt, PlayerRef player)
{
    const uint64 objectId = player->objectInfo->object_id();
    if (_objects.find(objectId) == _objects.end())
        return;

    Protocol::S_UNEQUIP_GEAR rPkt;
    rPkt.set_object_id(objectId);

    if (player->UnequipGear(OUT rPkt, pkt.mutable_slot()) == false)
        return;

    // 탈착한 유저에게만 그대로 전송.
    {
        SessionRef session = player->session.lock();
        SEND_PACKET(rPkt);
    }

    // 다른 유저들한테는 변경된 stat을 보내지 않는다.
    {
        rPkt.clear_updated_stat_info();
        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(rPkt);
        Broadcast(sendBuffer, objectId);
    }
}

void Room::UpdateTick()
{
	cout << "Update Room" << endl;

	DoTimer(100, &Room::UpdateTick);
}

RoomRef Room::GetRoomRef()
{
	return static_pointer_cast<Room>(shared_from_this());
}

bool Room::AddObject(ObjectRef object)
{
	// 있다면 문제가 있다.
	if (_objects.find(object->objectInfo->object_id()) != _objects.end())
		return false;

	_objects.insert(make_pair(object->objectInfo->object_id(), object));

	object->room.store(GetRoomRef());

	return true;
}

bool Room::RemoveObject(uint64 objectId)
{
	// 없다면 문제가 있다.
	if (_objects.find(objectId) == _objects.end())
		return false;

	ObjectRef object = _objects[objectId];
	PlayerRef player = dynamic_pointer_cast<Player>(object);
	if (player)
		player->room.store(weak_ptr<Room>());

	_objects.erase(objectId);

	return true;
}

void Room::Broadcast(SendBufferRef sendBuffer, uint64 exceptId)
{
	for (auto& item : _objects)
	{
		PlayerRef player = dynamic_pointer_cast<Player>(item.second);
		if (player == nullptr)
			continue;
		if (player->objectInfo->object_id() == exceptId)
			continue;

		if (GameSessionRef session = player->session.lock())
			session->Send(sendBuffer);
	}
}