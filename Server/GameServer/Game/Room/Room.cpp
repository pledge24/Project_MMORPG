#include "Core/pch.h"
#include "Game/Room/Room.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/Monster.h"
#include "Game/Entities/Npc.h"
#include "Game/Combat/Combat.h"
#include "Game/Room/MoveValidation.h"
#include "Game/Navigation/NavMeshes.h"
#include "Game/Navigation/NavQuery.h"
#include "Game/Room/RoomManager.h"
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
            if (optional<MapPosition> town = RoomTransfer::FindTownRespawn())
                player->ApplyTownRespawnForSave(town.value());
            else
                GLogger->Error("플레이어 {}에 마을 리스폰을 적용하지 못해 사망한 상태 그대로 저장합니다(지역 표에 마을 리스폰 지점이 없다)", player->GetEntityId());
        }

        return player->MakeSaveData();
    }

    // 서 있는 엔티티의 위치. 움직이지 않으므로 이동 방향이 없다.
    Protocol::PosInfo MakeStandingPos(const vector3D& pos, float yaw)
    {
        Protocol::PosInfo posInfo;
        posInfo.mutable_pos()->set_x(pos.x);
        posInfo.mutable_pos()->set_y(pos.y);
        posInfo.mutable_pos()->set_z(pos.z);
        posInfo.set_yaw(yaw);
        posInfo.set_state(Protocol::MOVE_STATE_IDLE);
        return posInfo;
    }

    SendBufferRef MakeDespawnPacket(int64 entityId)
    {
        Protocol::S_DESPAWN despawnPkt;
        despawnPkt.add_entity_ids(entityId);
        return ServerPacketHandler::MakeSerializedPacket(despawnPkt);
    }

    SendBufferRef MakeSpawnPacket(const EntityRef& entity)
    {
        Protocol::S_SPAWN spawnPkt;
        spawnPkt.add_entities()->CopyFrom(entity->GetEntityInfo());
        return ServerPacketHandler::MakeSerializedPacket(spawnPkt);
    }

    void SendToPlayer(const EntityRef& entity, const SendBufferRef& sendBuffer)
    {
        if (entity->IsPlayer() == false)
            return;

        if (GameSessionRef session = static_pointer_cast<Player>(entity)->GetSession())
            session->Send(sendBuffer);
    }
}

Room::~Room() = default;

RoomRef Room::Create(int32 roomId, int32 channel, const MapPlacement& map)
{
    RoomRef newRoom = make_shared<Room>();
    newRoom->Init(roomId, channel, map);
    return newRoom;
}

void Room::Init(int32 roomId, int32 channel, const MapPlacement& map)
{
    _roomId = roomId;
    _channel = channel;
    _map = map;

    // 지역은 룸 안에서 규칙이 다른 공간이다. 포털을 지역에서 찾는다.
    for (const auto& [areaId, area] : Gamedata::GetAreas())
    {
        if (area.MapId == map.mapId)
            _areas.push_back(area);
    }

    // 몬스터는 지역이 아니라 레벨에 배치한 스폰 마커에서 나온다.
    _monsterSpawner.Init(Gamedata::GetSpawnPoints(map.mapId));

    _cellMatrix.Init(_map.bounds, _map.cellSize);

    // 맵의 navmesh는 같은 맵의 룸들이 함께 읽고, 질의 상태만 룸마다 둔다. 테스트처럼 navmesh 없이 만든 룸은 지형을 보지 않는다.
    if (shared_ptr<const NavMesh> navMesh = NavMeshes::Find(map.mapId))
        _navQuery = make_unique<NavQuery>(std::move(navMesh));
}

bool Room::Start()
{
    // NPC 스폰. 배치 데이터대로 두고, 시야는 다른 엔티티처럼 셀을 따른다.
    if (SpawnNpcs() == false)
        return false;

    // 몬스터 스폰.
    if (SpawnInitialMonsters() == false)
        return false;

    _lastTickTime = GetTickCount64();
    DoTimer(ROOM_TICK_INTERVAL_MS, &Room::RunScheduledTick);

    return true;
}

