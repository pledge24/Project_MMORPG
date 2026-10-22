#include "Core/pch.h"
#include "Network/AuthTicket.h"
#include "Network/GameEntry.h"
#include "Network/GameSessionManager.h"
#include "DB/DAOCommon.h"
#include "Lobby/CharacterCreation.h"
#include "Lobby/LobbyRequests.h"
#include "Utils/SnowflakeIdGenerator.h"

// 로비 패킷의 핸들러. 선언은 생성된 핸들러 헤더에 있다.
// 세션 단계는 GameSession::OnRecvPacket이 확인하므로, 여기 오는 패킷은 그 패킷을 받는 단계의 세션이 보낸 것이다.
// 로비는 배선과 게임 입장 창구(GameEntry::RequestEnter)만 부른다.

namespace
{
    // 로비 접속에 실패하면 결과를 알린 뒤 끊는다. 클라이언트는 결과 값으로 로그인 화면에 문구를 띄운다.
    void RejectLobbyConnect(const GameSessionRef& session, Protocol::LobbyResult result, const char* cause)
    {
        Protocol::S_CONNECT_LOBBY connectLobbyPkt;
        connectLobbyPkt.set_result(result);
        SendPacketAndDisconnect(session, connectLobbyPkt, cause);
    }

    // 게임 입장 창구로 넘기기 전에 거절한다. 세션은 로비로 돌아가 다시 고른 캐릭터의 입장을 받는다.
    void RejectEnterGame(const GameSessionRef& session)
    {
        Protocol::S_ENTER_GAME enterGameFailPkt;
        enterGameFailPkt.set_success(false);
        GameEntry::FinishEnter(session, enterGameFailPkt);
    }
}

bool Handle_C_CONNECT_LOBBY(PacketSessionRef& session, Protocol::C_CONNECT_LOBBY& pkt)
{
    GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

    // 받는 순간 단계를 바꾼다. 잡이 끝난 뒤에 바꾸면 그 전에 같은 세션이 보낸 두 번째 로비 접속도 단계 검사를 통과한다.
    if (gameSession->BeginLobbyConnect() == false)
        return false;

    // 인증 티켓을 확인하기 전에는 userId를 모르므로 아무 DB 큐나 고른다.
    int32 dbQueueCount = GDBManager->GetDBQueueCount();
    DBQueueRef dbQueue = GDBManager->GetDBQueue(Utils::GetRandom(0, dbQueueCount - 1));

    JobRef job = make_shared<Job>(
        [gameSession, authTicket = pkt.auth_ticket()]()
        {
            // 인증 티켓은 한 번만 쓴다. 같은 티켓의 로비 접속이 다른 DB 큐에서 동시에 와도
            // 키를 지운 쪽 하나만 통과한다. 키 형식은 인증 서버와의 계약이다.
            const string ticketKey = "authTicket:" + authTicket;
            RedisRef redis = GRedisManager->GetRedis();
            auto val = redis->get(ticketKey);
            if (val.has_value() == false || redis->del(ticketKey) == 0)
            {
                GLogger->Warning("인증 티켓이 없거나 이미 쓰였다");
                RejectLobbyConnect(gameSession, Protocol::LOBBY_RESULT_INVALID_AUTH_TICKET, "인증 티켓 없음");
                return;
            }

            // 티켓은 이미 지웠다. 값을 읽지 못하면 다시 쓸 수 없으므로 응답 없이 끝내지 말고 끊는다.
            optional<int64> accountId = AuthTicket::ParseAccountId(*val);
            if (accountId.has_value() == false)
            {
                GLogger->Warning("인증 티켓 값의 형식이 틀렸다");
                RejectLobbyConnect(gameSession, Protocol::LOBBY_RESULT_INVALID_AUTH_TICKET, "인증 티켓 값 형식 오류");
                return;
            }

            const int64 userId = *accountId;
            GLogger->Info("로비 접속 userId: {}", userId);

            // 같은 계정의 앞 연결은 세션 관리자가 밀어낸다. 이 세션이 그새 끊겼으면 할 일이 없다.
            if (GSessionManager->BindAccount(userId, gameSession) == false)
                return;

            Protocol::S_CHARACTER_LIST characterListPkt;
            try
            {
                DBConnectionGuard conn;
                characterListPkt = LobbyRequests::ConnectLobby(*conn, userId);
            }
            catch (const exception& error)
            {
                GLogger->Error("계정 {} 캐릭터 목록 불러오기 실패: {}", userId, error.what());
                RejectLobbyConnect(gameSession, Protocol::LOBBY_RESULT_INTERNAL_ERROR, "캐릭터 목록 불러오기 실패");
                return;
            }

            // 응답을 보낸 뒤에 바꾸면 목록을 받은 클라이언트의 첫 로비 요청이 로비 접속 중 단계에서 버려진다.
            gameSession->CompleteLobbyConnect();

            Protocol::S_CONNECT_LOBBY connectLobbyPkt;
            connectLobbyPkt.set_result(Protocol::LOBBY_RESULT_SUCCESS);
            SendPacket(gameSession, connectLobbyPkt);
            SendPacket(gameSession, characterListPkt);
        }
    );

    dbQueue->Push(std::move(job));

    return true;
}

