#pragma once

class GameSession;

using GameSessionRef = shared_ptr<GameSession>;

class GameSessionManager
{
public:
	void Add(GameSessionRef session);
	void Remove(GameSessionRef session);
	void Broadcast(SendBufferRef sendBuffer);

	// 세션을 계정에 묶는다. 한 계정은 세션 하나만 가지므로 같은 계정의 기존 세션이 있으면 교체하고
	// 그 세션을 돌려준다. 돌려받은 세션을 끊는 것은 호출자의 몫이다.
	GameSessionRef RegisterUser(int64 userId, GameSessionRef session);

private:
	MAKE_LOCK;
	set<GameSessionRef> _sessions;
	map<int64, GameSessionRef> _userSessions; // userId → 그 계정의 현재 세션
};

extern GameSessionManager GSessionManager;
