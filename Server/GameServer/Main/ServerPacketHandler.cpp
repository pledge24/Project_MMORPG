#include "pch.h"
#include "ServerPacketHandler.h"
#include "Protocol.pb.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "Player.h"
#include "Room.h"
#include "EntityUtils.h"
#include "Inventory.h"
#include "EquippedGear.h"
#include "Gamedata.h"
#include "CharacterListDAO.h"
#include "ProgressStorage.h"
#include "CharacterCreation.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO: Log
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

namespace
{
    // 사유를 알린 뒤 곧바로 끊는다. 다른 송신이 진행 중이면 사유 패킷은 송신 큐에서 기다리다 버려질 수 있다.
    // 그때 클라이언트는 사유 없이 끊긴 것으로 보고 일반 문구를 띄운다.
    void KickSession(const GameSessionRef& target, Protocol::LeaveReason reason, const char* cause)
    {
        Protocol::S_LEAVE_GAME leavePkt;
        leavePkt.set_reason(reason);
        SEND_PACKET_USING_THIS_SESSION(target, leavePkt)

        target->Disconnect(cause);
    }
}

bool Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt)
{
    // 랜덤으로 아무 DBQueue에게 Job을 준다.
    int32 dbQueueCount = GDBManager->GetDBQueueCount();
    DBQueueRef dbQueue = GDBManager->GetDBQueue(Utils::GetRandom(0, dbQueueCount - 1));

    JobRef job = make_shared<Job>(
        [session, pkt]()
        {
            GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

            // 액세스 토큰은 한 번만 쓴다. 같은 토큰의 로그인이 다른 DB 큐에서 동시에 와도
            // 키를 지운 쪽 하나만 통과한다.
            const string tokenKey = "accessToken:" + pkt.access_token();
            RedisRef redis = GRedisManager->GetRedis();
            auto val = redis->get(tokenKey);
            if (val.has_value() == false || redis->del(tokenKey) == 0)
            {
                wcout << L"액세스 토큰이 없거나 이미 쓰였다" << '\n';
                KickSession(gameSession, Protocol::LEAVE_REASON_INVALID_TOKEN, "Invalid Access Token");
                return;
            }

            Json json = Json::parse(*val);
            string username = json["username"];
            int64 userId = json["userId"];

            cout << "userId: " << userId << endl;
            cout << "username: " << username << endl;

            // 한 계정은 세션 하나만 가진다. 나중에 온 로그인이 이기고 기존 세션은 끊긴다.
            // 기존 세션의 룸 퇴장과 저장은 접속 종료 경로(GameSession::OnDisconnected)가 한다.
            if (GameSessionRef replaced = GSessionManager.RegisterUser(userId, gameSession))
                KickSession(replaced, Protocol::LEAVE_REASON_DUPLICATE_LOGIN, "Duplicate Login");

            // 등록하기 전에 이 세션이 이미 끊겼다면 접속 종료의 Remove가 먼저 지나갔다. 등록을 여기서 거둔다.
            if (gameSession->IsConnected() == false)
            {
                GSessionManager.Remove(gameSession);
                return;
            }

            CharacterListDAO::LoadCharacterList(session, userId);
        }
    );

    dbQueue->Push(std::move(job));

	return true;
}

bool Handle_C_CREATE_CHARACTER(PacketSessionRef& session, Protocol::C_CREATE_CHARACTER& pkt)
{
    // 거절하면 DB 큐로 넘기지 않고 사유를 곧바로 돌려준다. 클라이언트가 사유를 생성 화면에 띄운다.
    if (optional<string> cause = CharacterCreation::Validate(pkt.character()))
    {
        Protocol::S_CREATE_CHARACTER createCharacterPkt;
        createCharacterPkt.set_success(false);
        createCharacterPkt.set_cause(cause.value());
        SEND_PACKET(createCharacterPkt)
        return true;
    }

    // 유저 Id를 통해 DBQueue를 선택
    int64 userId = static_pointer_cast<GameSession>(session)->_userId;
    DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(userId);

    JobRef job = make_shared<Job>(
        [session, pkt, userId]()
        {
            const Protocol::CharacterOverview& character = pkt.character();
            CharacterListDAO::CreateCharacter(session, character, userId);
        }
    );

    dbQueue->Push(std::move(job));

    return true;
}

bool Handle_C_DELETE_CHARACTER(PacketSessionRef& session, Protocol::C_DELETE_CHARACTER& pkt)
{
    // 소유 확인은 DeleteCharacter의 SQL이 user_id를 함께 대조해서 한다.

    // 유저 Id를 통해 DBQueue를 선택
    int64 userId = static_pointer_cast<GameSession>(session)->_userId;
    DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(userId);

    JobRef job = make_shared<Job>(
        [session, pkt]()
        {
            int64 characterId = pkt.character_id();
            CharacterListDAO::DeleteCharacter(session, characterId);
        }
    );

    dbQueue->Push(std::move(job));

    return true;
}

bool Handle_C_ENTER_GAME(PacketSessionRef& session, Protocol::C_ENTER_GAME& pkt)
{
    // 유저 Id를 통해 DBQueue를 선택
    int64 userId = static_pointer_cast<GameSession>(session)->_userId;
    DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(userId);

    // 플레이어 생성은 잡 안에서 한다. C_ENTER_GAME은 character_id만 싣고 오고
    // room_id는 LoadAllCharactersData가 DB에서 읽어야 알 수 있으므로,
    // 이 시점에 넘길 룸 큐가 없다. 아키텍처가 게임 입장에 지정한 경로가 DBQueue이고
    // 생성 직후의 소비자도 같은 잡이라 여기로 모은다.
    JobRef job = make_shared<Job>(
        [session, pkt]()
        {
            // 플레이어 생성 및 초기화
            PlayerRef player = EntityUtils::CreatePlayer(static_pointer_cast<GameSession>(session));
            if (player == nullptr)
            {
                wcout << L"Warning: 플레이어 생성 실패" << '\n';
                return;
            }

            int64 characterId = pkt.character_id();
            ProgressStorage::Load(session, characterId);
        }
    );

    dbQueue->Push(std::move(job));

	return true;
}

