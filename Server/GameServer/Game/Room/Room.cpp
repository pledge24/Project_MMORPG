#include "Core/pch.h"
#include "Game/Room/Room.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/Monster.h"
#include "Game/Combat/Combat.h"
#include "Game/Room/MoveValidation.h"
#include "Network/ProgressCoordinator.h"

namespace
{
    // 셀 행렬은 평면 좌표만 본다.
    vector2D ToPlanePos(const Protocol::PosInfo& posInfo)
    {
        return vector2D(posInfo.pos().x(), posInfo.pos().y());
    }

    // 룸에서 빠진 끊긴 플레이어의 저장 사본. 사망한 채 끊겼으면 마을 리스폰을 적용해 저장한다.
    PlayerSaveData MakeDisconnectSaveData(const PlayerRef& player)
    {
        if (player->IsDead())
        {
            if (optional<TownRespawn> town = RoomTransfer::FindTownRespawn())
                player->ApplyTownRespawnForSave(town.value());
            else
                GLogger->Error("플레이어 {}에 마을 리스폰을 적용하지 못해 사망한 상태 그대로 저장합니다(맵 표에 마을 리스폰 지점이 없다)", player->GetEntityId());
        }

        return player->MakeSaveData();
    }
}

RoomRef Room::Create(const MapTemplate& mapTemplate)
{
    RoomRef newRoom = make_shared<Room>();
    newRoom->Init(mapTemplate);
    return newRoom;
}

void Room::Init(const MapTemplate& mapTemplate)
{
    _mapTemplate = mapTemplate;

    CacheRoomData();

    _cellMatrix.Init(_roomMinX, _roomMaxX, _roomMinY, _roomMaxY, CELL_SIZE);
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
    _monsterIds = _mapTemplate.monsterIds;
}

bool Room::Start()
{
    // 몬스터 스폰. 마을처럼 몬스터 목록이 빈 룸은 건너뛴다.
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
                GLogger->Error("룸 {}: 몬스터 {}를 스폰하지 못했다", _roomId, monsterTemplateId);
                return false;
            }
        }
    }

    _lastTickTime = GetTickCount64();
    DoTimer(ROOM_TICK_INTERVAL_MS, &Room::RunScheduledTick);

    return true;
}

void Room::Tick(float deltaTime)
{
    // 몬스터가 플레이어를 찾을 때 셀을 본다. 엔티티 Tick보다 먼저 채워야 이번 틱의 탐색이 지난 틱의 위치를 본다.
    UpdateCellMatrix();

    // 엔티티의 Tick이 룸에서 엔티티를 뺄 수 있다(피격으로 사망한 몬스터). 순회 중인 맵을 바꾸지 않도록 목록을 떠 둔다.
    vector<EntityRef> entities;
    entities.reserve(_entities.size());
    for (auto& [entityId, entity] : _entities)
        entities.push_back(entity);

    for (const EntityRef& entity : entities)
    {
        // 이번 틱에서 앞선 엔티티가 뺀 엔티티는 돌리지 않는다.
        if (FindEntityAs<Entity>(entity->GetEntityId()) != entity)
            continue;

        entity->Tick(deltaTime);
    }

    _timeSinceMoveSend += deltaTime;
    if (_timeSinceMoveSend >= MOVE_SEND_INTERVAL)
    {
        _timeSinceMoveSend = 0.f;
        BroadcastMonsterMoves();
    }
}

void Room::RunScheduledTick()
{
    const uint64 now = GetTickCount64();
    const float deltaTime = (now - _lastTickTime) / 1000.f;
    _lastTickTime = now;

    DoTimer(ROOM_TICK_INTERVAL_MS, &Room::RunScheduledTick);

    Tick(deltaTime);
}