bool Handle_C_CREATE_CHARACTER(PacketSessionRef& session, Protocol::C_CREATE_CHARACTER& pkt)
{
    // 계정 번호는 여기서 한 번 읽어 잡에 넘긴다. 잡이 세션에서 다시 읽으면 그사이 바뀐 계정으로 일한다.
    const int64 userId = static_pointer_cast<GameSession>(session)->GetUserId();

    // 거절하면 DB 큐로 넘기지 않고 결과를 곧바로 돌려준다. 캐릭터 ID도 발급하지 않는다.
    if (optional<Protocol::LobbyResult> rejection = CharacterCreation::Validate(pkt.name(), pkt.character_class()))
    {
        Protocol::S_CREATE_CHARACTER createCharacterPkt;
        createCharacterPkt.set_result(rejection.value());
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
                // 거절된 생성이 받은 ID는 다시 쓰지 않고 버린다.
                createCharacterPkt = LobbyRequests::CreateCharacter(*conn, userId, GIdGenerator->Next(), pkt);
            }
            catch (const exception& error)
            {
                GLogger->Error("계정 {} 캐릭터 생성 실패: {}", userId, error.what());
                createCharacterPkt.Clear();
                createCharacterPkt.set_result(Protocol::LOBBY_RESULT_INTERNAL_ERROR);
            }

            SendPacket(session, createCharacterPkt);
        }
    );

    dbQueue->Push(std::move(job));

    return true;
}

bool Handle_C_DELETE_CHARACTER(PacketSessionRef& session, Protocol::C_DELETE_CHARACTER& pkt)
{
    // 계정 번호는 여기서 한 번 읽어 잡에 넘긴다. 잡이 세션에서 다시 읽으면 그사이 바뀐 계정으로 일한다.
    const int64 userId = static_pointer_cast<GameSession>(session)->GetUserId();

    // 한 계정의 DB 작업이 순서대로 돌도록 userId로 DB 큐를 고른다.
    DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(userId);

    JobRef job = make_shared<Job>(
        [session, userId, characterId = pkt.character_id()]()
        {
            Protocol::S_DELETE_CHARACTER deleteCharacterPkt;
            try
            {
                DBConnectionGuard conn;
                deleteCharacterPkt = LobbyRequests::DeleteCharacter(*conn, userId, characterId);
            }
            catch (const exception& error)
            {
                GLogger->Error("계정 {} 캐릭터 {} 삭제 실패: {}", userId, characterId, error.what());
                deleteCharacterPkt.Clear();
                deleteCharacterPkt.set_result(Protocol::LOBBY_RESULT_INTERNAL_ERROR);
                deleteCharacterPkt.set_character_id(characterId);
            }

            SendPacket(session, deleteCharacterPkt);
        }
    );

    dbQueue->Push(std::move(job));

    return true;
}

bool Handle_C_ENTER_GAME(PacketSessionRef& session, Protocol::C_ENTER_GAME& pkt)
{
    GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

    // 계정 번호는 여기서 한 번 읽어 잡에 넘긴다. 잡이 세션에서 다시 읽으면 그사이 바뀐 계정으로 일한다.
    const int64 userId = gameSession->GetUserId();

    // 입장이 끝나거나 거절될 때까지 이 세션의 로비 요청을 받지 않는다. 입장 중인 캐릭터를 지우거나 두 번 입장하지 못하게 한다.
    if (gameSession->BeginGameEntry() == false)
        return false;

    // 한 계정의 DB 작업이 순서대로 돌도록 userId로 DB 큐를 고른다.
    DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(userId);

    JobRef job = make_shared<Job>(
        [gameSession, userId, characterId = pkt.character_id()]()
        {
            // 캐릭터 선택은 로비가 받는다. 이 계정의 삭제되지 않은 캐릭터일 때만 게임 입장 창구로 넘긴다.
            Protocol::LobbyResult result = Protocol::LOBBY_RESULT_INTERNAL_ERROR;
            try
            {
                DBConnectionGuard conn;
                result = LobbyRequests::SelectCharacter(*conn, userId, characterId);
            }
            catch (const exception& error)
            {
                GLogger->Error("계정 {} 캐릭터 {} 소유 확인 실패: {}", userId, characterId, error.what());
                RejectEnterGame(gameSession);
                return;
            }

            if (result != Protocol::LOBBY_RESULT_SUCCESS)
            {
                GLogger->Warning("계정 {}의 캐릭터가 아닌 {}로 입장을 요청했다", userId, characterId);
                RejectEnterGame(gameSession);
                return;
            }

            GameEntry::RequestEnter(gameSession, userId, characterId);
        }
    );

    dbQueue->Push(std::move(job));

    return true;
}
