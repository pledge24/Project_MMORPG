#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include "Monster.h"
#include "ObjectUtils.h"
#include "EquippedGear.h"

Room::Room()
{
}


Room::~Room()
{

}

void Room::Init(const Json& roomData)
{
    // roomData가 들어있는 지 확인한다.
    if (roomData.empty())
        throw wstring(L"roomData가 없습니다");

    _roomId = roomData[JsonProperty::Map::TemplateId];
    _roomData = roomData;

    cout << "roomId: " << _roomId << endl;
    cout << _roomData.dump(2) << endl;

    // 몬스터를 room에 스폰한다.
}

bool Room::EnterRoom(ObjectRef object, bool moveRoom, bool randPos)
{
    // 현재 방에 해당 Object를 추가
    if (AddObject(object) == false)
        return false;

    // randPos
    if (randPos)
    {
        SetupRandPos(object->posInfo, true);
    }

	// 이 Room에 있는 다른 Player들에게 Object Spawn Broadcast
	{
		Protocol::S_SPAWN spawnPkt;

		Protocol::ObjectInfo* objectInfo = spawnPkt.add_objects();
		objectInfo->CopyFrom(*object->objectInfo);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(spawnPkt);
        Broadcast(sendBuffer, object->objectInfo->object_id());
	}

    // 추가된 Object가 Player인 경우
	if (auto player = dynamic_pointer_cast<Player>(object))
	{
        // 이 Room에 있는 모든 Object들을 가져온다.(본인 제외)
        RepeatedPtrField<Protocol::ObjectInfo> roomObjects;
        {
            uint64 playerId = object->objectInfo->object_id();

            for (auto& item : _objects)
            {
                if (item.second->objectInfo->object_id() == playerId)
                    continue;

                Protocol::ObjectInfo* objectInfo = roomObjects.Add();
                objectInfo->CopyFrom(*item.second->objectInfo);
            }
        }

        // 2) 입장한 플레이어가 단순 Room 이동이면 S_MOVE_ROOM, 새로 입장이면 S_SPAWN
        if (moveRoom == true)
        {
            Protocol::S_MOVE_ROOM moveRoomPkt;
            moveRoomPkt.set_map_id(player->objectInfo->map_id());
            moveRoomPkt.mutable_info()->CopyFrom(*player->posInfo);
            moveRoomPkt.mutable_objects()->Swap(&roomObjects);

            SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(moveRoomPkt);
            if (auto session = player->session.lock())
                session->Send(sendBuffer);
        }
        else
        {
            // Spawn할 Object에 본인을 추가
            Protocol::ObjectInfo* myObjectInfo = roomObjects.Add();
            myObjectInfo->CopyFrom(*object->objectInfo);
            
            Protocol::S_SPAWN spawnPkt;
            spawnPkt.mutable_objects()->Swap(&roomObjects);

            SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(spawnPkt);
            if (auto session = player->session.lock())
                session->Send(sendBuffer);
        }
	}

	return true;
}

bool Room::LeaveRoom(ObjectRef object, bool moveRoom /*false*/)
{
    if (object == nullptr)
		return false;

    const uint64 objectId = object->objectInfo->object_id();

    // 현재 방에 해당 Object를 제거
    if (RemoveObject(objectId) == false)
        return false;

    // 퇴장한 플레이어가 방 이동이 아닌 경우, Despawn 패킷 전송
    if(PlayerRef player = dynamic_pointer_cast<Player>(object)){

        if (moveRoom == false)
        {
            Protocol::S_DESPAWN despawnPkt;
            despawnPkt.add_object_ids(objectId);

            SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(despawnPkt);
            if (auto session = player->session.lock())
                session->Send(sendBuffer);
        }
    }

	// 이 Room에 있는 모든 Player들에게 Object Despawn Broadcast
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_object_ids(objectId);

		SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(despawnPkt);

        Broadcast(sendBuffer);
	}

	return true;
}

bool Room::HandleEnterPlayer(PlayerRef player, bool moveRoom)
{
	return EnterRoom(player, moveRoom, false);
}

bool Room::HandleLeavePlayer(PlayerRef player, bool moveRoom)
{
	return LeaveRoom(player, moveRoom);
}

void Room::HandleMove(Protocol::C_MOVE pkt)
{
	const uint64 objectId = pkt.info().object_id();
	if (_objects.find(objectId) == _objects.end())
		return;

	// 적용
	PlayerRef player = dynamic_pointer_cast<Player>(_objects[objectId]);
	player->posInfo->CopyFrom(pkt.info());

	// 이동 사실을 알린다 (본인 빼고)
	{
		Protocol::S_MOVE movePkt;
		{
			Protocol::PosInfo* info = movePkt.mutable_info();
			info->CopyFrom(pkt.info());
		}
		SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(movePkt);
		Broadcast(sendBuffer, objectId);
	}
}