bool Room::SpawnNpcs()
{
    for (const auto& [npcId, placement] : Gamedata::GetNpcPlacements())
    {
        if (placement.mapId != _map.mapId)
            continue;

        NpcSpawnParams spawnParams;
        spawnParams.npcId = npcId;
        spawnParams.spawnPos = MakeStandingPos(placement.pos, placement.yaw);

        if (SpawnEntity<Npc>(spawnParams) == nullptr)
        {
            GLogger->Error("룸 {}: NPC {}를 스폰하지 못했다", _roomId, npcId);
            return false;
        }
    }

    return true;
}

void Room::Tick(float deltaTime)
{
    // 몬스터가 플레이어를 찾을 때 셀을 본다. 엔티티 Tick보다 먼저 맞춰야 이번 틱의 탐색이 지난 틱의 위치를 본다.
    // 셀을 넘은 몬스터의 스폰과 디스폰도 여기서 나간다.
    RefreshVisibility();

    // 사망한 몬스터의 리젠. 룸에 들어간 몬스터는 이번 틱부터 돈다. 근처의 플레이어에게 스폰을 알린다.
    for (int32 spawnPointIndex : _monsterSpawner.Tick(deltaTime))
    {
        if (MonsterRef monster = SpawnMonster(spawnPointIndex))
            BroadcastNearby(monster->GetEntityId(), MakeSpawnPacket(monster), monster->GetEntityId());
        else
            _monsterSpawner.OnMonsterDied(spawnPointIndex); // 다시 예약하지 않으면 그 마커의 몬스터 수가 영영 줄어든다.
    }

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
    // 받는 플레이어마다 보는 몬스터가 다르므로 패킷을 플레이어마다 만든다. 셀은 이번 틱이 시작할 때의 것이다.
    // 그사이 셀을 넘은 몬스터는 다음 틱에 스폰이 먼저 나가고 그 뒤로 위치가 간다.
    for (auto& [playerId, entity] : _entities)
    {
        if (entity->IsPlayer() == false)
            continue;

        Protocol::S_MOVE movePkt;
        for (const EntityRef& nearby : FindNearbyEntities(playerId))
        {
            if (dynamic_pointer_cast<Monster>(nearby) == nullptr)
                continue;

            Protocol::EntityMove* move = movePkt.add_moves();
            move->set_entity_id(nearby->GetEntityId());
            move->mutable_pos_info()->CopyFrom(nearby->GetPosInfo());
        }

        if (movePkt.moves_size() == 0)
            continue;

        SendToPlayer(entity, ServerPacketHandler::MakeSerializedPacket(movePkt));
    }
}

bool Room::EnterPlayer(PlayerRef enterPlayer, RoomEnterData roomEnterData)
{
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
            Protocol::S_ENTER_ROOM enterRoomPkt;
            enterRoomPkt.set_result(Protocol::ENTRY_RESULT_INTERNAL_ERROR);
            SendPacket(session, enterRoomPkt);
        }

        return false;
    }

    enterPlayer->OnEnterRoom(static_pointer_cast<Room>(shared_from_this()), roomEnterData.enterPos);

    // 배치한 지점(포털 도착, 리스폰)의 높이는 바닥이다. 클라이언트는 받은 높이로 캡슐 중심을 맞추므로 바닥 위로 올린다.
    Protocol::PosInfo standingPos = enterPlayer->GetPosInfo();
    if (PlaceOnFloor(standingPos, PlayerHalfHeight()))
        enterPlayer->ApplyMove(standingPos, ::GetTickCount64());

    // AddEntity는 입장 전의 위치로 셀을 정했다. 들어간 위치의 셀로 옮겨 둬야 아래 알림이 맞는 이웃에게 간다.
    _cellMatrix.Place(enterPlayerId, _cellMatrix.ToCell(ToPlanePos(enterPlayer->GetPosInfo())));

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

    AnnounceEnter(enterPlayer);

    return true;
}

