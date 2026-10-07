#pragma once

class GameSession;

using GameSessionRef = shared_ptr<GameSession>;

/**
 * 접속 중인 모든 GameSession과, 계정(userId)마다 현재 세션을 들고 있다. 전역 객체 GSessionManager 하나만 있다.
 * IOCP 워커와 DB 스레드 등 여러 스레드에서 부르므로 모든 함수가 락을 잡는다.
 * 세션은 OnConnected에서 들어오고 OnDisconnected에서 빠진다. 그동안 이 객체가 세션의 수명을 붙잡는다.
 */
class GameSessionManager
{
public:
	void Add(GameSessionRef session);
	/** 세션 집합에서 빼고, 계정 등록이 이 세션을 가리킬 때만 그 등록도 지운다. */
	void Remove(GameSessionRef session);
	/** 락을 잡은 채로 모든 세션에 보낸다. */
	void Broadcast(SendBufferRef sendBuffer);

	/**
	 * 세션을 계정에 묶는다. 한 계정은 세션 하나만 가지므로 같은 계정의 기존 세션이 있으면 교체하고
	 * 그 세션을 돌려준다. 교체한 세션이 없으면 nullptr를 돌려준다. 돌려받은 세션을 끊는 것은 호출자의 몫이다.
	 * 이 세션이 다른 계정에 묶여 있었으면 그 등록을 거두고 session->_userId를 userId로 바꾼다.
	 */
	GameSessionRef RegisterUser(int64 userId, GameSessionRef session);

private:
	MAKE_LOCK;
	set<GameSessionRef> _sessions;
	map<int64, GameSessionRef> _userSessions; // userId → 그 계정의 현재 세션
};

extern GameSessionManager GSessionManager;
