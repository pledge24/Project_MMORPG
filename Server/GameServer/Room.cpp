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
        //SpawnMonster(5000);
        //SpawnMonster(5002);
        Update();
        return true;

        for (int32 i = 0; i < maxMonsterCount; i++)
        {
            int32 monsterTemplateId = monsterIds[Utils::GetRandom(0, kindOfMonster)];
            if (SpawnMonster(monsterTemplateId) == nullptr)
                return false;

            break;
        }
    }

    Update();

    return true;
}

void Room::Update()
{
    DoTimer(ROOM_UPDATE_INTERVAL_MS, &Room::Update);

    Protocol::S_MOVE movePkt;
    {
        for (auto pair : _objects)
        {
            ObjectRef object = pair.second;
            if (object->IsPlayer())
                continue;

            Protocol::PosInfo* info = movePkt.add_info();
            info->CopyFrom(*object->posInfo);
        }

        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(movePkt);
        Broadcast(sendBuffer);
    }
}

void Room::TickObject(ObjectRef object)
{
    int64 objectId = object->objectInfo->object_id();
    if (Contains(objectId) == false)
        return;

    uint64 curTime = GetTickCount64();
    uint64 prevTime = object->GetPrevTime();
    float deltaTime = (curTime - prevTime) / 1000.f;
    object->SetPrevTime(curTime);

    object->Tick(deltaTime);
}

bool Room::EnterPlayer(PlayerRef enterPlayer, RoomEnterData roomEnterData)
{
    Protocol::S_ENTER_ROOM enterRoomPkt;
    int64 enterPlayerId = enterPlayer->objectInfo->object_id();

    if (AddObject(enterPlayer) == false)
    {
        wcout << L"플레이어: " << enterPlayerId << "가 Room 입장에 실패했습니다" << '\n';

        if (auto session = enterPlayer->session.lock())
        {
            enterRoomPkt.set_success(false);
            enterRoomPkt.set_enter_type(roomEnterData.enterType);
            enterRoomPkt.set_room_id(_roomId);

            SEND_PACKET(enterRoomPkt);
        }

        return false;
    }
    else
    {
        enterPlayer->OnEnterRoom(static_pointer_cast<Room>(shared_from_this()), roomEnterData.enterPos);

        if (auto session = enterPlayer->session.lock())
        {
            enterRoomPkt.set_success(true);
            enterRoomPkt.set_enter_type(roomEnterData.enterType);
            enterRoomPkt.set_room_id(_roomId);

            if(roomEnterData.enterPos.has_value())
                enterRoomPkt.mutable_enter_pos()->CopyFrom(roomEnterData.enterPos.value());

            SEND_PACKET(enterRoomPkt);
        }
    }
  
    return true;
}