bool Room::LeavePlayer(PlayerRef leavePlayer, bool transferRoom)
{
    const int64 leavePlayerId = leavePlayer->GetEntityId();

    if (Contains(leavePlayerId) == false)
    {
        if (transferRoom)
            GLogger->Warning("플레이어 {}가 Room 이동 중 현재 Room {} 퇴장에 실패했습니다", leavePlayerId, _roomId);
        else
            GLogger->Warning("플레이어 {}가 Room {} 퇴장에 실패했습니다", leavePlayerId, _roomId);

        return false;
    }

    // 셀에서 빼기 전에 알린다. 빼고 나면 이 플레이어를 보던 쪽을 찾을 수 없다.
    BroadcastNearby(leavePlayerId, MakeDespawnPacket(leavePlayerId), leavePlayerId);

    // 룸을 옮기는 본인은 들어갈 룸의 입장 응답을 받으며 보던 엔티티를 모두 지우므로 디스폰을 받지 않는다.
    if (transferRoom == false)
    {
        if (auto session = leavePlayer->GetSession())
            session->Send(MakeDespawnPacket(leavePlayerId));
    }

    RemoveEntity(leavePlayerId);

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
    // 같은 룸이면 퇴장과 입장 없이 옮긴다. 같은 맵의 포털 이동과 리스폰은 채널을 바꾸지 않는다.
    if (destination.get() == this)
    {
        TeleportPlayer(player, enterPos);
        return;
    }

    LeavePlayer(player, true);

    // 맵이 바뀌면 클라이언트가 그 맵을 다 열 때까지 어느 룸에도 넣지 않는다. 로드 완료가 목적지 룸 큐에서 입장시킨다.
    if (destination->GetMapId() != GetMapId())
    {
        player->BeginMapLoad(destination, enterPos);
        return;
    }

    // 같은 맵의 다른 룸은 채널 변경이다. 클라이언트는 맵을 다시 열지 않는다.
    RoomEnterData enterData;
    enterData.nextRoomId = destination->GetRoomId();
    enterData.enterPos = enterPos;
    destination->DoAsync(&Room::EnterPlayer, player, enterData);
}

void Room::TeleportPlayer(const PlayerRef& player, const Protocol::PosInfo& pos)
{
    const int64 playerId = player->GetEntityId();

    // 리스폰은 플레이어의 위치를 그대로 넘기므로, 위치를 바꾸기 전에 사본을 뜬다.
    // 배치한 지점의 높이는 바닥이므로 캡슐 중심 높이로 올린다.
    Protocol::PosInfo destPos = pos;
    PlaceOnFloor(destPos, PlayerHalfHeight());

    // 떠나는 자리에서 보던 쪽에는 사라진다. 리스폰이면 사망한 모습이 사라지고 아래에서 되살아난 모습이 스폰된다.
    BroadcastNearby(playerId, MakeDespawnPacket(playerId), playerId);

    // 서버가 정한 위치이므로 이동 판정의 기준도 여기서 다시 잡는다.
    player->ApplyMove(destPos, ::GetTickCount64());
    _cellMatrix.Place(playerId, _cellMatrix.ToCell(ToPlanePos(destPos)));

    AnnounceEnter(player);
}

void Room::AnnounceEnter(const PlayerRef& player)
{
    const int64 playerId = player->GetEntityId();
    const vector<EntityRef> nearby = FindNearbyEntities(playerId);

    // 입장 알림. 내 엔티티는 입장 응답에만 싣고, 스폰은 다른 엔티티만 싣는다.
    // 클라이언트는 입장 응답을 받으면 보던 엔티티를 모두 지우므로, 같은 룸 안의 순간이동도 같은 순서로 보낸다.
    if (GameSessionRef session = player->GetSession())
    {
        Protocol::S_ENTER_ROOM enterRoomPkt;
        enterRoomPkt.set_result(Protocol::ENTRY_RESULT_SUCCESS);
        enterRoomPkt.mutable_my_entity()->CopyFrom(player->GetEntityInfo());
        enterRoomPkt.set_channel(_channel);
        SendPacket(session, enterRoomPkt);

        // 플레이어의 장비 외형은 공개 정보의 gear_appearance에 실려 간다.
        Protocol::S_SPAWN spawnPkt;
        for (const EntityRef& entity : nearby)
        {
            if (entity->GetEntityId() != playerId)
                spawnPkt.add_entities()->CopyFrom(entity->GetEntityInfo());
        }
        SendPacket(session, spawnPkt);
    }

    BroadcastNearby(playerId, MakeSpawnPacket(player), playerId);
}

