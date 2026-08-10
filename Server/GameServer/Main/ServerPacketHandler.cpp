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

// TODO: 클라이언트 맵 로드 완료 처리. Protocol.proto에 선언만 되어 있어
// 생성기가 만드는 핸들러 선언을 채우기 위한 스텁이다.
bool Handle_C_MAP_LOAD_COMPLETE(PacketSessionRef& session, Protocol::C_MAP_LOAD_COMPLETE& pkt)
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
    if (player == nullptr)
    {
        wcout << L"Warning: 플레이어 생성 실패" << '\n';
        return false;
    }

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

    RoomRef curRoom = player->room.load().lock();
    if (curRoom == nullptr)
    {
        // 아직 어떤 Room에도 속하지 않은 최초 입장.
        // player->room 은 Room::EnterPlayer 안에서만 세팅되므로 여기서는 항상 비어 있다.
        // 클라이언트가 무엇을 보냈든 서버가 INITIAL로 판정하고, 입장할 Room의 큐로 넘긴다.
        pkt.set_enter_type(Protocol::ENTER_TYPE_INITIAL);

        int32 roomId = pkt.has_room_id() ? pkt.room_id() : player->GetEnteringRoomId();
        RoomRef enterRoom = GRoomManager->GetRoomRefFromRoomId(roomId);
        if (enterRoom == nullptr)
        {
            wcout << L"최초 입장할 Room을 찾지 못함. roomId: " << roomId << '\n';
            return false;
        }

        enterRoom->DoAsync(&Room::C_HandleEnterRoom, pkt, player);

        return true;
    }

    curRoom->DoAsync(&Room::C_HandleEnterRoom, pkt, player);

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

    room->DoAsync(&Room::C_HandleBuyItem, pkt, player);

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

    room->DoAsync(&Room::C_HandleSellItem, pkt, player);

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

    room->DoAsync(&Room::C_HandleUseItem, pkt, player);

    return true;
}

bool Handle_C_RESPAWN(PacketSessionRef& session, Protocol::C_RESPAWN& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync(&Room::C_HandleRespawn, pkt, player);

    return true;
}