bool Handle_C_LEAVE_GAME(PacketSessionRef& session, Protocol::C_LEAVE_GAME& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    // 룸 퇴장과 저장은 GameSession::OnDisconnected가 한다. 비정상 종료와 같은 경로를 탄다.
    gameSession->Disconnect("Exit Game");

    return true;
}

bool Handle_C_ENTER_MAP(PacketSessionRef& session, Protocol::C_ENTER_MAP& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->_player.load();
    int32 roomId = pkt.room_id();

    if (player == nullptr)
    {
        // 플레이어가 없으면 잡을 올릴 룸 큐도 없다. 룸 소유 상태를 건드리지 않는
        // 실패 응답만 이 자리에서 돌려준다.
        Protocol::S_ENTER_MAP enterMapPkt;
        {
            enterMapPkt.set_success(false);
            enterMapPkt.set_map_id(pkt.map_id());
            enterMapPkt.set_room_id(roomId);

            SEND_PACKET(enterMapPkt)
        }

        return false;
    }

    // 플레이어 상태를 소유한 룸의 큐로 넘긴다. 아직 어떤 룸에도 속하지 않았다면
    // OnEnterMap이 세팅한 enteringRoomId를 뒤이어 읽게 될 목적지 룸의 큐로 넘긴다.
    RoomRef room = player->_room.load().lock();
    if (room == nullptr)
        room = GRoomManager->GetRoomRefFromRoomId(roomId);

    if (room == nullptr)
    {
        // 넘길 큐가 없으면 잡을 만들 수 없다. 이전 코드가 이 입력에도 응답을 돌려줬으므로
        // 클라를 대기 상태로 남기지 않도록 실패 응답은 유지한다.
        wcout << L"C_ENTER_MAP을 넘길 Room을 찾지 못함. roomId: " << roomId << '\n';

        Protocol::S_ENTER_MAP enterMapPkt;
        {
            enterMapPkt.set_success(false);
            enterMapPkt.set_map_id(pkt.map_id());
            enterMapPkt.set_room_id(roomId);

            SEND_PACKET(enterMapPkt)
        }

        return false;
    }

    room->DoAsync(&Room::C_HandleEnterMap, pkt, player);

    return true;
}

bool Handle_C_ENTER_ROOM(PacketSessionRef& session, Protocol::C_ENTER_ROOM& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->_player.load();
    if (player == nullptr)
        return false;

    RoomRef curRoom = player->_room.load().lock();
    if (curRoom == nullptr)
    {
        // 아직 어떤 Room에도 속하지 않은 최초 입장.
        // player->_room 은 Room::EnterPlayer 안에서만 세팅되므로 여기서는 항상 비어 있다.
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

	PlayerRef player = gameSession->_player.load();
	if (player == nullptr)
		return false;

	RoomRef room = player->_room.load().lock();
	if (room == nullptr)
		return false;

    room->DoAsync(&Room::C_HandleMove, pkt);

	return true;
}

bool Handle_C_CHAT(PacketSessionRef& session, Protocol::C_CHAT& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	PlayerRef player = gameSession->_player.load();
	if (player == nullptr)
		return false;

	RoomRef room = player->_room.load().lock();
	if (room == nullptr)
		return false;

	room->DoAsync(&Room::C_HandleChat, pkt, player);

	return true;
}

bool Handle_C_NORMAL_ATTACK(PacketSessionRef& session, Protocol::C_NORMAL_ATTACK& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->_player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->_room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync(&Room::C_HandleNormalAttack, pkt, player);

    return true;
}

bool Handle_C_BUY_ITEM(PacketSessionRef& session, Protocol::C_BUY_ITEM& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->_player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->_room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync(&Room::C_HandleBuyItem, pkt, player);

    return true;
}

bool Handle_C_SELL_ITEM(PacketSessionRef& session, Protocol::C_SELL_ITEM& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->_player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->_room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync(&Room::C_HandleSellItem, pkt, player);

    return true;
}

bool Handle_C_EQUIP_GEAR(PacketSessionRef& session, Protocol::C_EQUIP_GEAR& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->_player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->_room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync(&Room::C_HandleEquipGear, pkt, player);

    return true;
}


bool Handle_C_UNEQUIP_GEAR(PacketSessionRef& session, Protocol::C_UNEQUIP_GEAR& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->_player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->_room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync(&Room::C_HandleUnequipGear, pkt, player);

    return true;
}

bool Handle_C_USE_ITEM(PacketSessionRef& session, Protocol::C_USE_ITEM& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->_player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->_room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync(&Room::C_HandleUseItem, pkt, player);

    return true;
}

bool Handle_C_RESPAWN(PacketSessionRef& session, Protocol::C_RESPAWN& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->_player.load();
    if (player == nullptr)
        return false;

    RoomRef room = player->_room.load().lock();
    if (room == nullptr)
        return false;

    room->DoAsync(&Room::C_HandleRespawn, pkt, player);

    return true;
}