RoomRef Room::SelectDestinationRoom(int32 mapId)
{
    // 같은 맵이면 채널을 지킨 채 이 룸 안에서 옮긴다.
    if (mapId == GetMapId())
        return GetRoomRef();

    return GRoomManager->SelectRoom(mapId);
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

    const DesignData::PortalRow* portal = FindPortal(pkt.portal_id());
    if (portal == nullptr)
    {
        reject(Protocol::ENTRY_RESULT_INVALID_PORTAL, format("현재 룸 {}의 맵 {}에 없는 포털 {}", _roomId, GetMapId(), pkt.portal_id()));
        return;
    }

    // 기동할 때 포털 표의 포털마다 배치 데이터가 있는지 대조했으므로 없으면 서버의 결함이다.
    const PortalPlacement* placement = Gamedata::FindPortalPlacement(portal->Id);
    if (placement == nullptr)
    {
        reject(Protocol::ENTRY_RESULT_INTERNAL_ERROR, format("포털 {}의 배치 데이터가 없다", portal->Id));
        return;
    }

    if (optional<string> rejection = RoomTransfer::ValidatePortalUse(*placement, player->GetPosInfo()))
    {
        reject(Protocol::ENTRY_RESULT_TOO_FAR_FROM_PORTAL, rejection.value());
        return;
    }

    const optional<MapPosition> dest = RoomTransfer::FindPortalDestination(*portal, *placement);
    if (dest.has_value() == false)
    {
        reject(Protocol::ENTRY_RESULT_INTERNAL_ERROR, format("포털 {}의 도착 지역 {}이 지역 표에 없다", portal->Id, portal->DestAreaId));
        return;
    }

    RoomRef destination = SelectDestinationRoom(dest->mapId);
    if (destination == nullptr)
    {
        reject(Protocol::ENTRY_RESULT_INTERNAL_ERROR, format("포털 {}의 도착 맵 {}에 룸이 없다", portal->Id, dest->mapId));
        return;
    }

    // 응답을 먼저 보낸다. 같은 맵이면 곧바로 S_ENTER_ROOM이 뒤따른다.
    usePortalPkt.set_result(Protocol::ENTRY_RESULT_SUCCESS);
    if (destination->GetMapId() != GetMapId())
        usePortalPkt.set_map_id(destination->GetMapId());
    SendPacket(session, usePortalPkt);

    MovePlayer(player, destination, dest->pos);
}

void Room::C_HandleMove(Protocol::C_MOVE pkt, PlayerRef player)
{
    // 대상은 패킷의 엔티티 번호가 아니라 보낸 사람이다. 그사이 룸을 떠났으면 버린다.
    const int64 playerId = player->GetEntityId();
    if (Contains(playerId) == false)
        return;

    // 직전에 받아들인 위치에서 흐른 시간으로 갈 수 있는 곳인지 본다. 어기면 버리고, 서버의 위치는 그대로 둔다.
    const uint64 now = ::GetTickCount64();
    const vector3D from = MathUtil::PosInfoToVector3D(player->GetPosInfo());
    const vector3D to = MathUtil::PosInfoToVector3D(pkt.pos_info());
    if (optional<string> rejection = MoveValidation::Validate(from, to, now - player->GetLastPositionTick(),
        _map.bounds, _navQuery.get()))
    {
        GLogger->Warning("플레이어 {}의 이동을 버린다: {}", playerId, rejection.value());
        return;
    }

    player->ApplyMove(pkt.pos_info(), now);

    // 셀을 넘었으면 새로 보게 된 쪽이 먼저 스폰을 받는다. 그다음 이동이 나가므로 스폰을 받은 쪽에도 이동이 간다.
    UpdateVisibility(player);

    Protocol::S_MOVE movePkt;
    Protocol::EntityMove* move = movePkt.add_moves();
    move->set_entity_id(playerId);
    *move->mutable_pos_info() = player->GetPosInfo();
    BroadcastNearby(playerId, ServerPacketHandler::MakeSerializedPacket(movePkt), playerId);
}

