#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include "Monster.h"
#include "ObjectUtils.h"
#include "EquippedGear.h"

void Room::Tick()
{
    cout << "Update Room" << endl;

    DoTimer(100, &Room::Tick);
}

void Room::Init(const Json& roomData)
{
    if (roomData.empty())
        throw wstring(L"roomData가 없습니다");

    _roomData = roomData;

    // 자주 사용하는 JsonProperty Cache
    CacheRoomData();

    // 몬스터를 room에 스폰한다.
    int32 kindOfMonster = monsterIds.size();
    for (int32 i = 0; i < maxMonsterCount; i++)
    {
        int32 monsterTemplateId = monsterIds[Utils::GetRandom(0, kindOfMonster)];
        SpawnMonster(monsterTemplateId);
    }

}

void Room::CacheRoomData()
{
    using namespace JsonProperty::Map;

    _roomId = _roomData[TemplateId];

    const Json& centerPos = _roomData[CenterPos];
    roomCenterPos.x = centerPos[PosX].is_null() ? 0 : static_cast<float>(centerPos[PosX]);
    roomCenterPos.y = centerPos[PosY].is_null() ? 0 : static_cast<float>(centerPos[PosY]);
    roomCenterPos.z = centerPos[PosZ].is_null() ? 0 : static_cast<float>(centerPos[PosZ]);

    widthHalfExtent = _roomData[WidthHalfExtent].is_null() ? 0 : static_cast<float>(_roomData[WidthHalfExtent]);
    heightHalfExtent = _roomData[HeightHalfExtent].is_null() ? 0 : static_cast<float>(_roomData[HeightHalfExtent]);

    maxMonsterCount = _roomData[MaxMonsterCount].is_null() ? 0 : static_cast<int32>(_roomData[MaxMonsterCount]);
    monsterRespawnTime = _roomData[MonsterRespawnTime].is_null() ? 100000.f : static_cast<float>(_roomData[MonsterRespawnTime]);

    for (int32 monsterId : _roomData[MonsterIds])
    {
        monsterIds.push_back(monsterId);
    }
    
}

void Room::HandleEnterPlayer(PlayerRef enterPlayer, shared_ptr<Protocol::PosInfo> enterPos, bool moveRoom)
{
    uint64 enterPlayerId = enterPlayer->objectInfo->object_id();

    // 현재 방에 해당 player를 추가
    if (RegisterObject(enterPlayer) == false)
    {
        if (moveRoom)
            wcout << L"플레이어: " << enterPlayerId << "가 Room 이동시 다음 Room 입장에 실패했습니다" << '\n';
        else
            wcout << L"플레이어: " << enterPlayerId << "가 Room 입장에 실패했습니다" << '\n';

        return;
    }

    // 입장 위치 세팅
    enterPlayer->posInfo->CopyFrom(*enterPos);

    // OtherPlayer: Broadcast NewPlayer SpawnPkt In Room
    {
        Protocol::S_SPAWN spawnPkt;

        Protocol::ObjectInfo* objectInfo = spawnPkt.add_objects();
        objectInfo->CopyFrom(*enterPlayer->objectInfo);

        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(spawnPkt);
        Broadcast(sendBuffer, enterPlayerId);
    }

    // enterPlayer: Send Room's Objects
    {
        // 1) Room에 있는 모든 Object들을 가져온다.(본인 제외)
        RepeatedPtrField<Protocol::ObjectInfo> roomObjects;
        {
            for (auto& item : _objects)
            {
                if (item.second->objectInfo->object_id() == enterPlayerId)
                    continue;

                roomObjects.Add()->CopyFrom(*item.second->objectInfo);
            }
        }

        // 2) 입장한 플레이어가 단순 Room 이동이면 S_MOVE_ROOM, 새로 입장이면 S_SPAWN
        if (moveRoom)
        {
            Protocol::S_MOVE_ROOM moveRoomPkt;
            moveRoomPkt.set_map_id(enterPlayer->objectInfo->map_id());
            moveRoomPkt.mutable_info()->CopyFrom(*enterPos);
            moveRoomPkt.mutable_objects()->Swap(&roomObjects);

            SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(moveRoomPkt);
            if (auto session = enterPlayer->session.lock())
                session->Send(sendBuffer);
        }
        else
        {
            // Spawn할 Object에 본인을 추가
            roomObjects.Add()->CopyFrom(*enterPlayer->objectInfo);

            Protocol::S_SPAWN spawnPkt;
            spawnPkt.mutable_objects()->Swap(&roomObjects);

            SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(spawnPkt);
            if (auto session = enterPlayer->session.lock())
                session->Send(sendBuffer);
        }
    }
    
}

