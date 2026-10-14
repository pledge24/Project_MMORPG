#pragma once

class Player;
class Room;

/**
 * 게임 클라이언트 하나와의 연결.
 *
 * 클라이언트가 보낸 패킷을 처리 창구로 넘기고, 이 연결의 계정과 입장한 플레이어를 기억한다.
 */
class GameSession : public PacketSession
{
	//~ Begin PacketSession Interface
protected:
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;
	//~ End PacketSession Interface

	//~ 플레이어
public:
	/** 입장한 플레이어. 입장 전이면 nullptr. 어느 스레드에서 불러도 된다. */
	PlayerRef GetPlayer() const { return _player.load(); }
	/** 세션에 플레이어가 없을 때만 player를 등록한다. 이미 있으면 바꾸지 않고 false. */
	bool TryRegisterPlayer(const PlayerRef& player);

private:
	atomic<PlayerRef> _player;

	//~ 계정
public:
	/**
	 * 로그인하기 전이면 0. 로그인 잡이 언제든 바꿀 수 있으므로,
	 * 요청 하나에서는 핸들러가 한 번 읽은 값을 잡에 넘겨 쓰고 잡 안에서 다시 읽지 않는다.
	 */
	int64 GetUserId() const { return _userId.load(); }

private:
	friend class GameSessionManager;
	void SetUserId(int64 userId) { _userId.store(userId); }

	atomic<int64> _userId = 0;
};