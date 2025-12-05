#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include "Monster.h"
#include "ObjectUtils.h"
#include "EquippedGear.h"

RoomRef Room::Create(const Json& roomData)
{
    RoomRef newRoom = make_shared<Room>();

    if (newRoom->Init(roomData) == false)
    {
        return nullptr;
    }

    return newRoom;
}

bool Room::Init(const Json& roomData)
{
    if (roomData.empty())
    {
        wcout << L"roomData가 없습니다" << '\n';
        return false;
    }

    _roomData = roomData;

    // 자주 사용하는 JsonProperty Cache
    CacheRoomData();

    CreateCellMatrix();

    return true;
}

bool Room::Start()
{
    // 몬스터를 Room에 스폰한다.
    if (monsterIds.empty() == false && _roomId == 20)
    {
        int32 kindOfMonster = monsterIds.size();
        SpawnMonster(5000);
        //SpawnMonster(5002);
        UpdateTick();
        return true;

        for (int32 i = 0; i < maxMonsterCount; i++)
        {
            int32 monsterTemplateId = monsterIds[Utils::GetRandom(0, kindOfMonster)];
            if (SpawnMonster(monsterTemplateId) == nullptr)
                return false;

            break;
        }
    }

    UpdateTick();

    return true;
}

void Room::UpdateTick()
{
    uint64 curTickTime = GetTickCount64();
    float deltaTime = static_cast<float>(curTickTime - prevTickTime) / 1000.f;
    prevTickTime = curTickTime;

    // Tick All Objects In Room.
    ProcessTickGroupFunc(ETickGroup::TG_PreObjectTick, deltaTime);
    ProcessTickGroupFunc(ETickGroup::TG_PrePhysics, deltaTime);
    ProcessTickGroupFunc(ETickGroup::TG_DuringPhysics, deltaTime);
    ProcessTickGroupFunc(ETickGroup::TG_PostPhysics, deltaTime);

    elapsedTime += deltaTime;
    if (elapsedTime > SEND_MOVE_PACKET_TIME)
    {
        elapsedTime = 0.f;

        Protocol::S_MOVE movePkt;
        for (auto pair : _objects)
        {
            ObjectRef object = pair.second;
            if(PlayerRef player = dynamic_pointer_cast<Player>(object))
                continue;

            Protocol::PosInfo* info = movePkt.add_info();
            info->CopyFrom(*object->posInfo);
        }

        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(movePkt);
        Broadcast(sendBuffer);
    }

    DoTimer(ROOM_TICK, &Room::UpdateTick);
}

void Room::ProcessTickGroupFunc(ETickGroup tickGroup, float deltaTime)
{
    for (auto pair : _objects)
    {
        ObjectRef object = pair.second;
        object->ProcessTickGroupFunc(tickGroup, deltaTime);
    }

    // Room Function
    switch (tickGroup)
    {
    case ETickGroup::TG_PreObjectTick:
    {
        break;
    }
    case ETickGroup::TG_PrePhysics: 
    {
        UpdateCellMatrix();
        break;
    }
    case ETickGroup::TG_DuringPhysics:
    {
        break;
    }
    case ETickGroup::TG_PostPhysics:
    {
        break;
    }
    }
}

