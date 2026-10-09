#include "Core/pch.h"
#include "Network/GameSessionManager.h"

GameSessionRef GameSessionManager::RegisterUser(int64 userId, GameSessionRef session)
{
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