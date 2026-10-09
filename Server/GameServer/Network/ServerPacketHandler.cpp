#include "Core/pch.h"
#include "Network/ServerPacketHandler.h"
#include "Network/GameSessionManager.h"
#include "Network/ProgressCoordinator.h"
#include "Game/Entities/Player.h"
#include "Game/Room/Room.h"
#include "Network/GameEntry.h"
#include "Network/ItemRequests.h"
#include "DB/CharacterListDAO.h"
#include "DB/DAOCommon.h"
#include "Game/Characters/CharacterCreation.h"

namespace
{
    // 사유 패킷을 보낼 때까지 끊기를 미루는 상한이다. 넘기면 상대가 받지 않는 것으로 보고 끊는다.
    constexpr uint64 KICK_SEND_TIMEOUT_MS = 1000;

    // 사유를 알리고, 그 패킷을 보낸 뒤에 끊는다. 클라이언트는 사유를 받아야 밀려난 이유를 화면에 띄운다.
    void KickSession(const GameSessionRef& target, Protocol::LeaveReason reason, const char* cause)
    {
        Protocol::S_LEAVE_GAME leavePkt;
        leavePkt.set_reason(reason);
        SendPacket(target, leavePkt);

        target->DisconnectAfterSend(cause);

        // 이미 끊겼으면 Disconnect는 아무것도 하지 않는다.
        GSessionJobQueue->DoTimer(KICK_SEND_TIMEOUT_MS, [target, cause]()
            {
                target->Disconnect(cause);
            });
    }

    // 보낸 세션의 플레이어가 속한 룸의 큐에 job(room, player)을 넣는다. 플레이어나 룸이 없으면 넣지 않고 false.
    // 보낸 사람은 패킷 안의 id가 아니라 세션의 플레이어로 정한다.
    template<typename RoomJob>
    bool DispatchToPlayerRoom(const PacketSessionRef& session, RoomJob&& job)
    {
        PlayerRef player = static_pointer_cast<GameSession>(session)->GetPlayer();
        if (player == nullptr)
            return false;

        RoomRef room = player->GetRoom();
        if (room == nullptr)
            return false;

        room->DoAsync([room, player, job = std::forward<RoomJob>(job)]()
            {
                job(room, player);
            });

        return true;
    }

    void SendEnterGameFail(const PacketSessionRef& session)
    {
        Protocol::S_ENTER_GAME enterGameFailPkt;
        enterGameFailPkt.set_success(false);
        SendPacket(session, enterGameFailPkt);
    }
}

// 핸들러는 모두 IOCP 워커 스레드에서 불린다. C_ 패킷의 값은 클라이언트가 보낸 것이므로 전부 검증 대상이다.
// 룸 소유 상태는 여기서 건드리지 않는다. 룸 일은 room->DoAsync로, DB 일은 DB 큐로 넘기고 바로 리턴한다.
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

