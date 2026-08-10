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
}

void GameSessionManager::Broadcast(SendBufferRef sendBuffer)
{
	USE_LOCK;
	for (GameSessionRef session : _sessions)
	{
		session->Send(sendBuffer);
	}
}