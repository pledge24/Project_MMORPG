#pragma once
#include "Session.h"

class Player;
class Room;

class GameSession : public PacketSession
{
public:
	~GameSession()
	{
		cout << "~GameSession" << endl;
	}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

	// 접속 종료한 플레이어를 룸에서 빼고 진행을 저장한다. room의 큐 위에서만 부른다.
	static void LeaveGame(shared_ptr<Room> room, shared_ptr<Player> player); // pch의 RoomRef·PlayerRef보다 먼저 읽힌다

public:
	atomic<shared_ptr<Player>> _player; // PlayerRef
    int64 _userId;
};