bool Handle_C_LOGIN(PacketSessionRef& session, Protocol::C_LOGIN& pkt)
{
    // 토큰을 확인하기 전에는 userId를 모르므로 아무 DB 큐나 고른다.
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
                GLogger->Warning("액세스 토큰이 없거나 이미 쓰였다");
                KickSession(gameSession, Protocol::LEAVE_REASON_INVALID_TOKEN, "Invalid Access Token");
                return;
            }

            Json json = Json::parse(*val);
            string username = json["username"];
            int64 userId = json["userId"];

            GLogger->Info("로그인 userId: {}, username: {}", userId, username);

            // 한 계정은 세션 하나만 가진다. 나중에 온 로그인이 이기고 기존 세션은 끊긴다.
            // 기존 세션의 룸 퇴장과 저장은 접속 종료 경로(GameSession::OnDisconnected)가 한다.
            if (GameSessionRef replaced = GSessionManager->RegisterUser(userId, gameSession))
            {
                // 밀어내기 전에 조율자에 알린다. 기존 세션의 저장이 끝나기 전에 새 세션이 입장하지 않게 한다.
                GProgressCoordinator->OnDuplicateLogin(userId, replaced->GetPlayer());

                KickSession(replaced, Protocol::LEAVE_REASON_DUPLICATE_LOGIN, "Duplicate Login");
            }

            // 등록하기 전에 이 세션이 이미 끊겼다면 접속 종료의 UnregisterUser가 먼저 지나갔다. 등록을 여기서 거둔다.
            if (gameSession->IsConnected() == false)
            {
                GSessionManager->UnregisterUser(gameSession);
                return;
            }

            Protocol::S_LOGIN loginPkt;
            try
            {
                vector<Protocol::CharacterOverview> characters;
                DBConnectionGuard conn;
                CharacterListDAO::LoadCharacterList(*conn, userId, OUT characters);

                for (Protocol::CharacterOverview& character : characters)
                    *loginPkt.add_characters() = std::move(character);

                // 클라이언트는 이 값으로 슬롯을 보여 주고 생성 버튼을 막는다. CreateCharacter가 막는 한도와 같은 값이다.
                loginPkt.set_character_slot_count(CharacterCreation::DEFAULT_CHARACTER_SLOT_COUNT);
                loginPkt.set_success(true);
            }
            catch (const exception& error)
            {
                GLogger->Error("계정 {} 캐릭터 목록 불러오기 실패: {}", userId, error.what());
                loginPkt.Clear();
                loginPkt.set_success(false);
            }

            SendPacket(session, loginPkt);
        }
    );

    dbQueue->Push(std::move(job));

	return true;
}

bool Handle_C_CREATE_CHARACTER(PacketSessionRef& session, Protocol::C_CREATE_CHARACTER& pkt)
{
    // 계정 번호는 여기서 한 번 읽는다. 로그인 검사와 잡이 같은 값을 쓴다.
    const int64 userId = static_pointer_cast<GameSession>(session)->GetUserId();

    // 세션 상태는 핸들러가 본다. 로그인하지 않은 세션은 계정 번호 0으로 INSERT를 시도하게 된다.
    if (userId == 0)
    {
        Protocol::S_CREATE_CHARACTER createCharacterPkt;
        createCharacterPkt.set_success(false);
        createCharacterPkt.set_cause("로그인이 필요합니다.");
        SendPacket(session, createCharacterPkt);
        return false;
    }

    // 거절하면 DB 큐로 넘기지 않고 사유를 곧바로 돌려준다. 클라이언트가 사유를 생성 화면에 띄운다.
    if (optional<string> cause = CharacterCreation::Validate(pkt.character()))
    {
        Protocol::S_CREATE_CHARACTER createCharacterPkt;
        createCharacterPkt.set_success(false);
        createCharacterPkt.set_cause(cause.value());
        SendPacket(session, createCharacterPkt);
        return true;
    }

    // 한 계정의 DB 작업이 순서대로 돌도록 userId로 DB 큐를 고른다.
    DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(userId);

    JobRef job = make_shared<Job>(
        [session, pkt, userId]()
        {
            Protocol::S_CREATE_CHARACTER createCharacterPkt;
            try
            {
                DBConnectionGuard conn;
                const CreateCharacterResult result = CharacterListDAO::CreateCharacter(*conn, pkt.character(), userId);
                if (result.rejection.has_value())
                {
                    createCharacterPkt.set_success(false);
                    createCharacterPkt.set_cause(string(ToMessage(result.rejection.value())));
                }
                else
                {
                    createCharacterPkt.set_success(true);
                    createCharacterPkt.set_character_id(result.characterId);
                }
            }
            catch (const exception& error)
            {
                GLogger->Error("계정 {} 캐릭터 생성 실패: {}", userId, error.what());
                createCharacterPkt.Clear();
                createCharacterPkt.set_success(false);
                createCharacterPkt.set_cause("서버 내부 오류");
            }

            SendPacket(session, createCharacterPkt);
        }
    );

    dbQueue->Push(std::move(job));

    return true;
}