void Room::C_HandleNormalAttack(Protocol::C_NORMAL_ATTACK pkt, PlayerRef player)
{
    const int64 entityId = player->GetEntityId();
    if (Contains(entityId) == false)
        return;

    // 사망한 플레이어는 리스폰할 때까지 룸에 남으므로 여기서 거른다.
    const bool isDead = player->IsDead();

    Protocol::S_NORMAL_ATTACK normalAttackPkt;
    normalAttackPkt.set_result(isDead ? Protocol::COMBAT_RESULT_DEAD : Protocol::COMBAT_RESULT_SUCCESS);
    if (GameSessionRef session = player->GetSession())
        SendPacket(session, normalAttackPkt);

    if (isDead)
        return;

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

    optional<MapPosition> town = RoomTransfer::FindTownRespawn();
    RoomRef destination = town.has_value() ? SelectDestinationRoom(town->mapId) : nullptr;
    if (destination == nullptr)
    {
        reject(Protocol::ENTRY_RESULT_INTERNAL_ERROR);
        return;
    }

    // 부활 처리를 이 룸에서 먼저 한다. 목적지에서 다른 플레이어에게 죽은 상태가 나가지 않는다.
    RespawnResult result = player->ProcessRespawn(respawnType, town->pos);

    // 응답을 먼저 보낸다. 같은 맵이면 곧바로 S_ENTER_ROOM이 뒤따른다.
    respawnPkt.set_result(Protocol::ENTRY_RESULT_SUCCESS);
    if (destination->GetMapId() != GetMapId())
        respawnPkt.set_map_id(destination->GetMapId());
    *respawnPkt.mutable_changed_stats() = std::move(result.updatedStats);
    SendPacket(session, respawnPkt);

    // 같은 룸 안의 리스폰도 보던 쪽에는 사망한 모습의 디스폰을, 마을 주변에는 리스폰한 모습의 스폰을 보낸다.
    MovePlayer(player, destination, player->GetPosInfo());
}

void Room::C_HandleChannelList(PlayerRef player)
{
    GameSessionRef session = player->GetSession();
    if (session == nullptr)
        return;

    // 다른 채널의 인원은 그 룸 큐가 쓰는 값을 읽은 순간의 것이다. 목록을 보는 동안에도 바뀐다.
    Protocol::S_CHANNEL_LIST channelListPkt;
    for (const RoomRef& room : GRoomManager->GetChannelRooms(GetMapId()))
    {
        Protocol::ChannelInfo* channel = channelListPkt.add_channels();
        channel->set_channel(room->GetChannel());
        channel->set_player_count(room->GetPlayerCount());
    }

    SendPacket(session, channelListPkt);
}