void Room::BroadcastMonsterMoves()
{
    Protocol::S_MOVE movePkt;
    for (auto& [entityId, entity] : _entities)
    {
        if (entity->IsPlayer())
            continue;

        Protocol::EntityMove* move = movePkt.add_moves();
        move->set_entity_id(entityId);
        move->mutable_pos_info()->CopyFrom(entity->GetPosInfo());
    }

    if (movePkt.moves_size() == 0)
        return;

    Broadcast(ServerPacketHandler::MakeSerializedPacket(movePkt));
}

bool Room::EnterPlayer(PlayerRef enterPlayer, RoomEnterData roomEnterData)
{
    Protocol::S_ENTER_ROOM enterRoomPkt;
    int64 enterPlayerId = enterPlayer->GetEntityId();

    if (AddEntity(enterPlayer) == false)
    {
        GLogger->Warning("플레이어 {}가 Room {} 입장에 실패했습니다", enterPlayerId, _roomId);

        // 룸 이동 중에 끊겼다. 떠난 룸은 이 플레이어를 찾지 못하고 이 룸에도 들어오지 못했으므로, 저장을 여기서 넘긴다.
        // 넘기지 않으면 접속 종료가 건 저장 대기를 풀 곳이 없다.
        if (enterPlayer->GetPresence() == Player::Presence::LEFT_IN_ROOM)
        {
            GProgressCoordinator->SaveLeftPlayer(enterPlayer, MakeDisconnectSaveData(enterPlayer));
            return false;
        }

        if (auto session = enterPlayer->GetSession())
        {
            enterRoomPkt.set_result(Protocol::ENTRY_RESULT_INTERNAL_ERROR);
            SendPacket(session, enterRoomPkt);
        }

        return false;
    }

    enterPlayer->OnEnterRoom(static_pointer_cast<Room>(shared_from_this()), roomEnterData.enterPos);

    // 들어오는 사이에 접속이 끊겼는지 여기서 판정한다. AddEntity가 _room을 먼저 쓰고 진행 표시를 바꾼다.
    // 접속 종료(ProgressCoordinator::OnDisconnected)는 진행 표시를 먼저 바꾸고 _room을 읽는다.
    // 그래서 접속 종료가 IN_ROOM을 보았다면 _room이 이미 이 룸이다. 둘 다 이 룸 큐에 퇴장을 넣으면 두 번째는 퇴장에 실패해 저장하지 않는다.
    switch (enterPlayer->MarkEnteredRoom())
    {
    case Player::Presence::IN_ROOM:
        break;

    case Player::Presence::LEFT_BEFORE_ROOM:
        // 첫 입장 전에 끊겼다. 바뀐 것이 없으므로 저장하지 않고, 접속 종료가 이미 저장 대기를 풀었다.
        // 여기서 저장하면 그사이 대기 없이 입장한 새 세션의 불러오기 뒤에 이 저장이 돈다.
        RemoveEntity(enterPlayerId);
        return false;

    case Player::Presence::LEFT_IN_ROOM:
        // 룸 이동 중에 끊겼다. 이전 룸은 이 플레이어를 찾지 못하므로 퇴장과 저장을 여기서 이어 받는다.
        GProgressCoordinator->LeaveRoomAndSave(static_pointer_cast<Room>(shared_from_this()), enterPlayer);
        return false;

    default:
        break;
    }

    // 입장 알림. 내 엔티티는 입장 응답에만 싣고, 스폰은 다른 엔티티만 싣는다.
    if (auto session = enterPlayer->GetSession())
    {
        enterRoomPkt.set_result(Protocol::ENTRY_RESULT_SUCCESS);
        enterRoomPkt.mutable_my_entity()->CopyFrom(enterPlayer->GetEntityInfo());

        SendPacket(session, enterRoomPkt);
    }

    SpawnPlayer(enterPlayer);
    ReplicateRoomData(enterPlayer);

    return true;
}

