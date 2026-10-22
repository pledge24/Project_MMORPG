#include "Core/pch.h"
#include "Network/AuthTicket.h"
#include "Network/GameEntry.h"
#include "Network/GameSessionManager.h"
#include "Network/ProgressCoordinator.h"
#include "DB/DAOCommon.h"
#include "Lobby/AccountDAO.h"
#include "Lobby/CharacterCreation.h"
#include "Lobby/CharacterListDAO.h"
#include "Utils/SnowflakeIdGenerator.h"

// 로비 패킷의 핸들러. 선언은 생성된 핸들러 헤더에 있다.
// 세션 단계는 GameSession::OnRecvPacket이 확인하므로, 여기 오는 패킷은 그 패킷을 받는 단계의 세션이 보낸 것이다.
// 로비는 배선과 게임 입장 창구(GameEntry::RequestEnter)만 부른다.

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

    // 게임 입장 창구로 넘기기 전에 거절한다. 세션은 로비로 돌아가 다시 고른 캐릭터의 입장을 받는다.
    void RejectEnterGame(const GameSessionRef& session)
    {
        Protocol::S_ENTER_GAME enterGameFailPkt;
        enterGameFailPkt.set_success(false);
        GameEntry::FinishEnter(session, enterGameFailPkt);
    }
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

            // 인증 티켓은 한 번만 쓴다. 같은 티켓의 로그인이 다른 DB 큐에서 동시에 와도
            // 키를 지운 쪽 하나만 통과한다. 키 형식은 인증 서버와의 계약이다.
            const string ticketKey = "authTicket:" + pkt.access_token();
            RedisRef redis = GRedisManager->GetRedis();
            auto val = redis->get(ticketKey);
            if (val.has_value() == false || redis->del(ticketKey) == 0)
            {
                GLogger->Warning("인증 티켓이 없거나 이미 쓰였다");
                KickSession(gameSession, Protocol::LEAVE_REASON_INVALID_TOKEN, "인증 티켓 없음");
                return;
            }

            // 티켓은 이미 지웠다. 값을 읽지 못하면 다시 쓸 수 없으므로 응답 없이 끝내지 말고 끊는다.
            optional<int64> accountId = AuthTicket::ParseAccountId(*val);
            if (accountId.has_value() == false)
            {
                GLogger->Warning("인증 티켓 값의 형식이 틀렸다");
                KickSession(gameSession, Protocol::LEAVE_REASON_INVALID_TOKEN, "인증 티켓 값 형식 오류");
                return;
            }

            const int64 userId = *accountId;
            GLogger->Info("로그인 userId: {}", userId);

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
                DBConnectionGuard conn;
                // 이 월드에 처음 들어온 계정이면 계정 행을 만든다. 캐릭터 생성은 이 행이 있어야 한다.
                const int32 characterSlotCount = AccountDAO::RecordLogin(*conn, userId);

                vector<Protocol::CharacterOverview> characters;
                CharacterListDAO::LoadCharacterList(*conn, userId, OUT characters);

                for (Protocol::CharacterOverview& character : characters)
                    *loginPkt.add_characters() = std::move(character);

                // 클라이언트는 이 값으로 슬롯을 보여 주고 생성 버튼을 막는다. CreateCharacter가 막는 한도와 같은 계정 행의 값이다.
                loginPkt.set_character_slot_count(characterSlotCount);
                loginPkt.set_success(true);
            }
            catch (const exception& error)
            {
                GLogger->Error("계정 {} 캐릭터 목록 불러오기 실패: {}", userId, error.what());
                loginPkt.Clear();
                loginPkt.set_success(false);
            }

            // 목록을 보낸 뒤에 바꾸면 목록을 받은 클라이언트의 첫 로비 요청이 로비 접속 전 단계에서 버려진다.
            if (loginPkt.success())
                gameSession->CompleteLobbyConnect();

            SendPacket(session, loginPkt);
        }
    );

    dbQueue->Push(std::move(job));

    return true;
}

bool Handle_C_CREATE_CHARACTER(PacketSessionRef& session, Protocol::C_CREATE_CHARACTER& pkt)
{
    // 계정 번호는 여기서 한 번 읽어 잡에 넘긴다. 잡이 세션에서 다시 읽으면 그사이 바뀐 계정으로 일한다.
    const int64 userId = static_pointer_cast<GameSession>(session)->GetUserId();

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
                // 거절된 생성이 받은 ID는 다시 쓰지 않고 버린다.
                const CreateCharacterResult result = CharacterListDAO::CreateCharacter(*conn, userId, GIdGenerator->Next(), pkt.character());
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
    // 계정 번호는 여기서 한 번 읽어 잡에 넘긴다. 잡이 세션에서 다시 읽으면 그사이 바뀐 계정으로 일한다.
    const int64 userId = static_pointer_cast<GameSession>(session)->GetUserId();

    // 한 계정의 DB 작업이 순서대로 돌도록 userId로 DB 큐를 고른다.
    DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(userId);

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
            bool owned = false;
            try
            {
                DBConnectionGuard conn;
                owned = CharacterListDAO::OwnsCharacter(*conn, userId, characterId);
            }
            catch (const exception& error)
            {
                GLogger->Error("계정 {} 캐릭터 {} 소유 확인 실패: {}", userId, characterId, error.what());
                RejectEnterGame(gameSession);
                return;
            }

            if (owned == false)
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