bool Room::LeavePlayer(PlayerRef leavePlayer, bool transferRoom)
{
    const int64 leavePlayerId = leavePlayer->objectInfo->object_id();

    if (RemoveObject(leavePlayerId) == false)
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

bool Room::TransferPlayer(PlayerRef player, RoomEnterData roomEnterData)
{
    // Leave Current Room
    if (LeavePlayer(player, true) == false)
    {
        return false;
    }

    // Enter Next Room
    RoomRef nextRoom = GRoomManager->GetRoomRefFromRoomId(roomEnterData.nextRoomId);
    nextRoom->DoAsync(&Room::EnterPlayer, player, roomEnterData);

    return true;
}

void Room::C_HandleEnterRoom(Protocol::C_ENTER_ROOM pkt, PlayerRef player)
{
    RoomEnterData roomEnterData{};
    RoomRef enterRoom = nullptr;

    Protocol::EnterType enterType = pkt.enter_type();
    switch (enterType)
    {
    case Protocol::ENTER_TYPE_MAP_CHANGE:
    {
        bool hasRoomID = pkt.has_room_id();
        bool invalidRoomID = pkt.room_id() != player->GetEnteringRoomId();
        if (!hasRoomID || invalidRoomID)
        {
            Protocol::S_ENTER_ROOM enterRoom;
            {
                enterRoom.set_success(false);
                enterRoom.set_enter_type(Protocol::ENTER_TYPE_MAP_CHANGE);

                SEND_PACKET(enterRoom);
            }

            return false;
        }

        int32 roomId = pkt.room_id();
        enterRoom = GRoomManager->GetRoomRefFromRoomId(roomId);

        // RoomEnterData 세팅
        roomEnterData.nextRoomId = roomId;
        roomEnterData.enterType = pkt.enter_type();
        roomEnterData.enterPos->CopyFrom(*player->posInfo);

        enterRoom->DoAsync([self = enterRoom, player, roomEnterData]()
            {
                if (self->EnterPlayer(player, roomEnterData) == false)
                    return;

                if (self->SpawnPlayer(player) == nullptr)
                    return;

                self->ReplicateRoomData(player, true);
            });

        break;
    }
    case Protocol::ENTER_TYPE_ROOM_CHANGE:
    {
        bool hasPortalId = pkt.has_portal_id();
        if (!hasPortalId)
        {
            Protocol::S_ENTER_ROOM enterRoom;
            {
                enterRoom.set_success(false);
                enterRoom.set_enter_type(Protocol::ENTER_TYPE_ROOM_CHANGE);

                SEND_PACKET(enterRoom);
            }

            return false;
        }

        RoomRef curRoom = player->room.load().lock();
        if (curRoom == nullptr)
            return false;

        optional<Json> portalDataOpt = curRoom->GetPortalDataFromPortalId(pkt.portal_id());
        if (portalDataOpt.has_value() == false)
        {
            wcout << "플레이어가 현재 Room에 존재하지 않는 포탈사용 시도" << '\n';
            return false;
        }

        // RoomEnterData 세팅
        {
            using namespace JsonProperty::Map;
            const Json& portalData = portalDataOpt.value();
            const Json& dst = portalData[Dst];

            roomEnterData.nextRoomId = dst[TemplateId];
            roomEnterData.enterType = pkt.enter_type();

            Protocol::PosInfo enterPosInfo;
            Protocol::Vector& pos = *enterPosInfo.mutable_pos();
            enterPosInfo.set_object_id(player->objectInfo->object_id());
            pos.set_x(dst[PosX]);
            pos.set_y(dst[PosY]);
            pos.set_z(dst[PosZ]);
            enterPosInfo.set_yaw(dst[Yaw]);
            enterPosInfo.set_state(Protocol::MoveState::MOVE_STATE_IDLE);

            roomEnterData.enterPos->Swap(&enterPosInfo);
        }


        curRoom->DoAsync([self = curRoom, enterRoom, player, roomEnterData]()
            {
                if (self->TransferPlayer(player, roomEnterData) == false)
                    return;

                enterRoom->DoAsync(&Room::ReplicateRoomData, player, true);
            });

        break;
    }
    default:
    {
        cout << "Handle_C_ENTER_ROOM: Invalid Enter Type" << '\n';
        return false;
    }
    case Protocol::ENTER_TYPE_NONE:
        break;
    case Protocol::EnterType_INT_MIN_SENTINEL_DO_NOT_USE_:
        break;
    case Protocol::EnterType_INT_MAX_SENTINEL_DO_NOT_USE_:
        break;
    }
}

void Room::C_HandleMove(Protocol::C_MOVE pkt)
{
	const int64 objectId = pkt.info().object_id();
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

void Room::C_HandleBuyItem(Protocol::C_BUY_ITEM pkt, PlayerRef player)
{
    auto session = player->session.lock();
    if (session == nullptr)
        return;

    Protocol::S_BUY_ITEM buyItemPkt;
    Protocol::Slot* updatedSlot = buyItemPkt.mutable_updated_slot();
    int32 templateId = pkt.template_id();
    int64 totalGold = 0;

    if (player->ProcessBuyItem(OUT updatedSlot, OUT totalGold, templateId) == false)
    {
        buyItemPkt.set_success(false);

        SEND_PACKET(buyItemPkt);
        return;
    }

    // 아이템 구매 성공 처리
    {
        buyItemPkt.set_success(true);
        buyItemPkt.set_gold(totalGold);

        SEND_PACKET(buyItemPkt);
        cout << buyItemPkt.DebugString() << endl;
    }

    return;
}

void Room::C_HandleSellItem(Protocol::C_SELL_ITEM pkt, PlayerRef player)
{
    auto session = player->session.lock();
    if (session == nullptr)
        return;

    Protocol::S_SELL_ITEM sellItemPkt;
    const Protocol::Slot& requestSlot = pkt.slot();
    Protocol::Slot* updatedSlot = sellItemPkt.mutable_updated_slot();
    int64 totalGold = 0;

    if (player->ProcessSellItem(requestSlot, OUT updatedSlot, OUT totalGold) == false)
    {
        sellItemPkt.set_success(false);

        SEND_PACKET(sellItemPkt);
        return;
    }

    // 아이템 판매 성공 처리
    {
        sellItemPkt.set_success(true);
        sellItemPkt.set_gold(totalGold);

        SEND_PACKET(sellItemPkt);
        cout << sellItemPkt.DebugString() << endl;
    }

}

void Room::C_HandleUseItem(Protocol::C_USE_ITEM pkt, PlayerRef player)
{
    auto session = player->session.lock();
    if (session == nullptr)
        return;

    Protocol::S_USE_ITEM useItemPkt;
    const Protocol::Slot& targetSlot = pkt.slot();

    if (player->ProcessUseItem(targetSlot, OUT useItemPkt) == false)
    {
        useItemPkt.set_success(false);

        SEND_PACKET(useItemPkt);
        return;
    }

    // 아이템 판매 성공 처리
    {
        useItemPkt.set_success(true);

        SEND_PACKET(useItemPkt);
        cout << useItemPkt.DebugString() << endl;
    }

}

void Room::C_HandleEquipGear(Protocol::C_EQUIP_GEAR pkt, PlayerRef player)
{
    const int64 objectId = player->objectInfo->object_id();
    if (_objects.contains(objectId) == false)
        return;

    Protocol::S_EQUIP_GEAR equipGearPkt;
    {
        const Protocol::Slot& slot = pkt.slot();

        equipGearPkt.set_success(true);
        equipGearPkt.set_object_id(objectId);
        equipGearPkt.set_slot_id(slot.slot_id());
        equipGearPkt.set_template_id(slot.item().template_id());
    }

    if (player->ProcessEquipGear(pkt.slot(), OUT equipGearPkt) == false)
    {
        if (SessionRef session = player->session.lock())
        {
            equipGearPkt.set_success(false);
            SEND_PACKET(equipGearPkt);
        }

        return;
    }

    // 장착한 유저에게만 그대로 전송.
    {
        SessionRef session = player->session.lock();
        cout << equipGearPkt.DebugString() << endl;
        SEND_PACKET(equipGearPkt);
    }

    // 다른 유저들한테는 변경된 stat을 보내지 않는다.
    {
        equipGearPkt.clear_updated_slots();
        equipGearPkt.clear_updated_stat();
        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(equipGearPkt);
        Broadcast(sendBuffer, objectId);
    }

}

void Room::C_HandleUnequipGear(Protocol::C_UNEQUIP_GEAR pkt, PlayerRef player)
{
    const int64 objectId = player->objectInfo->object_id();
    if (_objects.contains(objectId) == false)
        return;

    Protocol::S_UNEQUIP_GEAR unequipGearPkt;
    {
        const Protocol::Slot& slot = pkt.slot();

        unequipGearPkt.set_success(true);
        unequipGearPkt.set_object_id(objectId);
        unequipGearPkt.set_slot_id(slot.slot_id());
        unequipGearPkt.set_template_id(slot.item().template_id());
    }

    if (player->ProcessUnequipGear(pkt.slot(), OUT unequipGearPkt) == false)
    {
        if (SessionRef session = player->session.lock())
        {
            unequipGearPkt.set_success(false);

            SEND_PACKET(unequipGearPkt);
        }
        return;
    }

    // 탈착한 유저에게만 그대로 전송.
    {
        SessionRef session = player->session.lock();
        SEND_PACKET(unequipGearPkt);
    }

    // 다른 유저들한테는 변경된 stat을 보내지 않는다.
    {
        unequipGearPkt.clear_updated_slots();
        unequipGearPkt.clear_updated_stat();
        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(unequipGearPkt);
        Broadcast(sendBuffer, objectId);
    }

}

void Room::C_HandleNormalAttack(Protocol::C_NORMAL_ATTACK pkt, PlayerRef player)
{
    const int64 objectId = player->objectInfo->object_id();
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

void Room::C_HandleRespawn(Protocol::C_RESPAWN pkt, PlayerRef player)
{
    // TODO: Validation(Ex. 해당 캐릭터가 리스폰 조건을 만족했는가?)

    // 1. Find Respawn Room
    RoomRef respawnRoom = nullptr;
    Protocol::PosInfo respawnPos;
    Protocol::RespawnType respawnType = pkt.respawn_type();

    player->GetRespawnData(respawnType, OUT respawnRoom, OUT respawnPos);

    if (respawnRoom == nullptr)
    {
        wcout << L"리스폰할 룸을 찾지 못함" << '\n';
        return;
    }

    // 2. Process Respawn
    if (shared_from_this() == respawnRoom)
    {
        HandleRespawn(player, respawnType, respawnPos);
    }
    else
    {
        // 죽은 Room과 다른 Room에서 Respawn하는 경우 진입
        using namespace JsonProperty::Map;

        RoomEnterData enterData = RoomEnterData{};
        // 1) Room 이동 데이터 설정
        {
            enterData.nextRoomId = respawnRoom->GetRoomId();
            enterData.enterType = Protocol::ENTER_TYPE_RESPAWN;

            Protocol::PosInfo enterPos;
            enterPos.CopyFrom(respawnPos);
            enterPos.set_object_id(player->objectInfo->object_id());
            enterData.enterPos->Swap(&enterPos);
        }

        // Room 이동
        TransferPlayer(player, enterData);
    }

}

void Room::HandleNormalAttack(int32 combo, CreatureRef creature)
{
    Protocol::S_NORMAL_ATTACK normalAttackPkt;
    {
        normalAttackPkt.set_object_id(creature->objectInfo->object_id());
        normalAttackPkt.set_combo(combo);
        normalAttackPkt.set_yaw(creature->posInfo->yaw());

        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(normalAttackPkt);
        Broadcast(sendBuffer);
    }
}

void Room::HandleHit(ObjectRef attacker, Protocol::AttackInfo attackInfo)
{
    // 1) 해당 공격에 맞은 대상을 찾는다.
    vector<CreatureRef> HitCreatures;
    if (attackInfo.has_target_id())
    {
        int64 targetId = attackInfo.target_id();
        if(Contains(targetId) == false)
            return;

        // TODO: 피격이 가능한 대상?
        if (CreatureRef creature = dynamic_pointer_cast<Creature>(_objects[targetId]))
        {
            HitCreatures.push_back(creature);
        }
    }
    else
    {

    }
    
    // 2) 피격 대상에게 결과를 적용한다.
    for (CreatureRef creature : HitCreatures)
    {
        creature->OnHit(attacker, attackInfo);

        Protocol::S_HIT HitPkt;
        {
            HitPkt.set_object_id(creature->objectInfo->object_id());
            HitPkt.set_damage(attackInfo.damage());
            HitPkt.set_updated_hp(creature->GetStatValue(Protocol::STAT_TYPE_HP));

            SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(HitPkt);
            Broadcast(sendBuffer);
        }

        if (creature->IsDead())
        {
            PlayerRef player = dynamic_pointer_cast<Player>(attacker);
            MonsterRef monster = dynamic_pointer_cast<Monster>(creature);
            if (player && monster)
            {
                HandleMonsterKill(player, monster);
            }
            HandleDie(creature);
        }
    }

}

void Room::HandleMonsterKill(PlayerRef player, MonsterRef monster)
{
    Protocol::S_REWARD_RESULT rewardResultPkt;
    {
        rewardResultPkt.set_type(Protocol::REWARD_TYPE_MONSTER_KILL);
        Protocol::Reward reward;
        {
            reward.set_exp(monster->GetExpReward());
            reward.set_gold(monster->GetGoldReward());
        }
        rewardResultPkt.mutable_reward()->Swap(&reward);
    }

    player->OnGetReward(rewardResultPkt);

    if (auto session = player->session.lock())
    {
        SEND_PACKET(rewardResultPkt);
    }
}

void Room::HandleDie(CreatureRef creature)
{
    int64 objectId = creature->objectInfo->object_id();

    Protocol::S_DIE diePkt;
    {
        diePkt.set_object_id(objectId);

        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(diePkt);
        Broadcast(sendBuffer);
    }

    // 바로 Room에서 제거한다.
    RemoveObject(objectId);
}

void Room::HandleRespawn(PlayerRef player, Protocol::RespawnType& respawnType, Protocol::PosInfo& respawnPos)
{
    int64 playerId = player->objectInfo->object_id();
    auto session = player->session.lock();
    if (session == nullptr)
        return false;

    Protocol::S_RESPAWN respawnPkt;
    if (respawnPoint == nullptr)
    {
        wcout << "리스폰 위치가 없는 Room에서 리스폰 시도" << '\n';
        {
            respawnPkt.set_success(false);
            respawnPkt.set_error_message(string("No Respawn Point"));

            SEND_PACKET(respawnPkt);
        }

        return false;
    }

    if (player->ProcessRespawn(pkt.respawn_type(), respawnPoint, respawnPkt) == false)
    {
        wcout << "ProcessRespawn가 false를 반환" << '\n';
        {
            respawnPkt.set_success(false);
            respawnPkt.set_error_message(string("Fail to Respawn"));

            SEND_PACKET(respawnPkt);
        }

        return false;
    }

    // 리스폰 성공 처리
    SEND_PACKET(respawnPkt);
}

void Room::ReplicateRoomData(PlayerRef player, bool excludeThisPlayer)
{
    int64 playerId = player->objectInfo->object_id();

    // 해당 플레이어에게 Room Object 전송
    Protocol::S_SPAWN spawnPkt;
    if (auto session = player->session.lock())
    {
        for (auto& item : _objects)
        {
            if (!excludeThisPlayer && item.second->objectInfo->object_id() == playerId)
                continue;

            spawnPkt.add_objects()->CopyFrom(*item.second->objectInfo);
            // equipped_gear_summary 활용하기
        }

        SEND_PACKET(spawnPkt);
    }
}

MonsterRef Room::SpawnMonster(int32 templateId)
{
    MonsterRef newMonster = ObjectUtils::CreateMonster(templateId);
    if (newMonster == nullptr)
        return nullptr;

    // Set PosInfo
    {
        Protocol::PosInfo respawnPos;
        SetRandomPos(&respawnPos, true, true);

        newMonster->SetPosInfo(respawnPos);
    }

    if (AddObject(newMonster) == false)
    {
        wcout << L"SpawnMonster 실패" << '\n';
        return nullptr;
    }

    //newMonster->PrintMonsterAllData(); // DEBUG

    return newMonster;
}

PlayerRef Room::SpawnPlayer(int64 objectId)
{
    if (_objects.contains(objectId) == false)
        return nullptr;

    PlayerRef targetPlayer = dynamic_pointer_cast<Player>(_objects[objectId]);
    if (targetPlayer == nullptr)
        return nullptr;

    return SpawnPlayer(targetPlayer);
}

PlayerRef Room::SpawnPlayer(PlayerRef targetPlayer)
{
    Protocol::S_SPAWN spawnPkt;
    {
        Protocol::ObjectInfo* objectInfo = spawnPkt.add_objects();
        objectInfo->CopyFrom(*targetPlayer->objectInfo);

        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(spawnPkt);
        Broadcast(sendBuffer);
    }

    return targetPlayer;
}

vector2D Room::GetRandomLocation(bool usePadding)
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
    vector2D randomPos = GetRandomLocation(usePadding);
    Protocol::Vector* pos = posInfo->mutable_pos();

    pos->set_x(randomPos.x);
    pos->set_y(randomPos.y);
    pos->set_z(_roomCenterPos.z + LOCATION_PADDING_Z);

    if(randYaw)
        posInfo->set_yaw(GetRandomYaw());
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
    Protocol::Vector* pos = posInfo->mutable_pos();

    float minX = pos->x() - range;
    float maxX = pos->x() + range;
    float minY = pos->y() - range;
    float maxY = pos->y() + range;

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

        for (int64 objectId : cell)
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

        respawnPoint->mutable_pos()->set_x(posX);
        respawnPoint->mutable_pos()->set_y(posY);
        respawnPoint->mutable_pos()->set_z(posZ);
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

void Room::UpdateCellMatrix()
{
    ClearCellMatrix();

    for (auto& pair : _objects)
    {
        int64 objectId = pair.first;
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
    return GetCellIndicesFromPos(vector2D(posInfo->pos().x(), posInfo->pos().y()));
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
    return GetCellFromPos(vector2D(posInfo->pos().x(), posInfo->pos().y()));
}

bool Room::AddObject(ObjectRef object)
{
    if (object == nullptr)
        return false;

    int64 objectId = object->objectInfo->object_id();
	if (_objects.contains(objectId))
		return false;

	_objects.insert(make_pair(objectId, object));

	return true;
}

bool Room::RemoveObject(int64 objectId)
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

void Room::Broadcast(SendBufferRef sendBuffer, int64 exceptId)
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