bool Room::LeavePlayer(PlayerRef leavePlayer, bool transferRoom)
{
    const int64 leavePlayerId = leavePlayer->GetEntityId();

    if (RemoveEntity(leavePlayerId) == false)
    {
        if (transferRoom)
            GLogger->Warning("플레이어 {}가 Room 이동 중 현재 Room {} 퇴장에 실패했습니다", leavePlayerId, _roomId);
        else
            GLogger->Warning("플레이어 {}가 Room {} 퇴장에 실패했습니다", leavePlayerId, _roomId);

        return false;
    }

    {
        {
            Protocol::S_DESPAWN despawnPkt;
            despawnPkt.add_entity_ids(leavePlayerId);

            SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(despawnPkt);
            Broadcast(sendBuffer);
        }

        if (auto session = leavePlayer->GetSession())
        {
            if (transferRoom == false)
            {
                Protocol::S_DESPAWN despawnPkt;
                despawnPkt.add_entity_ids(leavePlayerId);

                SendPacket(session, despawnPkt);
            }
        }
    }

    return true;
}

optional<PlayerSaveData> Room::HandleDisconnect(PlayerRef player)
{
    // 소속 룸이 아니면 이 플레이어의 상태는 다른 룸 큐의 것이다. 여기서 읽으면 그 큐와 경쟁한다.
    if (player->GetRoom().get() != this)
        return nullopt;

    // 다른 맵의 로드를 기다리던 플레이어는 룸에 넣지 않았으므로 빼지 않고 저장한다.
    if (player->FinishMapLoad(_roomId))
        return MakeDisconnectSaveData(player);

    const int64 playerId = player->GetEntityId();

    if (Contains(playerId) == false)
        return nullopt;

    if (LeavePlayer(player, false) == false)
        return nullopt;

    return MakeDisconnectSaveData(player);
}

void Room::MovePlayer(const PlayerRef& player, const RoomRef& destination, const Protocol::PosInfo& enterPos)
{
    LeavePlayer(player, true);

    // 맵이 바뀌면 클라이언트가 그 맵을 다 열 때까지 어느 룸에도 넣지 않는다. 로드 완료가 목적지 룸 큐에서 입장시킨다.
    if (destination->GetMapId() != GetMapId())
    {
        player->BeginMapLoad(destination, enterPos);
        return;
    }

    RoomEnterData enterData;
    enterData.nextRoomId = destination->GetRoomId();
    enterData.enterPos = enterPos;
    destination->DoAsync(&Room::EnterPlayer, player, enterData);
}

void Room::C_HandleMapLoadComplete(PlayerRef player)
{
    // 잡을 넣은 뒤 다른 룸으로 옮겼으면 이 룸 큐는 그 플레이어의 상태를 읽지 않는다.
    if (player->GetRoom().get() != this)
        return;

    // 맵 로드를 기다리지 않는 플레이어의 로드 완료는 버린다. 받으면 룸에 있는 플레이어가 한 번 더 들어간다.
    if (player->FinishMapLoad(_roomId) == false)
    {
        GLogger->Warning("플레이어 {}: 맵 로드를 기다리지 않는데 온 맵 로드 완료를 버린다", player->GetEntityId());
        return;
    }

    // 맵 로드를 시작할 때 위치를 목적지로 바꿔 두었다.
    RoomEnterData enterData;
    enterData.nextRoomId = _roomId;
    enterData.enterPos = player->GetPosInfo();
    EnterPlayer(player, enterData);
}

