#include "Core/pch.h"
#include "Network/GameSessionManager.h"
#include "Network/ProgressCoordinator.h"

bool GameSessionManager::BindAccount(int64 userId, const GameSessionRef& session)
{
    // 한 계정은 세션 하나만 가진다. 나중에 온 로비 접속이 이기고 앞 세션은 끊긴다.
    // 앞 세션의 룸 퇴장과 저장은 접속 종료 경로(GameSession::OnDisconnected)가 한다.
    if (GameSessionRef replaced = RegisterUser(userId, session))
    {
        // 밀어내기 전에 조율자에 알린다. 앞 세션의 저장이 끝나기 전에 새 세션이 입장하지 않게 한다.
        GProgressCoordinator->OnDuplicateConnection(userId, replaced->GetPlayer());

        Protocol::S_DISCONNECT disconnectPkt;
        disconnectPkt.set_reason(Protocol::DISCONNECT_REASON_DUPLICATE_CONNECTION);
        SendPacketAndDisconnect(replaced, disconnectPkt, "같은 계정의 새 로비 접속");
    }

    // 등록하기 전에 이 세션이 이미 끊겼다면 접속 종료의 UnregisterUser가 먼저 지나갔다. 등록을 여기서 거둔다.
    if (session->IsConnected() == false)
    {
        UnregisterUser(session);
        return false;
    }

    return true;
}

GameSessionRef GameSessionManager::RegisterUser(int64 userId, GameSessionRef session)
{
    // 같은 계정의 로비 접속 잡이 서로 다른 DB 큐에서 동시에 돌 수 있다. 확인과 교체가 한 락 안에 있어야
    // 둘 중 나중 쪽이 먼저 쪽을 기존 세션으로 돌려받는다. 나누면 밀려난 세션이 끊기지 않고 남는다.
    USE_LOCK

    // 이전 등록을 먼저 지우므로, 같은 계정으로 다시 등록해도 자기 자신이 교체 대상으로 나오지 않는다.
    EraseRegistration(session);
    session->SetUserId(userId);
    return std::exchange(_userSessions[userId], session);
}

void GameSessionManager::UnregisterUser(GameSessionRef session)
{
    USE_LOCK
    EraseRegistration(session);
}

int32 GameSessionManager::GetAccountCount()
{
    USE_LOCK
    return static_cast<int32>(_userSessions.size());
}

void GameSessionManager::EraseRegistration(const GameSessionRef& session)
{
    // 새 로비 접속에 밀려난 세션이면 계정은 이미 새 세션을 가리킨다. 그 등록은 지우지 않는다.
    auto it = _userSessions.find(session->GetUserId());
    if (it != _userSessions.end() && it->second == session)
        _userSessions.erase(it);
}