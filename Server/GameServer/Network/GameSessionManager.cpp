#include "Core/pch.h"
#include "Network/GameSessionManager.h"

GameSessionRef GameSessionManager::RegisterUser(int64 userId, GameSessionRef session)
{
    // 같은 계정의 로그인 잡이 서로 다른 DB 큐에서 동시에 돌 수 있다. 확인과 교체가 한 락 안에 있어야
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

void GameSessionManager::EraseRegistration(const GameSessionRef& session)
{
    // 새 로그인에 밀려난 세션이면 계정은 이미 새 세션을 가리킨다. 그 등록은 지우지 않는다.
    auto it = _userSessions.find(session->GetUserId());
    if (it != _userSessions.end() && it->second == session)
        _userSessions.erase(it);
}