void Room::C_HandleChangeChannel(Protocol::C_CHANGE_CHANNEL pkt, PlayerRef player)
{
    GameSessionRef session = player->GetSession();
    if (session == nullptr)
        return;

    // 잡을 넣은 뒤 다른 룸으로 옮겼으면 이 룸 큐는 그 플레이어의 상태를 읽지 않는다.
    if (player->GetRoom().get() != this)
        return;

    // 맵 로드를 기다리는 플레이어는 거절 응답을 받는다. 같은 맵의 다른 룸으로 옮기는 중이면 이 룸에 없으므로 버린다.
    const bool isLoadingMap = player->IsLoadingMap();
    if (isLoadingMap == false && Contains(player->GetEntityId()) == false)
        return;

    Protocol::S_CHANGE_CHANNEL changeChannelPkt;
    auto reject = [&](Protocol::EntryResult result)
        {
            GLogger->Warning("C_HandleChangeChannel: 플레이어 {}의 {}채널 이동을 거절한다. 결과 {}", player->GetEntityId(),
                pkt.channel(), Protocol::EntryResult_Name(result));
            changeChannelPkt.set_result(result);
            SendPacket(session, changeChannelPkt);
        };

    const int32 channelCount = static_cast<int32>(GRoomManager->GetChannelRooms(GetMapId()).size());
    if (optional<Protocol::EntryResult> rejection = RoomTransfer::ValidateChannelChange(player->IsDead(), isLoadingMap,
        _channel, pkt.channel(), channelCount))
    {
        reject(rejection.value());
        return;
    }

    RoomRef destination = GRoomManager->FindRoom(GetMapId(), pkt.channel());
    if (destination == nullptr)
    {
        reject(Protocol::ENTRY_RESULT_INTERNAL_ERROR);
        return;
    }

    // 응답을 먼저 보낸다. 들어갈 룸의 큐가 곧바로 S_ENTER_ROOM을 보낼 수 있다.
    changeChannelPkt.set_result(Protocol::ENTRY_RESULT_SUCCESS);
    changeChannelPkt.set_channel(pkt.channel());
    SendPacket(session, changeChannelPkt);

    // 채널 변경도 퇴장과 입장을 거치는 룸 이동이다. 다른 룸의 오브젝트는 그 룸 큐가 바꾼다.
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
    BroadcastNearby(attackerId, ServerPacketHandler::MakeSerializedPacket(attackPkt), attackerId);
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

            BroadcastNearby(target->GetEntityId(), ServerPacketHandler::MakeSerializedPacket(hitPkt));
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

    // 셀에서 빼기 전에 알린다.
    {
        Protocol::S_DIE diePkt;
        diePkt.set_entity_id(entityId);
        BroadcastNearby(entityId, ServerPacketHandler::MakeSerializedPacket(diePkt));
    }

    // 사망한 플레이어는 리스폰할 때까지 룸에 남는다. 리스폰이 이 룸에서 떠나는 것부터 시작하기 때문이다.
    if (creature->IsPlayer())
        return;

    // 사망한 몬스터는 그 몬스터가 나온 스폰 마커에서 리젠한다.
    if (MonsterRef monster = dynamic_pointer_cast<Monster>(creature))
        _monsterSpawner.OnMonsterDied(monster->GetSpawnPointIndex());

    RemoveEntity(entityId);
}

bool Room::SpawnInitialMonsters()
{
    for (int32 spawnPointIndex : _monsterSpawner.GetInitialSpawns())
    {
        if (SpawnMonster(spawnPointIndex) == nullptr)
            return false;
    }

    return true;
}

vector2D Room::GetRandomLocation(const vector2D& center, float radius)
{
    // 반지름을 그대로 뽑으면 가운데에 몰린다. 넓이가 반지름의 제곱에 비례하므로 제곱근을 씌운다.
    const float distance = radius * sqrtf(Utils::GetRandom(0.f, 1.f));
    const float angle = Utils::GetRandom(0.f, 2.f * std::numbers::pi_v<float>);
    return vector2D(center.x + distance * cosf(angle), center.y + distance * sinf(angle));
}

MonsterRef Room::SpawnMonster(int32 spawnPointIndex)
{
    const SpawnPointPlacement& spawnPoint = _monsterSpawner.GetSpawnPoint(spawnPointIndex);
    const int32 monsterTemplateId = _monsterSpawner.PickMonster(spawnPointIndex);

    MonsterSpawnParams spawnParams;
    spawnParams.templateId = monsterTemplateId;
    spawnParams.spawnPos = MakeRandomSpawnPos(spawnPoint, monsterTemplateId);
    spawnParams.spawnPoint = spawnPoint;
    spawnParams.spawnPointIndex = spawnPointIndex;

    MonsterRef monster = SpawnEntity<Monster>(spawnParams);
    if (monster == nullptr)
        GLogger->Error("룸 {}: 스폰 그룹 {}의 마커에 몬스터 {}를 스폰하지 못했다", _roomId, spawnPoint.spawnGroupId, monsterTemplateId);

    return monster;
}

