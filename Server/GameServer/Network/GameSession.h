#pragma once

class Player;
class Room;

/**
 * 게임 클라이언트 하나와의 연결. 받은 패킷을 ServerPacketHandler로 넘기고, 끊기면 룸 퇴장과 진행 저장을 시작한다.
 * On* 훅은 모두 IOCP 워커 스레드에서 불린다.
 * 연결된 동안 Service의 세션 집합과 GSessionManager가 붙잡는다. Player는 이 세션을 weak_ptr로만 든다.
 */
class GameSession : public PacketSession
{
public:
	~GameSession()
	{
		cout << "~GameSession" << endl;
	}

	//~ PacketSession 인터페이스 구현
	virtual void OnConnected() override;
	/** 룸 퇴장과 저장은 끊긴 이유와 무관하게 여기서만 시작한다. C_LEAVE_GAME도 연결을 끊어 이 경로로 온다. */
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

	/** 플레이어가 룸에 있으면 끊길 때 진행을 저장한다. 저장 대기(SaveGate)를 걸지 정할 때 쓴다. */
	bool IsPlayerInRoom();

	/** 접속 종료한 플레이어를 룸에서 빼고, 룸이 저장할 데이터를 내주면 DB 큐에 저장을 넣는다. room의 큐 위에서만 부른다. */
	static void LeaveGame(shared_ptr<Room> room, shared_ptr<Player> player); // pch의 RoomRef·PlayerRef보다 먼저 읽힌다

public:
	atomic<shared_ptr<Player>> _player; // PlayerRef
    int64 _userId = 0; // GameSessionManager::RegisterUser가 채운다
};