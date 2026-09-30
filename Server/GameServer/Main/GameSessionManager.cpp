#include "pch.h"
#include "GameSessionManager.h"
#include "GameSession.h"

GameSessionManager GSessionManager;

void GameSessionManager::Add(GameSessionRef session)
{
	USE_LOCK;
    cout << "GameSession Added in Manager" << endl;
	_sessions.insert(session);
}

void GameSessionManager::Remove(GameSessionRef session)
{
	USE_LOCK;
    cout << "GameSession Removed in Manager" << endl;
	_sessions.erase(session);

	// 새 로그인에 밀려난 세션이면 계정은 이미 새 세션을 가리킨다. 그 등록은 지우지 않는다.
	auto it = _userSessions.find(session->_userId);
	if (it != _userSessions.end() && it->second == session)
		_userSessions.erase(it);
}

// 확인과 교체를 락 하나 안에서 한다. C_LOGIN은 무작위 DB 큐에서 돌아서 같은 계정의 로그인이 동시에 올 수 있다.
GameSessionRef GameSessionManager::RegisterUser(int64 userId, GameSessionRef session)
{
	USE_LOCK;
	session->_userId = userId;

	GameSessionRef& current = _userSessions[userId];
	GameSessionRef replaced = std::exchange(current, session);
	return replaced == session ? nullptr : replaced;
}

void GameSessionManager::Broadcast(SendBufferRef sendBuffer)
{
	USE_LOCK;
	for (GameSessionRef session : _sessions)
	{
		session->Send(sendBuffer);
	}
}