bool Room::EnterPlayer(PlayerRef enterPlayer, RoomEnterData roomEnterData)
{
    Protocol::S_ENTER_ROOM enterRoomPkt;
    uint64 enterPlayerId = enterPlayer->objectInfo->object_id();

    if (RegisterObject(enterPlayer) == false)
    {
        if (roomEnterData.enterType == Protocol::ENTER_TYPE_INNER_PORTAL)
            wcout << L"플레이어: " << enterPlayerId << "가 Room 입장에 실패했습니다" << '\n';
        else
            wcout << L"플레이어: " << enterPlayerId << "가 필드간 Room 이동을 실패했습니다" << '\n';

        if (auto session = enterPlayer->session.lock())
        {
            enterRoomPkt.set_success(false);
            enterRoomPkt.set_enter_type(roomEnterData.enterType);
            enterRoomPkt.set_room_id(_roomId);

            SEND_PACKET(enterRoomPkt);
        }

        return false;
    }

    // 플레이어 Room 입장 성공 처리
    {
        enterPlayer->OnEnterRoom(static_pointer_cast<Room>(shared_from_this()), roomEnterData);

        if (auto session = enterPlayer->session.lock())
        {
            enterRoomPkt.set_success(true);
            enterRoomPkt.set_enter_type(roomEnterData.enterType);
            enterRoomPkt.set_room_id(_roomId);

            SEND_PACKET(enterRoomPkt);
        }

        // OtherPlayer: Broadcast NewPlayer SpawnPkt In Room
        {
            Protocol::S_SPAWN spawnPkt;

            Protocol::ObjectInfo* objectInfo = spawnPkt.add_objects();
            objectInfo->CopyFrom(*enterPlayer->objectInfo);

            SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(spawnPkt);
            Broadcast(sendBuffer, enterPlayerId);
        }
    }

    // Room의 오브젝트 정보를 클라이언트에게 보내야 하는 경우 진입
    if (roomEnterData.sendRoomData)
    {
        bool excludeThisPlayer = false;
        switch(roomEnterData.enterType)
        {
        case Protocol::ENTER_TYPE_ENTER_GAME:
        case Protocol::ENTER_TYPE_OUTER_PORTAL:
        {
            excludeThisPlayer = true;   // 본인 제외
        }
        }
        
        SendAllObjectsData(enterPlayer, false);
    }
   
    return true;
}

bool Room::LeavePlayer(PlayerRef leavePlayer, bool transferRoom)
{
    const uint64 leavePlayerId = leavePlayer->objectInfo->object_id();

    if (UnRegisterObject(leavePlayerId) == false)
    {
        if (transferRoom)
            wcout << L"플레이어: " << leavePlayerId << "가 Room 이동 중 현재 Room 퇴장에 실패했습니다" << '\n';
        else
            wcout << L"플레이어: " << leavePlayerId << "가 Room 퇴장에 실패했습니다" << '\n';

        return false;
    }

    // 플레이어 Room 퇴장 성공 처리
    {
        // OtherPlayer: Broadcast Player Despawn In Room
        {
            Protocol::S_DESPAWN despawnPkt;
            despawnPkt.add_object_ids(leavePlayerId);

            SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(despawnPkt);
            Broadcast(sendBuffer);
        }

        // leavePlayer: Despawn Packet 전송
        if (auto session = leavePlayer->session.lock())
        {
            if (transferRoom == false)
            {
                Protocol::S_DESPAWN despawnPkt;
                despawnPkt.add_object_ids(leavePlayerId);

                SEND_PACKET(despawnPkt);
            }
        }
    }

    return true;
}

