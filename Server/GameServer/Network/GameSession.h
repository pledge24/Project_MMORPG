#pragma once

class Player;
class Room;

/**
 * 게임 클라이언트 하나와의 연결.
 * Service -> GameSession -> Player 순으로 참조 관계가 성립한다.
 */
class GameSession : public PacketSession
{
public:
	~GameSession()
	{
		cout << "~GameSession" << endl;
	}

protected:
	//~ Session/PacketSession 통신 이벤트 인터페이스 구현
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;
    
public:
	//~ 플레이어
	/** 입장한 플레이어. 입장 전이면 nullptr. 입장 잡(DB 스레드)이 쓰고, IOCP 스레드(핸들러, 접속 종료)와 룸 큐가 읽는다. */
	PlayerRef GetPlayer() const { return _player.load(); }
	/**
	 * 세션이 비어 있을 때만 player를 등록하고 true를 돌려준다. 이미 플레이어가 있으면 바꾸지 않고 false.
	 * 같은 세션의 입장 잡 둘이 다른 DB 큐에서 돌 수 있으므로 확인과 등록을 한 번에 한다.
	 */
	bool TryRegisterPlayer(const PlayerRef& player);

	//~ 계정
	/**
	 * 로그인하기 전이면 0. 로그인 잡(DB 스레드)이 GameSessionManager의 락 안에서 쓰고,
	 * IOCP 스레드(핸들러, 접속 종료)가 락 없이 읽는다. 그래서 atomic이다.
	 * 요청 하나는 핸들러가 한 번 읽은 값을 잡에 넘겨 쓴다. 잡 안에서 다시 읽지 않는다.
	 */
	int64 GetUserId() const { return _userId.load(); }

private:
	friend class GameSessionManager;
	void SetUserId(int64 userId) { _userId.store(userId); }

	atomic<PlayerRef> _player;
	atomic<int64> _userId = 0;
};