void Room::C_HandleUsePortal(Protocol::C_USE_PORTAL pkt, PlayerRef player)
{
    GameSessionRef session = player->GetSession();
    if (session == nullptr)
        return;

    // 맵 로드를 기다리거나 다른 룸으로 옮기는 중이면 이 룸에 없다. 이 룸에서 퇴장시킬 수 없으므로 버린다.
    if (Contains(player->GetEntityId()) == false)
        return;

    Protocol::S_USE_PORTAL usePortalPkt;
    auto reject = [&](Protocol::EntryResult result, const string& reason)
        {
            GLogger->Warning("C_HandleUsePortal: 플레이어 {}: {}", player->GetEntityId(), reason);
            usePortalPkt.set_result(result);
            SendPacket(session, usePortalPkt);
        };

    const PortalTemplate* portal = FindPortal(pkt.portal_id());
    if (portal == nullptr)
    {
        reject(Protocol::ENTRY_RESULT_INVALID_PORTAL, format("현재 룸 {}에 없는 포털 {}", _roomId, pkt.portal_id()));
        return;
    }

    if (optional<string> rejection = RoomTransfer::ValidatePortalUse(*portal, _mapTemplate.portalRadius, player->GetPosInfo()))
    {
        reject(Protocol::ENTRY_RESULT_TOO_FAR_FROM_PORTAL, rejection.value());
        return;
    }

    RoomRef destination = GRoomManager->FindRoom(portal->dstRoomId);
    if (destination == nullptr)
    {
        reject(Protocol::ENTRY_RESULT_INTERNAL_ERROR, format("포털 {}의 목적지 룸 {}이 없다", portal->portalId, portal->dstRoomId));
        return;
    }

    // 응답을 먼저 보낸다. 같은 맵이면 목적지 룸 큐가 곧바로 S_ENTER_ROOM을 보낼 수 있다.
    usePortalPkt.set_result(Protocol::ENTRY_RESULT_SUCCESS);
    if (destination->GetMapId() != GetMapId())
        usePortalPkt.set_map_id(destination->GetMapId());
    SendPacket(session, usePortalPkt);

    const RoomEnterData enterData = RoomTransfer::MakePortalEnterData(*portal);
    MovePlayer(player, destination, enterData.enterPos.value());
}

void Room::C_HandleMove(Protocol::C_MOVE pkt, PlayerRef player)
{
    // 대상은 패킷의 엔티티 번호가 아니라 보낸 사람이다. 그사이 룸을 떠났으면 버린다.
    const int64 playerId = player->GetEntityId();
    if (Contains(playerId) == false)
        return;

    // 직전에 받아들인 위치에서 흐른 시간으로 갈 수 있는 곳인지 본다. 어기면 버리고, 서버의 위치는 그대로 둔다.
    const uint64 now = ::GetTickCount64();
    const vector2D from = MathUtil::PosInfoToVector2D(&player->GetPosInfo());
    const vector2D to = MathUtil::PosInfoToVector2D(&pkt.pos_info());
    if (optional<string> rejection = MoveValidation::Validate(from, to, now - player->GetLastPositionTick(),
        MoveValidation::Bounds{ _roomMinX, _roomMaxX, _roomMinY, _roomMaxY }))
    {
        GLogger->Warning("플레이어 {}의 이동을 버린다: {}", playerId, rejection.value());
        return;
    }

    player->ApplyMove(pkt.pos_info(), now);

    Protocol::S_MOVE movePkt;
    Protocol::EntityMove* move = movePkt.add_moves();
    move->set_entity_id(playerId);
    *move->mutable_pos_info() = player->GetPosInfo();
    Broadcast(ServerPacketHandler::MakeSerializedPacket(movePkt), playerId);
}

void Room::C_HandleNormalAttack(Protocol::C_NORMAL_ATTACK pkt, PlayerRef player)
{
    const int64 entityId = player->GetEntityId();
    if (Contains(entityId) == false)
        return;

    GameSessionRef session = player->GetSession();

    // 사망한 플레이어는 리스폰할 때까지 룸에 남으므로 여기서 거른다.
    Protocol::S_NORMAL_ATTACK normalAttackPkt;
    if (player->IsDead())
    {
        normalAttackPkt.set_result(Protocol::COMBAT_RESULT_DEAD);
        if (session)
            SendPacket(session, normalAttackPkt);
        return;
    }

    normalAttackPkt.set_result(Protocol::COMBAT_RESULT_SUCCESS);
    if (session)
        SendPacket(session, normalAttackPkt);

    // 공격한 순간의 방향은 클라이언트만 안다. _posInfo의 yaw는 마지막 이동 패킷의 값이다.
    BroadcastAttack(entityId, pkt.combo(), pkt.yaw());
}

