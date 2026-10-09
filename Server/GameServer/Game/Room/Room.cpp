#include "Core/pch.h"
#include "Game/Room/Room.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/Monster.h"
#include "Game/Combat/Combat.h"

namespace
{
    // 셀 행렬은 평면 좌표만 본다.
    vector2D ToPlanePos(const Protocol::PosInfo& posInfo)
    {
        return vector2D(posInfo.pos().x(), posInfo.pos().y());
    }
}

RoomRef Room::Create(const MapTemplate& mapTemplate)
{
    RoomRef newRoom = make_shared<Room>();

    if (newRoom->Init(mapTemplate) == false)
    {
        return nullptr;
    }

    return newRoom;
}

bool Room::Init(const MapTemplate& mapTemplate)
{
    _mapTemplate = mapTemplate;

    // 자주 쓰는 맵 데이터를 멤버에 둔다.
    CacheRoomData();

    _cellMatrix.Init(_roomMinX, _roomMaxX, _roomMinY, _roomMaxY, CELL_SIZE);

    return true;
}

bool Room::Start()
{
    // 몬스터를 Room에 스폰한다. 마을처럼 몬스터 목록이 빈 룸은 건너뛴다.
    if (_monsterIds.empty() == false)
    {
        int32 kindOfMonster = static_cast<int32>(_monsterIds.size());
        for (int32 i = 0; i < _maxMonsterCount; i++)
        {
            int32 monsterTemplateId = _monsterIds[Utils::GetRandom(0, kindOfMonster - 1)];

            MonsterSpawnParams spawnParams;
            spawnParams.templateId = monsterTemplateId;
            SetRandomPos(&spawnParams.spawnPos, true, true);

            if (SpawnEntity<Monster>(spawnParams) == nullptr)
            {
                wcout << L"Room " << _roomId << L": 몬스터 " << monsterTemplateId << L" 스폰에 실패했습니다" << '\n';
                return false;
            }
        }
    }

    Update();

    return true;
}

void Room::Update()
{
    DoTimer(ROOM_UPDATE_INTERVAL_MS, &Room::Update);

    // 몬스터가 플레이어를 찾을 때 셀을 본다. 위치가 틱마다 바뀌므로 여기서 다시 채운다.
    UpdateCellMatrix();

    Protocol::S_MOVE movePkt;
    {
        for (auto pair : _entities)
        {
            EntityRef entity = pair.second;
            if (entity->IsPlayer())
                continue;

            Protocol::PosInfo* info = movePkt.add_info();
            info->CopyFrom(*entity->_posInfo);
        }

        // 몬스터가 없는 룸에서는 목록이 빈다. 틱마다 빈 패킷을 보내지 않도록 여기서 끝낸다.
        if (movePkt.info_size() == 0)
            return;

        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(movePkt);
        Broadcast(sendBuffer);
    }
}

void Room::TickEntity(EntityRef entity)
{
    int64 entityId = entity->GetEntityId();
    if (_entities.contains(entityId) == false)
        return;

    uint64 curTime = GetTickCount64();
    uint64 prevTime = entity->GetPrevTime();
    float deltaTime = (curTime - prevTime) / 1000.f;
    entity->SetPrevTime(curTime);

    entity->Tick(deltaTime);
}

bool Room::EnterPlayer(PlayerRef enterPlayer, RoomEnterData roomEnterData)
{
    Protocol::S_ENTER_ROOM enterRoomPkt;
    int64 enterPlayerId = enterPlayer->GetEntityId();

    if (AddEntity(enterPlayer) == false)
    {
        wcout << L"플레이어: " << enterPlayerId << "가 Room 입장에 실패했습니다" << '\n';

        if (auto session = enterPlayer->_session.lock())
        {
            enterRoomPkt.set_success(false);
            enterRoomPkt.set_enter_type(roomEnterData.enterType);
            enterRoomPkt.set_room_id(_roomId);

            SEND_PACKET(enterRoomPkt)
        }

        return false;
    }
    else
    {
        enterPlayer->OnEnterRoom(static_pointer_cast<Room>(shared_from_this()), roomEnterData.enterPos);

        // 룸 이동 중에 접속이 끊겼으면 이전 룸은 이 플레이어를 찾지 못한다. 퇴장과 저장을 여기서 이어 받는다.
        // AddEntity가 _room을 먼저 쓰고 여기서 표시를 읽는다. OnDisconnected는 표시를 먼저 쓰고 _room을 읽는다.
        // 그래서 둘 중 적어도 한쪽은 상대를 본다. 둘 다 보면 이 룸 큐에서 두 번 돌고, 두 번째는 퇴장에 실패해 저장하지 않는다.
        if (enterPlayer->_disconnected.load())
        {
            GameSession::LeaveGame(static_pointer_cast<Room>(shared_from_this()), enterPlayer);
            return false;
        }

        if (auto session = enterPlayer->_session.lock())
        {
            enterRoomPkt.set_success(true);
            enterRoomPkt.set_enter_type(roomEnterData.enterType);
            enterRoomPkt.set_room_id(_roomId);

            if(roomEnterData.enterPos.has_value())
                enterRoomPkt.mutable_enter_pos()->CopyFrom(roomEnterData.enterPos.value());

            SEND_PACKET(enterRoomPkt)
        }
    }
  
    return true;
}