void Room::HandleEquipGear(Protocol::C_EQUIP_GEAR pkt, PlayerRef player)
{
    const uint64 objectId = player->objectInfo->object_id();
    if (_objects.find(objectId) == _objects.end())
        return;

    Protocol::S_EQUIP_GEAR equipGearPkt;
    equipGearPkt.set_object_id(objectId);

    if (player->HandleEquipGear(OUT equipGearPkt, pkt.mutable_slot()) == false)
    {
        SessionRef session = player->session.lock();
        equipGearPkt.set_success(false);
        SEND_PACKET(equipGearPkt);
        return;
    }

    equipGearPkt.set_success(true);

    // 장착한 유저에게만 그대로 전송.
    {
        SessionRef session = player->session.lock();
        cout << equipGearPkt.DebugString() << endl;
        SEND_PACKET(equipGearPkt);
    }

    // 다른 유저들한테는 변경된 stat을 보내지 않는다.
    {
        equipGearPkt.clear_updated_inventory_slot();
        equipGearPkt.clear_updated_stat_info();
        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(equipGearPkt);
        Broadcast(sendBuffer, objectId);
    }
}

void Room::HandleUnequipGear(Protocol::C_UNEQUIP_GEAR pkt, PlayerRef player)
{
    const uint64 objectId = player->objectInfo->object_id();
    if (_objects.find(objectId) == _objects.end())
        return;

    Protocol::S_UNEQUIP_GEAR unequipGearPkt;
    unequipGearPkt.set_object_id(objectId);

    if (player->HandleUnequipGear(OUT unequipGearPkt, pkt.mutable_slot()) == false)
    {
        SessionRef session = player->session.lock();
        unequipGearPkt.set_success(false);
        SEND_PACKET(unequipGearPkt);
        return;
    }

    unequipGearPkt.set_success(true);

    // 탈착한 유저에게만 그대로 전송.
    {
        SessionRef session = player->session.lock();
        SEND_PACKET(unequipGearPkt);
    }

    // 다른 유저들한테는 변경된 stat을 보내지 않는다.
    {
        unequipGearPkt.clear_updated_inventory_slot();
        unequipGearPkt.clear_updated_stat_info();
        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(unequipGearPkt);
        Broadcast(sendBuffer, objectId);
    }
}

void Room::HandleNormalAttack(Protocol::C_NORMAL_ATTACK pkt, PlayerRef player)
{
    const uint64 objectId = player->objectInfo->object_id();
    if (_objects.find(objectId) == _objects.end())
        return;
    
    // 일반 공격 사실을 알린다 (본인 빼고)
    {
        Protocol::S_NORMAL_ATTACK normalAttackPkt;
        {
            normalAttackPkt.set_object_id(objectId);
            normalAttackPkt.set_combo(pkt.combo());
        }
        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(normalAttackPkt);
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

int32 Room::GetRoomId()
{
    return _roomId;
}

optional<Json> Room::GetPortalDataFromPortalId(int32 portalId)
{
    using namespace JsonProperty::Map;

    const Json& portalList = _roomData[Portals][Lists];
    if(portalList.size() == 0)
        return nullopt;

    for (const auto& portal : portalList)
    {
        if (portal[JsonProperty::Map::PortalId] == portalId)
            return portal;
    }

    return nullopt;
}

void Room::SetupRandPos(Protocol::PosInfo* posInfo, bool randYaw)
{
    using namespace JsonProperty::Map;

    float centerPosX = _roomData[CenterPos][PosX];
    float centerPosY = _roomData[CenterPos][PosY];
    float centerPosZ = _roomData[CenterPos][PosZ];

    float widthHalfExtent = _roomData[WidthHalfExtent];
    float heightHalfExtent = _roomData[HeightHalfExtent];

    posInfo->set_x(Utils::GetRandom(centerPosX - widthHalfExtent, centerPosX + widthHalfExtent));
    posInfo->set_y(Utils::GetRandom(centerPosY - heightHalfExtent, centerPosY + heightHalfExtent));
    posInfo->set_z(centerPosZ + 10.f);

    if(randYaw)
        posInfo->set_yaw(Utils::GetRandom(-180.f, 180.f));
}

bool Room::AddObject(ObjectRef object)
{
	// 있다면 문제가 있다.
	if (_objects.find(object->objectInfo->object_id()) != _objects.end())
		return false;

	_objects.insert(make_pair(object->objectInfo->object_id(), object));

	object->room.store(GetRoomRef()); // set Last RoomRef

	return true;
}

bool Room::RemoveObject(uint64 objectId)
{
	// 없다면 문제가 있다.
	if (_objects.find(objectId) == _objects.end())
		return false;

	ObjectRef object = _objects[objectId];
	PlayerRef player = dynamic_pointer_cast<Player>(object);

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