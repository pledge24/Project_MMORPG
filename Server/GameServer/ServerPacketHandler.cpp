#include "pch.h"
#include "ServerPacketHandler.h"
#include "Protocol.pb.h"
#include "GameSession.h"
#include "Player.h"
#include "Room.h"
#include "ObjectUtils.h"
#include "Inventory.h"
#include "EquippedGear.h"
#include "Gamedata.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO : Log
	return false;
}

bool Handle_C_PING(PacketSessionRef& session, Protocol::C_PING& pkt)
{
	return false;
}

bool Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt)
{
    // TODO : 해당 패킷이 유효한지 검증(Validate)
    // ...

    // 랜덤으로 아무 DBQueue에게 Job을 준다.
    int32 dbQueueCount = GDBManager->GetDBQueueCount();
    DBQueueRef dbQueue = GDBManager->GetDBQueue(Utils::GetRandom(0, dbQueueCount));

    JobRef job = make_shared<Job>(
        [session, pkt]()
        {
            // 클라로부터 받은 AccessToken을 Redis와 비교
            string accessToken = pkt.access_token();

            RedisRef redis = GRedisManager->GetRedis();
            auto val = redis->get("accessToken:" + accessToken);
            if (val)
            {
                Json json = Json::parse(*val);
                string username = json["username"];
                int64 userId = json["userId"];

                cout << "userId: " << userId << endl;
                cout << "username: " << username << endl;
                
                // 게임 세션에 userId 저장.
                GameSessionRef gameSession = static_pointer_cast<GameSession>(session);
                gameSession->userId = userId;

                // 있으면 DB에서 캐릭터 정보를 긁어온다.
                DBRequestFunctions::LoadUserCharactersData(session, userId);
            }
            else
            {
                cout << "Not Found AccessToken" << endl;
            }
        }
    );

    dbQueue->Push(std::move(job));

	return true;
}

bool Handle_C_CREATE_CHARACTER(PacketSessionRef& session, Protocol::C_CREATE_CHARACTER& pkt)
{
    // TODO : 해당 패킷이 유효한지 검증(Validate)
    // ...

    // 유저 Id를 통해 DBQueue를 선택
    int64 userId = static_pointer_cast<GameSession>(session)->userId;
    DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(userId);

    JobRef job = make_shared<Job>(
        [session, pkt, userId]()
        {
            const Protocol::CharacterOverview& character = pkt.character();
            DBRequestFunctions::CreateCharacter(session, character, userId);
        }
    );

    dbQueue->Push(std::move(job));

    return true;
}

bool Handle_C_DELETE_CHARACTER(PacketSessionRef& session, Protocol::C_DELETE_CHARACTER& pkt)
{
    // TODO : 해당 패킷이 유효한지 검증(Validate)
    // ...

    // 유저 Id를 통해 DBQueue를 선택
    int64 userId = static_pointer_cast<GameSession>(session)->userId;
    DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(userId);

    JobRef job = make_shared<Job>(
        [session, pkt]()
        {
            int64 characterId = pkt.character_id();
            DBRequestFunctions::DeleteCharacter(session, characterId);
        }
    );

    dbQueue->Push(std::move(job));

    return true;
}

bool Handle_C_ENTER_GAME(PacketSessionRef& session, Protocol::C_ENTER_GAME& pkt)
{
	// 플레이어 생성 및 초기화
	PlayerRef player = ObjectUtils::CreatePlayer(static_pointer_cast<GameSession>(session));
    player->Init();

    // 유저 Id를 통해 DBQueue를 선택
    int64 userId = static_pointer_cast<GameSession>(session)->userId;
    DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(userId);

    JobRef job = make_shared<Job>(
        [session, pkt]()
        {
            int64 characterId = pkt.character_id();
            DBRequestFunctions::LoadAllCharactersData(session, characterId);
        }
    );

    dbQueue->Push(std::move(job));

	return true;
}

bool Handle_C_LEAVE_GAME(PacketSessionRef& session, Protocol::C_LEAVE_GAME& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->room.load().lock();
    if (room == nullptr)
        return false;

    // Room 퇴장 처리
    room->DoAsync(&Room::LeavePlayer, player, false);


    // DB 업데이트 처리
    {
        int64 characterId = player->playerInfo->character_id();
        DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(characterId);

        // 게임 종료 플레이어 정보 DB에 저장.
        JobRef job = make_shared<Job>(
            [session, player]()
            {
                DBRequestFunctions::UpdateAllCharactersData(session);
            }
        );
        dbQueue->Push(std::move(job));
    }

    // GameSession 네트워크 연결 해제
    gameSession->Disconnect("Exit Game");

    return true;
}