void Room::HandleLeavePlayer(PlayerRef leavePlayer, bool moveRoom)
{
    const uint64 leavePlayerId = leavePlayer->objectInfo->object_id();

    // 방을 나간 Player를 제거
    if (UnRegisterObject(leavePlayerId) == false)
    {
        if (moveRoom)
            wcout << L"플레이어: " << leavePlayerId << "가 Room 이동 시 이전 Room 퇴장에 실패했습니다" << '\n';
        else
            wcout << L"플레이어: " << leavePlayerId << "가 Room 퇴장에 실패했습니다" << '\n';

        return;
    }

    // leavePlayer: Send Despawn Packet(If needed)
    {
        if (moveRoom == false)
        {
            Protocol::S_DESPAWN despawnPkt;
            despawnPkt.add_object_ids(leavePlayerId);

            SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(despawnPkt);
            if (auto session = leavePlayer->session.lock())
                session->Send(sendBuffer);
        }
    }

    // OtherPlayer: Broadcast Player Despawn In Room
    {
        Protocol::S_DESPAWN despawnPkt;
        despawnPkt.add_object_ids(leavePlayerId);

        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(despawnPkt);
        Broadcast(sendBuffer);
    }
}

void Room::HandleMove(Protocol::C_MOVE pkt)
{
	const uint64 objectId = pkt.info().object_id();
    if (_objects.contains(objectId) == false)
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
    if (_objects.contains(objectId) == false)
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
    if (_objects.contains(objectId) == false)
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
    if (_objects.contains(objectId) == false)
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

optional<Json> Room::GetPortalDataFromPortalId(int32 portalId)
{
    using namespace JsonProperty::Map;

    const Json& portalList = _roomData[Portals][Lists];
    if(portalList.empty())
        return nullopt;

    for (const auto& portal : portalList)
    {
        if (portal[JsonProperty::Map::PortalId] == portalId)
            return portal;
    }

    return nullopt;
}

void Room::SetRandomPos(Protocol::PosInfo* posInfo, float widthPadding, float heightPadding, bool randYaw)
{
    using namespace JsonProperty::Map;

    float minX = roomCenterPos.x - (widthHalfExtent - widthPadding);
    float maxX = roomCenterPos.x + (widthHalfExtent - widthPadding);
    float minY = roomCenterPos.y - (heightHalfExtent - heightPadding);
    float maxY = roomCenterPos.y + (heightHalfExtent - heightPadding);

    posInfo->set_x(Utils::GetRandom(minX, maxX));
    posInfo->set_y(Utils::GetRandom(minY, maxY));
    posInfo->set_z(roomCenterPos.z + SPAWN_PADDING_Z);

    if(randYaw)
        posInfo->set_yaw(Utils::GetRandom(-180.f, 180.f));
}

bool Room::RegisterObject(ObjectRef object)
{
    uint64 objectId = object->objectInfo->object_id();
	if (_objects.contains(objectId))
		return false;

	_objects.insert(make_pair(objectId, object));

    // Object가 속한 Room에 대한 정보 갱신
	object->room.store(GetRoomRef());
    object->objectInfo->set_map_id(_roomId);

	return true;
}

bool Room::UnRegisterObject(uint64 objectId)
{
	if (_objects.contains(objectId) == false)
		return false;

	_objects.erase(objectId);

	return true;
}

MonsterRef Room::SpawnMonster(int32 templateId)
{
    MonsterRef newMonster = ObjectUtils::CreateMonster(templateId);
    newMonster->Init();

    if (RegisterObject(newMonster) == false)
    {
        wcout << L"SpawnMonster 실패" << '\n';
        return nullptr;
    }

    SetRandomPos(newMonster->posInfo, SPAWN_PADDING_X, SPAWN_PADDING_Y, true);

    //newMonster->PrintMonsterAllData(); // DEBUG

    return newMonster;
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