void Room::C_HandleRespawn(Protocol::C_RESPAWN pkt, PlayerRef player)
{
    GameSessionRef session = player->GetSession();
    if (session == nullptr)
        return;

    // 맵 로드를 기다리거나 다른 룸으로 옮기는 중이면 이 룸에 없다. 이 룸에서 퇴장시킬 수 없으므로 버린다.
    if (Contains(player->GetEntityId()) == false)
        return;

    Protocol::S_RESPAWN respawnPkt;
    auto reject = [&](Protocol::EntryResult result)
        {
            GLogger->Warning("C_HandleRespawn: 플레이어 {}의 리스폰을 거절한다. 결과 {}", player->GetEntityId(), Protocol::EntryResult_Name(result));
            respawnPkt.set_result(result);
            SendPacket(session, respawnPkt);
        };

    // 지원하는 유형은 마을 리스폰뿐이라 판정을 통과하면 마을의 리스폰 지점을 찾는다.
    const Protocol::RespawnType respawnType = pkt.respawn_type();
    if (optional<Protocol::EntryResult> rejection = RoomTransfer::ValidateRespawn(player->IsDead(), respawnType))
    {
        reject(rejection.value());
        return;
    }

    optional<TownRespawn> town = RoomTransfer::FindTownRespawn();
    RoomRef destination = town.has_value() ? GRoomManager->FindRoom(town->roomId) : nullptr;
    if (destination == nullptr)
    {
        reject(Protocol::ENTRY_RESULT_INTERNAL_ERROR);
        return;
    }

    // 부활 처리를 이 룸에서 먼저 한다. 목적지에서 다른 플레이어에게 죽은 상태가 나가지 않는다.
    RespawnResult result = player->ProcessRespawn(respawnType, town->pos);

    // 응답을 먼저 보낸다. 같은 맵이면 목적지 룸 큐가 곧바로 S_ENTER_ROOM을 보낼 수 있다.
    respawnPkt.set_result(Protocol::ENTRY_RESULT_SUCCESS);
    if (destination->GetMapId() != GetMapId())
        respawnPkt.set_map_id(destination->GetMapId());
    *respawnPkt.mutable_changed_stats() = std::move(result.updatedStats);
    SendPacket(session, respawnPkt);

    // 사망한 룸으로 다시 리스폰해도 퇴장과 입장을 거친다. 다른 플레이어는 사망한 모습의 디스폰과 리스폰한 모습의 스폰을 받는다.
    MovePlayer(player, destination, player->GetPosInfo());
}

void Room::HandleNormalAttack(int32 combo, CreatureRef creature)
{
    BroadcastAttack(creature->GetEntityId(), combo, creature->GetPosInfo().yaw());
}

void Room::BroadcastAttack(int64 attackerId, int32 combo, float yaw)
{
    Protocol::S_ATTACK attackPkt;
    {
        attackPkt.set_entity_id(attackerId);
        attackPkt.set_combo(combo);
        attackPkt.set_yaw(yaw);
    }

    // 공격한 플레이어는 자기 공격을 이미 재생했으므로 받지 않는다. 몬스터의 번호는 어느 플레이어와도 겹치지 않는다.
    Broadcast(ServerPacketHandler::MakeSerializedPacket(attackPkt), attackerId);
}