bool Handle_C_ENTER_MAP(PacketSessionRef& session, Protocol::C_ENTER_MAP& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    int32 roomId = pkt.room_id();

    // TODO: 나중에 레벨 이동이 생기면 검증 코드 추가
    // ...

    if (player == nullptr)
    {
        Protocol::S_ENTER_MAP enterMapPkt;
        {
            enterMapPkt.set_success(false);
            enterMapPkt.set_map_id(pkt.map_id());
            enterMapPkt.set_room_id(roomId);

            SEND_PACKET(enterMapPkt);
        }

        return false;
    }

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

            SEND_PACKET(enterMapPkt);
        }
    }

    return true;
}

bool Handle_C_ENTER_ROOM(PacketSessionRef& session, Protocol::C_ENTER_ROOM& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

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

    return true;
}

bool Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	PlayerRef player = gameSession->player.load();
	if (player == nullptr)
		return false;

	RoomRef room = player->room.load().lock();
	if (room == nullptr)
		return false;

    room->DoAsync(&Room::C_HandleMove, pkt);

	return true;
}

bool Handle_C_NORMAL_ATTACK(PacketSessionRef& session, Protocol::C_NORMAL_ATTACK& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync(&Room::C_HandleNormalAttack, pkt, player);

    return true;
}

bool Handle_C_BUY_ITEM(PacketSessionRef& session, Protocol::C_BUY_ITEM& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync([self = room, pkt, player]()
        {
            self->C_HandleBuyItem(pkt, player);
        });

    return true;
}

bool Handle_C_SELL_ITEM(PacketSessionRef& session, Protocol::C_SELL_ITEM& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync([self = room, pkt, player]()
        {
            self->C_HandleSellItem(pkt, player);
        });
    
    return true;
}

bool Handle_C_EQUIP_GEAR(PacketSessionRef& session, Protocol::C_EQUIP_GEAR& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync(&Room::C_HandleEquipGear, pkt, player);

    return true;
}


bool Handle_C_UNEQUIP_GEAR(PacketSessionRef& session, Protocol::C_UNEQUIP_GEAR& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync(&Room::C_HandleUnequipGear, pkt, player);

    return true;
}

bool Handle_C_USE_ITEM(PacketSessionRef& session, Protocol::C_USE_ITEM& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync([self = room, pkt, player]()
        {
            self->C_HandleUseItem(pkt, player);
        });

    return true;
}

bool Handle_C_RESPAWN(PacketSessionRef& session, Protocol::C_RESPAWN& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    RoomRef curRoom = player->room.load().lock();
    if (curRoom == nullptr)
        return false;

    // Find Respawn Room
    RoomRef respawnRoom = nullptr;
    shared_ptr<Protocol::PosInfo> respawnPos = make_shared<Protocol::PosInfo>();
    Protocol::RespawnType respawnType = pkt.respawn_type();

    switch (respawnType)
    {
    case Protocol::RESPAWN_TYPE_TOWN:
    case Protocol::RESPAWN_TYPE_CHECKPOINT:
    case Protocol::RESPAWN_TYPE_IN_PLACE:
    case Protocol::RESPAWN_TYPE_GUILD_BASE:
    {
        int32 roomId = player->GetRespawnRoomId(respawnType);
        respawnRoom = GRoomManager->GetRoomRefFromRoomId(roomId);
        respawnPos = respawnRoom->GetRespawnPoint();
        break;
    }
    case Protocol::RESPAWN_TYPE_RESURRECTION_ITEM:
    case Protocol::RESPAWN_TYPE_CASH_ITEM:
    {
        // 아이템 사용
        break;
    }
    case Protocol::RESPAWN_TYPE_PARTY_MEMBER:
    case Protocol::RESPAWN_TYPE_BATTLE_RESURRECTION:
    {
        // objectId가 존재하는 경우
        break;
    }

    }

    if (respawnRoom == nullptr)
    {
        wcout << L"리스폰할 룸을 찾지 못함" << '\n';
        return false;
    }

    if (curRoom == respawnRoom)
    {
        curRoom->DoAsync(&Room::C_HandleRespawn, pkt, player, respawnPos);
    }
    else
    {
        // 죽은 Room과 다른 Room에서 Respawn하는 경우 진입
        using namespace JsonProperty::Map;

        RoomEnterData enterData = RoomEnterData{};
        // 1) Room 이동 데이터 설정
        {
            Protocol::PosInfo enterPos;
            enterPos.CopyFrom(*respawnPos);
            enterPos.set_object_id(player->objectInfo->object_id());
            enterData.enterPos->Swap(&enterPos);
        }

        // 2) Room을 이동 -> 리스폰 패킷 전송 -> Room 정보 전송
        curRoom->DoAsync([self = curRoom, respawnRoom, pkt, player, enterData, respawnPos]()
            {
                if (self->TransferPlayer(player, enterData) == false)
                    return;

                respawnRoom->DoAsync([self = respawnRoom, pkt, player, respawnPos]()
                    {
                        if (self->C_HandleRespawn(pkt, player, respawnPos) == false)
                            return;

                        self->ReplicateRoomData(player, false);
                    });
            });

    }

    return true;
}