Protocol::PosInfo Room::MakeRandomSpawnPos(const SpawnPointPlacement& spawnPoint, int32 monsterTemplateId)
{
    // 클라이언트가 받은 높이로 캡슐 중심을 맞추므로, 그 몬스터의 캡슐이 바닥에 서는 높이로 둔다.
    const CreatureCombat* combat = Gamedata::FindMonsterCombat(monsterTemplateId);
    const float halfHeight = combat != nullptr ? combat->capsuleHalfHeight : 0.f;

    Protocol::PosInfo posInfo;
    posInfo.set_yaw(GetRandomYaw());
    Protocol::Vector* pos = posInfo.mutable_pos();

    // 반경 안에 장애물이 있으면 뽑은 곳에 바닥이 없을 수 있다. 몇 번 다시 뽑고, 그래도 없으면 마커 위치에 둔다.
    // 마커 위치가 navmesh 위에 있는지는 navmesh 빌드가 검사했다.
    for (int32 attempt = 0; attempt < SPAWN_POS_ATTEMPTS; ++attempt)
    {
        const vector2D randomPos = GetRandomLocation(vector2D(spawnPoint.pos.x, spawnPoint.pos.y), spawnPoint.radius);
        pos->set_x(randomPos.x);
        pos->set_y(randomPos.y);
        pos->set_z(spawnPoint.pos.z + SPAWN_HEIGHT_PADDING);
        if (_navQuery == nullptr)
            return posInfo;

        if (const optional<float> floor = _navQuery->FindFloorHeight(MathUtil::PosInfoToVector3D(posInfo)))
        {
            pos->set_z(floor.value() + halfHeight);
            return posInfo;
        }
    }

    pos->set_x(spawnPoint.pos.x);
    pos->set_y(spawnPoint.pos.y);
    pos->set_z(spawnPoint.pos.z + SPAWN_HEIGHT_PADDING);
    PlaceOnFloor(posInfo, halfHeight);
    return posInfo;
}

const DesignData::PortalRow* Room::FindPortal(int32 portalId) const
{
    for (const DesignData::AreaRow& area : _areas)
    {
        for (const DesignData::PortalRow& portal : area.Portals)
        {
            if (portal.Id == portalId)
                return &portal;
        }
    }

    return nullptr;
}

vector3D Room::GetCenterPoint() const
{
    const PlaneBounds& bounds = _map.bounds;
    return vector3D((bounds.minX + bounds.maxX) * 0.5f, (bounds.minY + bounds.maxY) * 0.5f, 0.f);
}

bool Room::PlaceOnFloor(Protocol::PosInfo& posInfo, float halfHeight)
{
    if (_navQuery == nullptr)
        return false;

    const vector3D pos = MathUtil::PosInfoToVector3D(posInfo);
    const optional<float> floor = _navQuery->FindFloorHeight(pos);
    if (floor.has_value() == false)
    {
        GLogger->Warning("룸 {}: ({}, {}, {}) 아래에 걸을 수 있는 바닥이 없어 높이를 그대로 둔다", _roomId, pos.x, pos.y, pos.z);
        return false;
    }

    posInfo.mutable_pos()->set_z(floor.value() + halfHeight);
    return true;
}

float Room::PlayerHalfHeight() const
{
    return _navQuery != nullptr ? _navQuery->GetAgentHalfHeight() : 0.f;
}

NpcRef Room::FindNpc(int64 entityId)
{
    return FindEntityAs<Npc>(entityId);
}

bool Room::AddEntity(EntityRef entity)
{
    if (entity == nullptr)
        return false;

    int64 entityId = entity->GetEntityId();
	if (_entities.contains(entityId))
		return false;

	_entities.insert(make_pair(entityId, entity));
    _cellMatrix.Place(entityId, _cellMatrix.ToCell(ToPlanePos(entity->GetPosInfo())));
    if (entity->IsPlayer())
        _playerCount++;

    // 틱은 다음 룸 틱부터 받는다. 처음 들어온 엔티티면 JoinRoom이 Start를 부른다.
    entity->JoinRoom(GetRoomRef());

	return true;
}