void Room::HandleHit(EntityRef attacker, AttackInfo attackInfo)
{
    // 그사이 룸을 떠났으면 공격은 없던 것이 된다.
    // id만 보면 그사이 나갔다 다시 들어온 같은 엔티티도 통과하므로 객체까지 대조한다.
    if (FindEntityAs<Entity>(attacker->GetEntityId()) != attacker)
        return;

    vector<CreatureRef> targets;
    if (attackInfo.targetId.has_value())
    {
        int64 targetId = attackInfo.targetId.value();
        if(Contains(targetId) == false)
            return;

        // 피격은 Creature만 받는다.
        if (CreatureRef creature = FindEntityAs<Creature>(targetId))
        {
            targets.push_back(creature);
        }
    }

    for (const CreatureRef& target : targets)
    {
        optional<Combat::HitResult> result = Combat::ResolveHit(attacker, target, attackInfo);
        if (result.has_value() == false)
            continue;

        Protocol::S_HIT hitPkt;
        {
            hitPkt.set_entity_id(target->GetEntityId());
            hitPkt.set_damage(attackInfo.damage);
            hitPkt.set_hp(result->updatedHp);

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

void Room::HandleMonsterKill(PlayerRef player, const Reward& reward)
{
    RewardResult result = player->OnGetReward(reward);

    Protocol::S_REWARD rewardPkt;
    {
        rewardPkt.set_exp(result.exp);
        rewardPkt.set_gold(result.gold);

        if (result.levelUp.has_value())
        {
            Protocol::LevelUp* levelUp = rewardPkt.mutable_level_up();
            levelUp->set_level(result.levelUp->level);
            *levelUp->mutable_changed_stats() = std::move(result.levelUp->changedStats);
        }
    }

    if (auto session = player->GetSession())
    {
        SendPacket(session, rewardPkt);
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

PlayerRef Room::SpawnPlayer(PlayerRef targetPlayer)
{
    // 룸 이동 뒤에 예약한 스폰 잡은 그사이 EnterPlayer가 접속 종료로 플레이어를 뺐어도 돈다.
    // 룸에 없는 플레이어를 알리면 다른 클라이언트에 유령이 남는다.
    if (Contains(targetPlayer->GetEntityId()) == false)
        return nullptr;

    // 본인은 빼고 알린다. 본인의 엔티티는 입장 응답에 실린다.
    Protocol::S_SPAWN spawnPkt;
    {
        Protocol::EntityInfo* entityInfo = spawnPkt.add_entities();
        entityInfo->CopyFrom(targetPlayer->GetEntityInfo());

        SendBufferRef sendBuffer = ServerPacketHandler::MakeSerializedPacket(spawnPkt);
        Broadcast(sendBuffer, targetPlayer->GetEntityId());
    }

    return targetPlayer;
}

void Room::ReplicateRoomData(PlayerRef player)
{
    int64 playerId = player->GetEntityId();

    Protocol::S_SPAWN spawnPkt;
    if (auto session = player->GetSession())
    {
        for (auto& item : _entities)
        {
            if (item.second->GetEntityId() == playerId)
                continue;

            // 플레이어의 장비 외형은 공개 정보의 gear_appearance에 실려 간다.
            spawnPkt.add_entities()->CopyFrom(item.second->GetEntityInfo());
        }

        SendPacket(session, spawnPkt);
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

    // 틱은 다음 룸 틱부터 받는다. 처음 들어온 엔티티면 JoinRoom이 Start를 부른다.
    entity->JoinRoom(GetRoomRef());

	return true;
}

bool Room::RemoveEntity(int64 entityId)
{
	if (_entities.contains(entityId) == false)
		return false;

    EntityRef entity = _entities[entityId];

    // 격자 밖에 있던 엔티티는 어느 셀에도 없다.
    _cellMatrix.Remove(entityId, ToPlanePos(entity->GetPosInfo()));

	_entities.erase(entityId);

	return true;
}

pair<PlayerRef, float> Room::FindClosestPlayer(const Protocol::PosInfo* posInfo, float range)
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

            float squareDist = MathUtil::Distance(posInfo, &player->GetPosInfo(), true);
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

void Room::UpdateCellMatrix()
{
    vector<pair<int64, vector2D>> positions;
    positions.reserve(_entities.size());

    for (auto& [entityId, entity] : _entities)
        positions.emplace_back(entityId, ToPlanePos(entity->GetPosInfo()));

    _cellMatrix.Update(positions);
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

		if (GameSessionRef session = player->GetSession())
			session->Send(sendBuffer);
	}
}