bool Room::LeavePlayer(PlayerRef leavePlayer, bool transferRoom)
{
    const int64 leavePlayerId = leavePlayer->GetEntityId();

    if (RemoveEntity(leavePlayerId) == false)
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
            despawnPkt.add_entity_ids(leavePlayerId);

            SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(despawnPkt);
            Broadcast(sendBuffer);
        }

        // leavePlayer: Despawn Packet 전송
        if (auto session = leavePlayer->_session.lock())
        {
            if (transferRoom == false)
            {
                Protocol::S_DESPAWN despawnPkt;
                despawnPkt.add_entity_ids(leavePlayerId);

                SEND_PACKET(despawnPkt)
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

optional<PlayerSaveData> Room::HandleDisconnect(PlayerRef player)
{
    const int64 playerId = player->GetEntityId();

    if (Contains(playerId) == false)
        return nullopt;

    if (LeavePlayer(player, false) == false)
        return nullopt;

    if (player->IsDead() && player->ApplyTownRespawnForSave() == false)
        wcout << L"플레이어: " << playerId << L"에 마을 리스폰을 적용하지 못해 사망한 상태 그대로 저장합니다" << '\n';

    return player->MakeSaveData();
}

// 이 함수는 Room의 데이터를 쓰지 않는다. Room에 두는 이유는 큐 하나뿐이다.
// 여기서 세팅하는 enteringRoomId를 뒤이어 C_HandleEnterRoom이 같은 큐에서 읽는다.
void Room::C_HandleEnterMap(Protocol::C_ENTER_MAP pkt, PlayerRef player)
{
    // 잡이 도는 시점에 세션이 끊겼을 수 있다. 응답을 보낼 곳이 없으면 그대로 끝낸다.
    auto session = player->_session.lock();
    if (session == nullptr)
        return;

    const int32 roomId = pkt.room_id();

    // TODO: 나중에 레벨 이동이 생기면 검증 코드 추가
    // ...

    // TODO: Map 입장에 제한(ex. 인원수 제한)을 두고 싶다면 로직 추가
    // ...

    // 성공적인 Map 입장 처리
    {
        player->OnEnterMap(pkt.map_id(), roomId);

        Protocol::S_ENTER_MAP enterMapPkt;
        {
            enterMapPkt.set_success(true);
            enterMapPkt.set_map_id(pkt.map_id());
            enterMapPkt.set_room_id(roomId);

            SEND_PACKET(enterMapPkt)
        }
    }
}

void Room::C_HandleEnterRoom(Protocol::C_ENTER_ROOM pkt, PlayerRef player)
{
    auto session = player->_session.lock();
    if (session == nullptr)
        return;

    const Protocol::EnterType enterType = pkt.enter_type();

    // 입장 실패는 거절 사유와 무관하게 같은 모양으로 알린다. 클라이언트는 실패를 로그로만 남긴다.
    auto sendEnterRoomFailure = [&session, enterType]()
        {
            Protocol::S_ENTER_ROOM enterRoomPkt;
            enterRoomPkt.set_success(false);
            enterRoomPkt.set_enter_type(enterType);

            SEND_PACKET(enterRoomPkt)
        };

    if (optional<string> rejection = RoomTransfer::ValidateEnterRequest(pkt, player->GetEnteringRoomId()))
    {
        GLogger->Warning("C_HandleEnterRoom: {}", rejection.value());
        sendEnterRoomFailure();
        return;
    }

    switch (enterType)
    {
    case Protocol::ENTER_TYPE_INITIAL:
    {
        // 최초 입장. ServerPacketHandler가 목적지 Room의 JobQueue로 넘겨주므로
        // 여기서는 이미 입장할 Room 위에서 실행 중이다. 떠날 Room이 없다.
        RoomEnterData roomEnterData{};
        roomEnterData.nextRoomId = _roomId;
        roomEnterData.enterType = Protocol::ENTER_TYPE_INITIAL;
        roomEnterData.enterPos = *player->_posInfo;

        if (EnterPlayer(player, roomEnterData) == false)
            return;

        if (SpawnPlayer(player) == nullptr)
            return;

        ReplicateRoomData(player, true);

        break;
    }
    case Protocol::ENTER_TYPE_CROSS_MAP_TRANSFER:
    {
        // 맵 간 이동. 현재 Room의 JobQueue 위에서 실행되므로 퇴장까지 직접 처리한다.
        const int32 roomId = pkt.room_id();
        RoomRef enterRoom = GRoomManager->GetRoomRefFromRoomId(roomId);
        if (enterRoom == nullptr)
        {
            GLogger->Warning("맵 간 이동할 Room을 찾지 못함. roomId: {}", roomId);
            sendEnterRoomFailure();
            return;
        }

        RoomEnterData roomEnterData{};
        roomEnterData.nextRoomId = roomId;
        roomEnterData.enterType = Protocol::ENTER_TYPE_CROSS_MAP_TRANSFER;
        roomEnterData.enterPos = *player->_posInfo;

        if (TransferPlayer(player, roomEnterData) == false)
            return;

        // TransferPlayer가 목적지 큐에 EnterPlayer를 넣은 뒤에 실행된다.
        // 클라는 OpenLevel 직후라 월드가 비어 있으므로 자기 자신까지 다시 스폰해야 한다.
        enterRoom->DoAsync([enterRoom, player]()
            {
                if (enterRoom->SpawnPlayer(player) == nullptr)
                    return;

                enterRoom->ReplicateRoomData(player, true);
            });

        break;
    }
    case Protocol::ENTER_TYPE_SAME_MAP_TRANSFER:
    {
        // 포탈을 통한 같은 맵 내 이동. 현재 Room의 JobQueue 위에서 실행된다.
        const PortalTemplate* portal = FindPortal(pkt.portal_id());
        if (portal == nullptr)
        {
            GLogger->Warning("플레이어 {}가 현재 Room에 없는 포털 {}을 쓰려고 했다", player->GetEntityId(), pkt.portal_id());
            sendEnterRoomFailure();
            return;
        }

        const RoomEnterData roomEnterData = RoomTransfer::MakePortalEnterData(*portal, player->GetEntityId());

        RoomRef enterRoom = GRoomManager->GetRoomRefFromRoomId(roomEnterData.nextRoomId);
        if (enterRoom == nullptr)
        {
            GLogger->Warning("포탈 목적지 Room을 찾지 못함. roomId: {}", roomEnterData.nextRoomId);
            sendEnterRoomFailure();
            return;
        }

        if (TransferPlayer(player, roomEnterData) == false)
            return;

        // TransferPlayer가 목적지 큐에 EnterPlayer를 넣은 뒤에 실행된다.
        enterRoom->DoAsync([enterRoom, player]()
            {
                // 목적지 Room의 다른 플레이어들에게 내 등장을 알린다.
                if (enterRoom->SpawnPlayer(player) == nullptr)
                    return;

                // 클라는 HandleDespawnAll(true)로 내 액터를 유지하므로 나는 제외한다.
                enterRoom->ReplicateRoomData(player, false);
            });

        break;
    }
    default:
        // ValidateEnterRequest가 나머지 유형을 거절한다.
        break;
    }
}

void Room::C_HandleMove(Protocol::C_MOVE pkt)
{
	PlayerRef player = FindEntityAs<Player>(pkt.info().entity_id());
    if (player == nullptr)
        return;

	// 적용
	player->_posInfo->CopyFrom(pkt.info());

	// 이동 사실을 알린다 (본인 빼고)
	{
		Protocol::S_MOVE movePkt;
		{
            Protocol::PosInfo* info = movePkt.add_info();
			info->CopyFrom(pkt.info());
		}
		SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(movePkt);
		Broadcast(sendBuffer, player->GetEntityId());
	}
}

void Room::C_HandleBuyItem(Protocol::C_BUY_ITEM pkt, PlayerRef player)
{
    auto session = player->_session.lock();
    if (session == nullptr)
        return;

    Protocol::S_BUY_ITEM buyItemPkt;
    int32 templateId = pkt.template_id();
    int64 totalGold = 0;

    if (player->ProcessBuyItem(OUT buyItemPkt.mutable_updated_slots(), OUT totalGold, templateId) == false)
    {
        buyItemPkt.set_success(false);
        buyItemPkt.clear_updated_slots();

        SEND_PACKET(buyItemPkt)
        return;
    }

    // 아이템 구매 성공 처리
    {
        buyItemPkt.set_success(true);
        buyItemPkt.set_gold(totalGold);

        SEND_PACKET(buyItemPkt)
        cout << buyItemPkt.DebugString() << endl;
    }

    return;
}

void Room::C_HandleSellItem(Protocol::C_SELL_ITEM pkt, PlayerRef player)
{
    auto session = player->_session.lock();
    if (session == nullptr)
        return;

    Protocol::S_SELL_ITEM sellItemPkt;
    const Protocol::Slot& requestSlot = pkt.slot();
    Protocol::Slot* updatedSlot = sellItemPkt.mutable_updated_slot();
    int64 totalGold = 0;

    if (player->ProcessSellItem(requestSlot, OUT updatedSlot, OUT totalGold) == false)
    {
        sellItemPkt.set_success(false);

        SEND_PACKET(sellItemPkt)
        return;
    }

    // 아이템 판매 성공 처리
    {
        sellItemPkt.set_success(true);
        sellItemPkt.set_gold(totalGold);

        SEND_PACKET(sellItemPkt)
        cout << sellItemPkt.DebugString() << endl;
    }

}

void Room::C_HandleUseItem(Protocol::C_USE_ITEM pkt, PlayerRef player)
{
    auto session = player->_session.lock();
    if (session == nullptr)
        return;

    Protocol::S_USE_ITEM useItemPkt;
    const Protocol::Slot& targetSlot = pkt.slot();

    if (player->ProcessUseItem(targetSlot, ::GetTickCount64(), OUT useItemPkt) == false)
    {
        useItemPkt.set_success(false);

        SEND_PACKET(useItemPkt)
        return;
    }

    // 아이템 사용 성공 처리
    {
        useItemPkt.set_success(true);

        SEND_PACKET(useItemPkt)
        cout << useItemPkt.DebugString() << endl;
    }

}

void Room::C_HandleEquipGear(Protocol::C_EQUIP_GEAR pkt, PlayerRef player)
{
    const int64 entityId = player->GetEntityId();
    if (Contains(entityId) == false)
        return;

    // slot_id와 template_id는 처리 결과(장비 부위와 그 부위의 아이템)라서 ProcessEquipGear가 채운다.
    Protocol::S_EQUIP_GEAR equipGearPkt;
    {
        equipGearPkt.set_success(true);
        equipGearPkt.set_entity_id(entityId);
    }

    if (player->ProcessEquipGear(pkt.slot(), OUT equipGearPkt) == false)
    {
        if (SessionRef session = player->_session.lock())
        {
            equipGearPkt.set_success(false);
            SEND_PACKET(equipGearPkt)
        }

        return;
    }

    // 장착한 유저에게만 그대로 전송.
    // 잡이 기다리는 사이 끊겼으면 세션이 없다. 실패 응답과 같이 확인하고 보낸다.
    if (SessionRef session = player->_session.lock())
    {
        SEND_PACKET(equipGearPkt)
    }

    // 다른 유저들한테는 변경된 stat을 보내지 않는다.
    {
        equipGearPkt.clear_updated_slots();
        equipGearPkt.clear_updated_stat();
        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(equipGearPkt);
        Broadcast(sendBuffer, entityId);
    }

}

void Room::C_HandleUnequipGear(Protocol::C_UNEQUIP_GEAR pkt, PlayerRef player)
{
    const int64 entityId = player->GetEntityId();
    if (Contains(entityId) == false)
        return;

    // slot_id와 template_id는 처리 결과(장비 부위와 그 부위의 아이템)라서 ProcessUnequipGear가 채운다.
    Protocol::S_UNEQUIP_GEAR unequipGearPkt;
    {
        unequipGearPkt.set_success(true);
        unequipGearPkt.set_entity_id(entityId);
    }

    if (player->ProcessUnequipGear(pkt.slot(), OUT unequipGearPkt) == false)
    {
        if (SessionRef session = player->_session.lock())
        {
            unequipGearPkt.set_success(false);

            SEND_PACKET(unequipGearPkt)
        }
        return;
    }

    // 탈착한 유저에게만 그대로 전송.
    // 잡이 기다리는 사이 끊겼으면 세션이 없다. 실패 응답과 같이 확인하고 보낸다.
    if (SessionRef session = player->_session.lock())
    {
        SEND_PACKET(unequipGearPkt)
    }

    // 다른 유저들한테는 변경된 stat을 보내지 않는다.
    {
        unequipGearPkt.clear_updated_slots();
        unequipGearPkt.clear_updated_stat();
        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(unequipGearPkt);
        Broadcast(sendBuffer, entityId);
    }

}

void Room::C_HandleNormalAttack(Protocol::C_NORMAL_ATTACK pkt, PlayerRef player)
{
    const int64 entityId = player->GetEntityId();
    if (Contains(entityId) == false)
        return;
    
    // 일반 공격 사실을 Broadcast.
    {
        Protocol::S_NORMAL_ATTACK normalAttackPkt;
        {
            normalAttackPkt.set_entity_id(entityId);
            normalAttackPkt.set_combo(pkt.combo());
            // 공격한 순간의 방향은 클라이언트만 안다. _posInfo의 yaw는 마지막 이동 패킷의 값이다.
            normalAttackPkt.set_yaw(pkt.yaw());
        }
        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(normalAttackPkt);
        Broadcast(sendBuffer, entityId);
    }
}

void Room::C_HandleRespawn(Protocol::C_RESPAWN pkt, PlayerRef player)
{
    // 1. Validate + Find Respawn Room
    RoomRef respawnRoom = nullptr;
    Protocol::PosInfo respawnPos;
    const Protocol::RespawnType respawnType = pkt.respawn_type();

    // 지원하는 유형은 마을 리스폰뿐이라 판정을 통과하면 마을의 리스폰 지점을 찾는다.
    optional<string> rejection = RoomTransfer::ValidateRespawn(player->IsDead(), respawnType);
    if (rejection.has_value() == false
        && player->FindTownRespawnPoint(OUT respawnRoom, OUT respawnPos) == false)
    {
        rejection = "리스폰할 위치를 찾지 못했습니다.";
    }

    if (rejection.has_value())
    {
        cout << "C_HandleRespawn: " << rejection.value() << '\n';

        if (auto session = player->_session.lock())
        {
            Protocol::S_RESPAWN respawnPkt;
            respawnPkt.set_success(false);
            respawnPkt.set_respawn_type(respawnType);
            respawnPkt.set_error_message(rejection.value());

            SEND_PACKET(respawnPkt)
        }

        return;
    }

    // 2. Process Respawn
    if (shared_from_this() == respawnRoom)
    {
        if (HandleRespawn(player, respawnType, respawnPos) == false)
            return;

        // 같은 룸이면 룸 이동이 없어서 다른 플레이어에게 알릴 경로가 없다.
        // 사망한 모습을 지우고 리스폰 위치에 다시 스폰시킨다. 룸이 다를 때와 같은 결과다.
        const int64 playerId = player->GetEntityId();

        Protocol::S_DESPAWN despawnPkt;
        despawnPkt.add_entity_ids(playerId);
        Broadcast(ServerPacketHandler::MakeSerializedPacket(despawnPkt), playerId);

        Protocol::S_SPAWN spawnPkt;
        spawnPkt.add_entities()->CopyFrom(*player->_entityInfo);
        Broadcast(ServerPacketHandler::MakeSerializedPacket(spawnPkt), playerId);
    }
    else
    {
        // 죽은 Room과 다른 Room에서 Respawn하는 경우 진입
        RoomEnterData enterData = RoomEnterData{};
        // 1) Room 이동 데이터 설정
        {
            enterData.nextRoomId = respawnRoom->GetRoomId();
            enterData.enterType = Protocol::ENTER_TYPE_RESPAWN;

            Protocol::PosInfo enterPos;
            enterPos.CopyFrom(respawnPos);
            enterPos.set_entity_id(player->GetEntityId());
            enterData.enterPos = std::move(enterPos);
        }

        // 2) Room을 이동 -> 리스폰 패킷 전송 -> Room 정보 전송
        if (TransferPlayer(player, enterData) == false)
            return;

        // TransferPlayer가 목적지 큐에 EnterPlayer를 넣은 뒤에 실행된다.
        respawnRoom->DoAsync([respawnRoom, player, respawnType, respawnPos]()
            {
                // 부활 처리를 먼저 해야 다른 플레이어에게 죽은 상태가 나가지 않는다.
                // 룸 이동 중에 접속이 끊기면 세션이 없어 실패한다. 같은 룸 경로처럼 실패하면 멈춘다.
                if (respawnRoom->HandleRespawn(player, respawnType, respawnPos) == false)
                    return;

                respawnRoom->SpawnPlayer(player);
                respawnRoom->ReplicateRoomData(player, false);
            });
    }

}

void Room::C_HandleChat(Protocol::C_CHAT pkt, PlayerRef player)
{
	// 같은 Room의 모든 플레이어에게 그대로 중계한다 (본인 포함).
	Protocol::S_CHAT chatPkt;
	chatPkt.set_entity_id(player->GetEntityId());
	chatPkt.set_msg(pkt.msg());

	SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(chatPkt);
	Broadcast(sendBuffer);
}

void Room::HandleNormalAttack(int32 combo, CreatureRef creature)
{
    Protocol::S_NORMAL_ATTACK normalAttackPkt;
    {
        normalAttackPkt.set_entity_id(creature->GetEntityId());
        normalAttackPkt.set_combo(combo);
        normalAttackPkt.set_yaw(creature->_posInfo->yaw());

        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(normalAttackPkt);
        Broadcast(sendBuffer);
    }
}

void Room::HandleHit(EntityRef attacker, Protocol::AttackInfo attackInfo)
{
    // 그사이 룸을 떠났으면 공격은 없던 것이 된다.
    // id만 보면 그사이 나갔다 다시 들어온 같은 엔티티도 통과하므로 객체까지 대조한다.
    if (FindEntityAs<Entity>(attacker->GetEntityId()) != attacker)
        return;

    // 1) 해당 공격에 맞은 대상을 찾는다.
    vector<CreatureRef> targets;
    if (attackInfo.has_target_id())
    {
        int64 targetId = attackInfo.target_id();
        if(Contains(targetId) == false)
            return;

        // TODO: 피격이 가능한 대상?
        if (CreatureRef creature = FindEntityAs<Creature>(targetId))
        {
            targets.push_back(creature);
        }
    }
    else
    {

    }
    
    // 2) 판정 결과를 알린다.
    for (const CreatureRef& target : targets)
    {
        optional<Combat::HitResult> result = Combat::ResolveHit(attacker, target, attackInfo);
        if (result.has_value() == false)
            continue;

        Protocol::S_HIT hitPkt;
        {
            hitPkt.set_entity_id(target->GetEntityId());
            hitPkt.set_damage(attackInfo.damage());
            hitPkt.set_updated_hp(result->updatedHp);

            SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(hitPkt);
            Broadcast(sendBuffer);
        }

        if (result->kill.has_value())
        {
            HandleMonsterKill(result->kill->killer, result->kill->reward);
        }

        if (result->isDead)
        {
            HandleDie(target);
        }
    }

}

void Room::HandleMonsterKill(PlayerRef player, const Protocol::Reward& reward)
{
    Protocol::S_REWARD_RESULT rewardResultPkt;
    {
        rewardResultPkt.set_type(Protocol::REWARD_TYPE_MONSTER_KILL);
        rewardResultPkt.mutable_reward()->CopyFrom(reward);
    }

    player->OnGetReward(rewardResultPkt);

    if (auto session = player->_session.lock())
    {
        SEND_PACKET(rewardResultPkt)
    }
}

void Room::HandleDie(CreatureRef creature)
{
    int64 entityId = creature->GetEntityId();

    Protocol::S_DIE diePkt;
    {
        diePkt.set_entity_id(entityId);

        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(diePkt);
        Broadcast(sendBuffer);
    }

    // 사망한 플레이어는 리스폰할 때까지 룸에 남는다. 리스폰이 이 룸에서 떠나는 것부터 시작하기 때문이다.
    if (creature->IsPlayer())
        return;

    RemoveEntity(entityId);
}

bool Room::HandleRespawn(PlayerRef player, Protocol::RespawnType respawnType, Protocol::PosInfo respawnPos)
{
    auto session = player->_session.lock();
    if (session == nullptr)
        return false;

    Protocol::S_RESPAWN respawnPkt;
    if (_respawnPoint == nullptr)
    {
        wcout << "리스폰 위치가 없는 Room에서 리스폰 시도" << '\n';
        {
            respawnPkt.set_success(false);
            respawnPkt.set_error_message(string("No Respawn Point"));

            SEND_PACKET(respawnPkt)
        }

        return false;
    }

    // 호출자가 FindTownRespawnPoint로 찾아 넘겨준 위치를 쓴다.
    shared_ptr<Protocol::PosInfo> targetPos = make_shared<Protocol::PosInfo>(std::move(respawnPos));

    if (player->ProcessRespawn(respawnType, targetPos, respawnPkt) == false)
    {
        wcout << "ProcessRespawn가 false를 반환" << '\n';
        {
            respawnPkt.set_success(false);
            respawnPkt.set_error_message(string("Fail to Respawn"));

            SEND_PACKET(respawnPkt)
        }

        return false;
    }

    // 리스폰 성공 처리
    SEND_PACKET(respawnPkt)

    return true;
}

void Room::ReplicateRoomData(PlayerRef player, bool includeThisPlayer)
{
    int64 playerId = player->GetEntityId();

    // 해당 플레이어에게 Room 엔티티 전송
    Protocol::S_SPAWN spawnPkt;
    if (auto session = player->_session.lock())
    {
        for (auto& item : _entities)
        {
            if (!includeThisPlayer && item.second->GetEntityId() == playerId)
                continue;

            // 플레이어의 장비 외형은 _entityInfo의 equipped_gear_summary에 실려 간다.
            spawnPkt.add_entities()->CopyFrom(*item.second->_entityInfo);
        }

        SEND_PACKET(spawnPkt)
    }
}

vector2D Room::GetRandomLocation(bool usePadding)
{
    float paddingX = usePadding ? LOCATION_PADDING_X : 0.f;
    float paddingY = usePadding ? LOCATION_PADDING_Y : 0.f;

    float paddedMinX = _roomMinX + paddingX;
    float paddedMaxX = _roomMaxX - paddingX;
    float paddedMinY = _roomMinY + paddingY;
    float paddedMaxY = _roomMaxY - paddingY;

    vector2D randomPos;

    randomPos.x = Utils::GetRandom(paddedMinX, paddedMaxX);
    randomPos.y = Utils::GetRandom(paddedMinY, paddedMaxY);

    return randomPos;
}

const PortalTemplate* Room::FindPortal(int32 portalId) const
{
    for (const PortalTemplate& portal : _mapTemplate.portals)
    {
        if (portal.portalId == portalId)
            return &portal;
    }

    return nullptr;
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

pair<PlayerRef, float> Room::FindClosestPlayer(Protocol::PosInfo* posInfo, float range)
{
    const vector2D center = ToPlanePos(*posInfo);

    PlayerRef closestPlayer = nullptr;
    float minDist = -1.f;
    float squareRange = range * range;

    // 셀 행렬은 탐색 상자에 걸친 칸의 엔티티를 준다. 대상인지와 실제 거리는 여기서 판정한다.
    for (int64 entityId : _cellMatrix.QueryRange(center, range))
    {
        auto it = _entities.find(entityId);
        if (it == _entities.end())
            continue;

        if (PlayerRef player = dynamic_pointer_cast<Player>(it->second))
        {
            // 사망한 플레이어는 리스폰할 때까지 룸에 남지만 대상이 아니다.
            if (player->IsDead())
                continue;

            float squareDist = MathUtil::Distance(posInfo, player->_posInfo, true);
            if (squareRange < squareDist)
                continue;

            if (minDist < 0.f || squareDist < minDist)
            {
                minDist = squareDist;
                closestPlayer = player;
            }
        }
    }

    return make_pair(closestPlayer, minDist);
}

PlayerRef Room::SpawnPlayer(int64 entityId)
{
    PlayerRef targetPlayer = FindEntityAs<Player>(entityId);
    if (targetPlayer == nullptr)
        return nullptr;

    return SpawnPlayer(targetPlayer);
}

PlayerRef Room::SpawnPlayer(PlayerRef targetPlayer)
{
    // 룸 이동 뒤에 예약한 스폰 잡은 그사이 EnterPlayer가 접속 종료로 플레이어를 뺐어도 돈다.
    // 룸에 없는 플레이어를 알리면 다른 클라이언트에 유령이 남는다.
    if (Contains(targetPlayer->GetEntityId()) == false)
        return nullptr;

    Protocol::S_SPAWN spawnPkt;
    {
        Protocol::EntityInfo* entityInfo = spawnPkt.add_entities();
        entityInfo->CopyFrom(*targetPlayer->_entityInfo);

        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(spawnPkt);
        Broadcast(sendBuffer);
    }

    return targetPlayer;
}

void Room::Broadcast(SendBufferRef sendBuffer, int64 exceptId)
{
	for (auto& item : _entities)
	{
		PlayerRef player = dynamic_pointer_cast<Player>(item.second);
		if (player == nullptr)
			continue;
		if (player->GetEntityId() == exceptId)
			continue;

		if (GameSessionRef session = player->_session.lock())
			session->Send(sendBuffer);
	}
}

bool Room::AddEntity(EntityRef entity)
{
    if (entity == nullptr)
        return false;

    int64 entityId = entity->GetEntityId();
	if (_entities.contains(entityId))
		return false;

	_entities.insert(make_pair(entityId, entity));

    // 틱과 AI는 소속 룸의 타이머로 돈다. 룸을 먼저 알려야 Start가 타이머를 건다.
    entity->_room.store(GetRoomRef());

    if (entity->_hasBegunPlay == false)
    {
        entity->_hasBegunPlay = true;
        entity->Start();
    }

	return true;
}

bool Room::RemoveEntity(int64 entityId)
{
	if (_entities.contains(entityId) == false)
		return false;

    EntityRef entity = _entities[entityId];

    // 셀 행렬에서 엔티티를 삭제한다. 격자 밖에 있으면 어느 셀에도 없다.
    _cellMatrix.Remove(entityId, ToPlanePos(*entity->_posInfo));

    // 엔티티를 삭제한다.
	_entities.erase(entityId);

	return true;
}

void Room::CacheRoomData()
{
    _roomId = _mapTemplate.templateId;

    _roomCenterPos.x = _mapTemplate.center.x;
    _roomCenterPos.y = _mapTemplate.center.y;
    _roomCenterPos.z = _mapTemplate.center.z;

    _depthHalfExtent = _mapTemplate.depthHalfExtent;
    _widthHalfExtent = _mapTemplate.widthHalfExtent;

    _roomMinX = _roomCenterPos.x - _depthHalfExtent;
    _roomMaxX = _roomCenterPos.x + _depthHalfExtent;
    _roomMinY = _roomCenterPos.y - _widthHalfExtent;
    _roomMaxY = _roomCenterPos.y + _widthHalfExtent;

    _maxMonsterCount = _mapTemplate.maxMonsterCount;
    _monsterRespawnTime = _mapTemplate.monsterRespawnTime;
    _monsterIds = _mapTemplate.monsterIds;

    // 리스폰 포인트 저장
    if (_mapTemplate.respawnPoint.has_value())
    {
        const TemplatePos& point = _mapTemplate.respawnPoint.value();

        _respawnPoint = make_shared<Protocol::PosInfo>();
        _respawnPoint->mutable_pos()->set_x(point.x);
        _respawnPoint->mutable_pos()->set_y(point.y);
        _respawnPoint->mutable_pos()->set_z(point.z);
        _respawnPoint->set_yaw(0.f);
        _respawnPoint->set_state(Protocol::MoveState::MOVE_STATE_IDLE);

        // 이 플래그가 없으면 GetRespawnPoint()가 항상 nullptr을 반환해 마을 리스폰이 실패한다.
        _hasRespawnPoint = true;
    }

}

void Room::UpdateCellMatrix()
{
    vector<pair<int64, vector2D>> positions;
    positions.reserve(_entities.size());

    for (auto& [entityId, entity] : _entities)
        positions.emplace_back(entityId, ToPlanePos(*entity->_posInfo));

    _cellMatrix.Update(positions);
}