bool Room::RemoveEntity(int64 entityId)
{
    auto it = _entities.find(entityId);
	if (it == _entities.end())
		return false;

    if (it->second->IsPlayer())
        _playerCount--;

    _cellMatrix.Remove(entityId);
	_entities.erase(it);

	return true;
}

void Room::UpdateVisibility(const EntityRef& entity)
{
    const int64 entityId = entity->GetEntityId();
    const optional<CellIndex> oldCell = _cellMatrix.FindCell(entityId);
    if (oldCell.has_value() == false)
        return;

    const CellIndex newCell = _cellMatrix.ToCell(ToPlanePos(entity->GetPosInfo()));
    if (oldCell.value() == newCell)
        return;

    // 다른 엔티티의 셀은 지금 기록된 셀로 본다. 여러 엔티티가 한 틱에 셀을 넘어도 하나씩 옮기며 알리므로,
    // 각 클라이언트가 아는 엔티티는 언제나 「기록된 셀끼리 이웃한 엔티티」와 같다.
    vector<EntityRef> hidden;
    for (int64 otherId : _cellMatrix.QueryNeighbors(oldCell.value()))
    {
        const optional<CellIndex> otherCell = _cellMatrix.FindCell(otherId);
        if (otherId != entityId && CellMatrix::IsNeighbor(otherCell.value(), newCell) == false)
            hidden.push_back(_entities.at(otherId));
    }

    vector<EntityRef> revealed;
    for (int64 otherId : _cellMatrix.QueryNeighbors(newCell))
    {
        const optional<CellIndex> otherCell = _cellMatrix.FindCell(otherId);
        if (otherId != entityId && CellMatrix::IsNeighbor(otherCell.value(), oldCell.value()) == false)
            revealed.push_back(_entities.at(otherId));
    }

    _cellMatrix.Place(entityId, newCell);

    if (hidden.empty() == false)
    {
        const SendBufferRef despawn = MakeDespawnPacket(entityId);
        Protocol::S_DESPAWN hiddenPkt;
        for (const EntityRef& other : hidden)
        {
            SendToPlayer(other, despawn);
            hiddenPkt.add_entity_ids(other->GetEntityId());
        }

        SendToPlayer(entity, ServerPacketHandler::MakeSerializedPacket(hiddenPkt));
    }

    if (revealed.empty() == false)
    {
        const SendBufferRef spawn = MakeSpawnPacket(entity);
        Protocol::S_SPAWN revealedPkt;
        for (const EntityRef& other : revealed)
        {
            SendToPlayer(other, spawn);
            revealedPkt.add_entities()->CopyFrom(other->GetEntityInfo());
        }

        SendToPlayer(entity, ServerPacketHandler::MakeSerializedPacket(revealedPkt));
    }
}

void Room::BroadcastNearby(int64 sourceId, SendBufferRef sendBuffer, int64 exceptId)
{
    for (const EntityRef& entity : FindNearbyEntities(sourceId))
    {
        if (entity->GetEntityId() == exceptId)
            continue;

        SendToPlayer(entity, sendBuffer);
    }
}

void Room::Broadcast(SendBufferRef sendBuffer, int64 exceptId)
{
    for (auto& [entityId, entity] : _entities)
    {
        if (entityId != exceptId)
            SendToPlayer(entity, sendBuffer);
    }
}

void Room::RefreshVisibility()
{
    // 알림이 엔티티를 빼지는 않지만, 순회 중인 맵을 건드리지 않도록 목록을 떠 둔다.
    vector<EntityRef> entities;
    entities.reserve(_entities.size());
    for (auto& [entityId, entity] : _entities)
        entities.push_back(entity);

    for (const EntityRef& entity : entities)
        UpdateVisibility(entity);
}

vector<EntityRef> Room::FindNearbyEntities(int64 entityId) const
{
    vector<EntityRef> nearby;

    const optional<CellIndex> cell = _cellMatrix.FindCell(entityId);
    if (cell.has_value() == false)
        return nearby;

    for (int64 nearbyId : _cellMatrix.QueryNeighbors(cell.value()))
    {
        auto it = _entities.find(nearbyId);
        if (it != _entities.end())
            nearby.push_back(it->second);
    }

    return nearby;
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
