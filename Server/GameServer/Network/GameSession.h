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
	/** 플레이어가 룸에 있으면 끊길 때 진행을 저장한다. 저장 대기(SaveGate)를 걸지 정할 때 쓴다. */
	bool IsPlayerInRoom();

	/** 접속 종료한 플레이어를 룸에서 빼고, 룸이 저장할 데이터를 내주면 DB 큐에 저장을 넣는다. room의 큐 위에서만 부른다. */
	static void LeaveGame(RoomRef room, PlayerRef player);

public:
	atomic<PlayerRef> _player;
    int64 _userId = 0;
};