void Room::TransferPlayer(PlayerRef player, RoomEnterData roomEnterData)
{
    // Leave Current Room
    LeavePlayer(player, true);

    // Enter Next Room
    RoomRef nextRoom = GRoomManager->GetRoomRefFromRoomId(roomEnterData.nextRoomId);
    nextRoom->DoAsync(&Room::EnterPlayer, player, roomEnterData);
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
            Protocol::PosInfo* info = movePkt.add_info();
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
    
    // 일반 공격 사실을 Broadcast.
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

void Room::HandleRespawn(Protocol::C_RESPAWN pkt, PlayerRef player, shared_ptr<Protocol::PosInfo> respawnPos)
{
    uint64 playerId = player->objectInfo->object_id();
    bool success = true;

    if (respawnPoint == nullptr)
    {
        wcout << "리스폰 위치가 없는 Room에서 리스폰 시도" << '\n';
        success = false;
    }
    else
    {
        player->posInfo->CopyFrom(*respawnPoint);
        player->OnRespawn();
    }
    
    // 리스폰 패킷 전송
    if (auto session = player->session.lock())
    {
        Protocol::S_RESPAWN respawnPkt;

        respawnPkt.set_success(success);
        respawnPkt.set_respawn_type(pkt.respawn_type());

        respawnPkt.set_object_id(playerId);
        respawnPkt.set_room_id(_roomId);
        respawnPkt.mutable_pos_info()->CopyFrom(*player->posInfo);
        respawnPkt.set_hp(player->statInfo->hp());

        SEND_PACKET(respawnPkt);
    }

}

void Room::SendAllObjectsData(PlayerRef player, bool excludeThisPlayer)
{
    uint64 playerId = player->objectInfo->object_id();

    // Room에 있는 모든 Object들을 가져온다.(본인 제외)
    RepeatedPtrField<Protocol::ObjectInfo> roomObjects;
    {
        for (auto& item : _objects)
        {
            if (!excludeThisPlayer && item.second->objectInfo->object_id() == playerId)
                continue;

            roomObjects.Add()->CopyFrom(*item.second->objectInfo);
            // equipped_gear_summary 활용하기

        }
    }

    // 해당 플레이어에게 Room Object 전송
    Protocol::S_SPAWN spawnPkt;
    spawnPkt.mutable_objects()->Swap(&roomObjects);

    SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(spawnPkt);
    if (auto session = player->session.lock())
        session->Send(sendBuffer);
}

vector2D Room::GetRandomPos(bool usePadding)
{
    float widthPadding = usePadding ? LOCATION_PADDING_X : 0.f;
    float heightPadding = usePadding ? LOCATION_PADDING_Y : 0.f;

    float paddedMinX = _roomMinX + widthPadding;
    float paddedMaxX = _roomMaxX - widthPadding;
    float paddedMinY = _roomMinY + heightPadding;
    float paddedMaxY = _roomMaxY - heightPadding;

    vector2D randomPos;

    randomPos.x = Utils::GetRandom(_roomMinX, _roomMaxX);
    randomPos.y = Utils::GetRandom(_roomMinY, _roomMaxY);

    return randomPos;
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

void Room::SetRandomPos(Protocol::PosInfo* posInfo, bool usePadding, bool randYaw)
{
    vector2D randomPos = GetRandomPos();

    posInfo->set_x(randomPos.x);
    posInfo->set_y(randomPos.y);
    posInfo->set_z(_roomCenterPos.z + LOCATION_PADDING_Z);

    if(randYaw)
        posInfo->set_yaw(Utils::GetRandom(-180.f, 180.f));
}

void Room::OnDie(Protocol::S_DIE& diePkt)
{
    ObjectRef DeadObject = _objects[diePkt.object_id()];
    if (DeadObject->IsPlayer() == false)
    {
        UnRegisterObject(diePkt.object_id());   // Room에서 이 오브젝트 삭제
    }

    // Broadcast Die Packet
    {
        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(diePkt);
        Broadcast(sendBuffer);
    }
}

vector2D Room::ClampLocation(float posX, float posY, bool usePadding)
{
    float widthPadding = usePadding ? LOCATION_PADDING_X : 0.f;
    float heightPadding = usePadding ? LOCATION_PADDING_Y : 0.f;

    float minX = _roomMinX + widthPadding;
    float maxX = _roomMaxX - widthPadding;
    float minY = _roomMinY + heightPadding;
    float maxY = _roomMaxY - heightPadding;

    float clampedPosX = std::clamp(posX, minX, maxX);
    float clampedPosY = std::clamp(posY, minY, maxY);

    return { clampedPosX, clampedPosY };
}

pair<PlayerRef, float> Room::FindClosestPlayer(Protocol::PosInfo* posInfo, float range)
{
    float minX = posInfo->x() - range;
    float maxX = posInfo->x() + range;
    float minY = posInfo->y() - range;
    float maxY = posInfo->y() + range;

    vector2D minPos = ClampLocation(minX, minY, false);
    vector2D maxPos = ClampLocation(maxX, maxY, false);

    auto minIndices = GetCellIndicesFromPos(minPos);
    auto maxIndices = GetCellIndicesFromPos(maxPos);

    if (minIndices == make_pair(-1, -1) || maxIndices == make_pair(-1, -1))
    {
        wcout << L"FindClosestPlayer 실패" << '\n';
        return make_pair(nullptr, -1.f);
    }

    // Find Interested Of Cell
    using Indices = pair<int32, int32>;
    vector<Indices> indicesList;
    for (int32 indexX = minIndices.first; indexX <= maxIndices.first; indexX++)
    {
        for (int32 indexY = minIndices.second; indexY <= maxIndices.second; indexY++)
        {
            indicesList.push_back(make_pair(indexX, indexY));
        }
    }

    PlayerRef closestPlayer = nullptr;
    float minDist = -1.f;
    float squareRange = range * range;
    for (Indices indices : indicesList)
    {
        const Cell& cell = _cellMatrix[indices.first][indices.second];

        for (uint64 objectId : cell)
        {
            if (PlayerRef player = dynamic_pointer_cast<Player>(_objects[objectId]))
            {
                float squareDist = MathUtil::Distance(posInfo, player->posInfo, true);
                if (squareRange < squareDist)
                    continue;

                if (minDist < 0.f || squareDist < minDist)
                {
                    minDist = squareDist;
                    closestPlayer = player;
                }
            }
        }

    }

    return make_pair(closestPlayer, squareRange);
}

void Room::CacheRoomData()
{
    using namespace JsonProperty::Map;

    _roomId = _roomData[TemplateId];

    const Json& centerPos = _roomData[CenterPos];
    _roomCenterPos.x = centerPos[PosX].is_null() ? 0 : static_cast<float>(centerPos[PosX]);
    _roomCenterPos.y = centerPos[PosY].is_null() ? 0 : static_cast<float>(centerPos[PosY]);
    _roomCenterPos.z = centerPos[PosZ].is_null() ? 0 : static_cast<float>(centerPos[PosZ]);

    _widthHalfExtent = _roomData[WidthHalfExtent].is_null() ? 0 : static_cast<float>(_roomData[WidthHalfExtent]);
    _heightHalfExtent = _roomData[HeightHalfExtent].is_null() ? 0 : static_cast<float>(_roomData[HeightHalfExtent]);

    _roomMinX = _roomCenterPos.x - _heightHalfExtent;
    _roomMaxX = _roomCenterPos.x + _heightHalfExtent;
    _roomMinY = _roomCenterPos.y - _widthHalfExtent;
    _roomMaxY = _roomCenterPos.y + _widthHalfExtent;

    maxMonsterCount = _roomData[MaxMonsterCount].is_null() ? 0 : static_cast<int32>(_roomData[MaxMonsterCount]);
    monsterRespawnTime = _roomData[MonsterRespawnTime].is_null() ? 100000.f : static_cast<float>(_roomData[MonsterRespawnTime]);

    for (int32 monsterId : _roomData[MonsterIds])
    {
        monsterIds.push_back(monsterId);
    }

    // 리스폰 포인트 저장
    if (_roomData[HasRespawnPoint] && _roomData[RespawnPoint].is_null() == false)
    {
        respawnPoint = make_shared<Protocol::PosInfo>();

        const Json& point = _roomData[RespawnPoint];
        float posX = point[PosX];
        float posY = point[PosY];
        float posZ = point[PosZ];

        respawnPoint->set_x(posX);
        respawnPoint->set_y(posY);
        respawnPoint->set_z(posZ);
        respawnPoint->set_yaw(0.f);
        respawnPoint->set_state(Protocol::MoveState::MOVE_STATE_IDLE);
    }

}

void Room::CreateCellMatrix()
{
    float snappedMinX = static_cast<float>(std::floor(_roomMinX / CELL_SIZE)) * CELL_SIZE;
    float snappedMaxX = static_cast<float>(std::ceil(_roomMaxX / CELL_SIZE)) * CELL_SIZE;
    float snappedMinY = static_cast<float>(std::floor(_roomMinY / CELL_SIZE)) * CELL_SIZE;
    float snappedMaxY = static_cast<float>(std::ceil(_roomMaxY / CELL_SIZE)) * CELL_SIZE;

    int32 cellCountX = static_cast<int32>((snappedMaxX - snappedMinX) / CELL_SIZE);
    int32 cellCountY = static_cast<int32>((snappedMaxY - snappedMinY) / CELL_SIZE);

    _cellMatrix.resize(cellCountX, vector<Cell>(cellCountY));
    _cellOffset = vector2D(snappedMinX, snappedMinY);
}

void Room::ClearCellMatrix()
{
    for (auto& inner_vec : _cellMatrix)
    {
        for (auto& s : inner_vec)
        {
            s.clear();
        }
    }
}

std::pair<int32, int32> Room::GetCellIndicesFromPos(const vector2D& objectPos)
{
    float offsetX = objectPos.x - _cellOffset.x;
    float offsetY = objectPos.y - _cellOffset.y;

    if (offsetX < 0.f || offsetY < 0.f)
        return make_pair(-1, -1);

    int32 indexX = static_cast<int32>(offsetX / CELL_SIZE);
    int32 indexY = static_cast<int32>(offsetY / CELL_SIZE);

    if(indexX < 0 || indexX >= _cellMatrix.size() || indexY < 0 || indexY >= _cellMatrix[0].size())
        return make_pair(-1, -1);

    return make_pair(indexX, indexY);
}

std::pair<int32, int32> Room::GetCellIndicesFromPos(Protocol::PosInfo* posInfo)
{
    return GetCellIndicesFromPos(vector2D(posInfo->x(), posInfo->y()));
}

Cell* Room::GetCellFromPos(const vector2D& pos)
{
    auto cellIndices = GetCellIndicesFromPos(pos);
    if (cellIndices == make_pair(-1, -1))
    {
        wcout << L"GetCellFromPos: 유효하지 않는 위치입니다" << '\n';
        return nullptr;
    }

    int32 indexX = cellIndices.first;
    int32 indexY = cellIndices.second;

    return &_cellMatrix[indexX][indexY];
}

Cell* Room::GetCellFromPos(Protocol::PosInfo* posInfo)
{
    return GetCellFromPos(vector2D(posInfo->x(), posInfo->y()));
}

void Room::UpdateCellMatrix()
{
    ClearCellMatrix();

    for (auto& pair : _objects)
    {
        uint64 objectId = pair.first;
        ObjectRef object = pair.second;

        Protocol::PosInfo* objectPos = object->posInfo;
        
        auto indices = GetCellIndicesFromPos(objectPos);
        if (indices == make_pair(-1, -1))
        {
            wcout << L"유효하지 않은 위치" << '\n';
            continue;
        }
        
        int32 indexX = indices.first;
        int32 indexY = indices.second;

        _cellMatrix[indexX][indexY].insert(objectId);

        //printf("object: %d (%d, %d)\n", objectId, indexX, indexY);
    }
}

bool Room::RegisterObject(ObjectRef object)
{
    if (object == nullptr)
        return false;

    uint64 objectId = object->objectInfo->object_id();
	if (_objects.contains(objectId))
		return false;

	_objects.insert(make_pair(objectId, object));

    // Object가 속한 Room에 대한 정보 갱신
	object->room.store(GetRoomRef());
    object->objectInfo->set_room_id(_roomId);

	return true;
}

bool Room::UnRegisterObject(uint64 objectId)
{
	if (_objects.contains(objectId) == false)
		return false;

    ObjectRef object = _objects[objectId];

    // cellMatrix에 object 삭제
    auto cellPos = GetCellIndicesFromPos(object->posInfo);
    _cellMatrix[cellPos.first][cellPos.second].erase(objectId);

    // object 삭제
	_objects.erase(objectId);

	return true;
}

MonsterRef Room::SpawnMonster(int32 templateId)
{
    MonsterRef newMonster = ObjectUtils::CreateMonster(templateId);

    SetRandomPos(newMonster->posInfo, true, true);
    newMonster->PostInit();

    if (RegisterObject(newMonster) == false)
    {
        wcout << L"SpawnMonster 실패" << '\n';
        return nullptr;
    }

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