bool Handle_C_DELETE_CHARACTER(PacketSessionRef& session, Protocol::C_DELETE_CHARACTER& pkt)
{
    // 계정 번호는 여기서 한 번 읽는다. 로그인 검사와 잡이 같은 값을 쓴다.
    const int64 userId = static_pointer_cast<GameSession>(session)->GetUserId();

    if (userId == 0)
    {
        Protocol::S_DELETE_CHARACTER deleteCharacterPkt;
        deleteCharacterPkt.set_success(false);
        SendPacket(session, deleteCharacterPkt);
        return false;
    }

    // 소유 확인은 DeleteCharacter의 SQL이 user_id를 함께 대조해서 한다.

    // 한 계정의 DB 작업이 순서대로 돌도록 userId로 DB 큐를 고른다.
    DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(userId);

    // 계정 번호는 핸들러가 여기서 한 번 읽어 넘긴다. 잡이 세션에서 다시 읽으면 그사이 바뀐 계정으로 일한다.
    JobRef job = make_shared<Job>(
        [session, pkt, userId]()
        {
            const int64 characterId = pkt.character_id();

            Protocol::S_DELETE_CHARACTER deleteCharacterPkt;
            try
            {
                DBConnectionGuard conn;
                deleteCharacterPkt.set_success(CharacterListDAO::DeleteCharacter(*conn, userId, characterId));
                if (deleteCharacterPkt.success())
                    deleteCharacterPkt.set_character_id(characterId);
            }
            catch (const exception& error)
            {
                GLogger->Error("계정 {} 캐릭터 {} 삭제 실패: {}", userId, characterId, error.what());
                deleteCharacterPkt.set_success(false);
            }

            SendPacket(session, deleteCharacterPkt);
        }
    );

    dbQueue->Push(std::move(job));

    return true;
}

bool Handle_C_ENTER_GAME(PacketSessionRef& session, Protocol::C_ENTER_GAME& pkt)
{
    // 계정 번호는 여기서 한 번 읽는다. 로그인 검사와 잡이 같은 값을 쓴다.
    const int64 userId = static_pointer_cast<GameSession>(session)->GetUserId();

    // 로그인하지 않았거나 이미 입장한 세션의 입장 요청은 DB에 가기 전에 거절한다.
    // 이미 입장했는지는 잡 안의 GameEntry::SpawnPlayer도 다시 막는다.
    if (userId == 0 || static_pointer_cast<GameSession>(session)->GetPlayer() != nullptr)
    {
        SendEnterGameFail(session);
        return false;
    }

    // 플레이어 생성은 잡 안에서 한다. C_ENTER_GAME은 character_id만 싣고 오고
    // room_id는 LoadAllCharactersData가 DB에서 읽어야 알 수 있으므로,
    // 이 시점에 넘길 룸 큐가 없다. 아키텍처가 게임 입장에 지정한 경로가 DBQueue이고
    // 생성 직후의 소비자도 같은 잡이라 여기로 모은다.
    // 언제 어느 DB 큐에서 돌지는 조율자가 정한다. 계정 번호는 핸들러가 여기서 한 번 읽어 넘긴다.
    auto load = [session, pkt, userId]()
        {
            // 저장을 기다리는 사이에 끊겼으면 불러올 이유가 없다. 불러오면 끊긴 것을 알아챈 뒤 또 저장한다.
            if (session->IsConnected() == false)
                return;

            // 불러오기가 모두 성공하고 검증을 통과해야 세션에 플레이어가 생긴다. 룸에는 그 뒤 EnterPlayer로 들어간다.
            Protocol::S_ENTER_GAME enterGamePkt;
            try
            {
                DBConnectionGuard conn;
                enterGamePkt = GameEntry::Enter(*conn, static_pointer_cast<GameSession>(session), userId, pkt.character_id());
            }
            catch (const exception& error)
            {
                // 실패해도 응답을 보내야 클라이언트가 캐릭터 선택 화면에서 기다리지 않는다.
                GLogger->Error("캐릭터 {} 입장 실패: {}", pkt.character_id(), error.what());
                enterGamePkt.set_success(false);
            }
            SendPacket(session, enterGamePkt);
        };

    auto reject = [session]()
        {
            SendEnterGameFail(session);
        };

    GProgressCoordinator->RequestEnter(userId, { std::move(load), std::move(reject) });

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

    PlayerRef player = gameSession->GetPlayer();
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

            SendPacket(session, enterMapPkt);
        }

        return false;
    }

    // 플레이어 상태를 소유한 룸의 큐로 넘긴다. 아직 어떤 룸에도 속하지 않았다면 요청한 룸의 큐로 넘긴다.
    // 그 경우 판정은 불러온 룸과 같은지만 보므로, 다른 룸을 요청하면 그 룸의 큐에서 거절된다.
    RoomRef room = player->GetRoom();
    if (room == nullptr)
        room = GRoomManager->FindRoom(roomId);

    if (room == nullptr)
    {
        // 넘길 큐가 없으면 잡을 만들 수 없다. 이전 코드가 이 입력에도 응답을 돌려줬으므로
        // 클라를 대기 상태로 남기지 않도록 실패 응답은 유지한다.
        GLogger->Warning("C_ENTER_MAP을 넘길 Room을 찾지 못함. roomId: {}", roomId);

        Protocol::S_ENTER_MAP enterMapPkt;
        {
            enterMapPkt.set_success(false);
            enterMapPkt.set_map_id(pkt.map_id());
            enterMapPkt.set_room_id(roomId);

            SendPacket(session, enterMapPkt);
        }

        return false;
    }

    // 여기서 기록한 enteringRoomId를 뒤이어 C_HandleEnterRoom이 같은 큐에서 읽는다.
    room->DoAsync(&Room::C_HandleEnterMap, pkt, player);

    return true;
}

bool Handle_C_ENTER_ROOM(PacketSessionRef& session, Protocol::C_ENTER_ROOM& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->GetPlayer();
    if (player == nullptr)
        return false;

    RoomRef curRoom = player->GetRoom();
    if (curRoom == nullptr)
    {
        // 아직 어떤 Room에도 속하지 않은 최초 입장.
        // 소속 룸은 Room::EnterPlayer 안에서만 세팅되므로 여기서는 항상 비어 있다.
        // 클라이언트가 무엇을 보냈든 서버가 INITIAL로 판정하고, 입장할 Room의 큐로 넘긴다.
        pkt.set_enter_type(Protocol::ENTER_TYPE_INITIAL);

        int32 roomId = pkt.has_room_id() ? pkt.room_id() : player->GetEnteringRoomId();
        RoomRef enterRoom = GRoomManager->FindRoom(roomId);
        if (enterRoom == nullptr)
        {
            GLogger->Warning("최초 입장할 Room을 찾지 못함. roomId: {}", roomId);
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
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            room->C_HandleMove(pkt, player);
        });
}

bool Handle_C_CHAT(PacketSessionRef& session, Protocol::C_CHAT& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            // 같은 Room의 모든 플레이어에게 그대로 중계한다(본인 포함).
            Protocol::S_CHAT chatPkt;
            chatPkt.set_entity_id(player->GetEntityId());
            chatPkt.set_msg(pkt.msg());

            room->Broadcast(ServerPacketHandler::MakeSerializedPacket(chatPkt));
        });
}

bool Handle_C_NORMAL_ATTACK(PacketSessionRef& session, Protocol::C_NORMAL_ATTACK& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            room->C_HandleNormalAttack(pkt, player);
        });
}

bool Handle_C_BUY_ITEM(PacketSessionRef& session, Protocol::C_BUY_ITEM& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            ItemRequests::HandleBuyItem(*room, player, pkt);
        });
}

bool Handle_C_SELL_ITEM(PacketSessionRef& session, Protocol::C_SELL_ITEM& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            ItemRequests::HandleSellItem(*room, player, pkt);
        });
}

bool Handle_C_EQUIP_GEAR(PacketSessionRef& session, Protocol::C_EQUIP_GEAR& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            ItemRequests::HandleEquipGear(*room, player, pkt);
        });
}


bool Handle_C_UNEQUIP_GEAR(PacketSessionRef& session, Protocol::C_UNEQUIP_GEAR& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            ItemRequests::HandleUnequipGear(*room, player, pkt);
        });
}

bool Handle_C_USE_ITEM(PacketSessionRef& session, Protocol::C_USE_ITEM& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            ItemRequests::HandleUseItem(*room, player, pkt);
        });
}

bool Handle_C_RESPAWN(PacketSessionRef& session, Protocol::C_RESPAWN& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            room->C_HandleRespawn(pkt, player);
        });
}

