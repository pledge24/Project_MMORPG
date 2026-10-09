#include "Core/pch.h"
#include "Network/GameSession.h"
#include "Network/GameSessionManager.h"
#include "Network/SaveGate.h"
#include "DB/ProgressStorage.h"
#include "DB/DAOCommon.h"
#include "Game/Entities/Player.h"
#include "Game/Room/Room.h"

void GameSession::OnDisconnected()
{
	PlayerRef player = _player.load();

	// 저장 대기는 UnregisterUser보다 먼저 건다. 반대로 하면 그 사이에 온 새 로그인이 대기 없이 입장해 저장 전의 진행을 불러온다.
	// 룸이 없으면 저장하지 않으므로 걸지 않는다. 걸면 풀어 줄 저장이 없어 다음 입장이 만료까지 막힌다.
	// 룸 입장 잡이 큐에 남아 있다가 저장하는 경우는 LeaveGame이 건다.
	if (IsPlayerInRoom())
		GSaveGate.Hold(_userId);

	GSessionManager.UnregisterUser(static_pointer_cast<GameSession>(shared_from_this()));

	if (player == nullptr)
		return;

	// 표시를 먼저 쓰고 룸을 읽는다. 순서는 Room::EnterPlayer의 주석을 본다.
	player->MarkDisconnected();

	// 룸에 들어간 적이 없으면 이 세션에서 바뀐 것이 없으므로 저장하지 않는다.
	// 불러오기가 도중에 실패했다면 절반만 채워진 상태를 덮어쓰게 된다.
	RoomRef room = player->GetRoom();
	if (room == nullptr)
		return;

	room->DoAsync([room, player]()
		{
			GameSession::LeaveGame(room, player);
		});
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef self = GetPacketSessionRef();

	// 핸들러가 거절했거나 본문을 풀지 못하면 false다. 버리면 프로토콜이 어긋나도 원인이 남지 않는다.
	// PacketSession::OnRecv가 헤더 크기 이상만 넘기므로 헤더는 읽을 수 있다.
	if (ServerPacketHandler::HandlePacket(self, buffer, len) == false)
	{
		const PacketHeader* header = reinterpret_cast<const PacketHeader*>(buffer);
		GLogger->Warning("패킷 {} 처리에 실패했다(길이 {}, 계정 {})", header->id, len, GetUserId());
	}
}

void GameSession::OnSend(int32 len)
{
}

bool GameSession::TryRegisterPlayer(const PlayerRef& player)
{
	PlayerRef expected = nullptr;
	return _player.compare_exchange_strong(expected, player);
}

bool GameSession::IsPlayerInRoom()
{
	PlayerRef player = _player.load();
	return player != nullptr && player->GetRoom() != nullptr;
}

void GameSession::LeaveGame(RoomRef room, PlayerRef player)
{
	optional<PlayerSaveData> saveData = room->HandleDisconnect(player);
	if (saveData.has_value() == false)
		return;

	// 끊길 때 룸이 없어 OnDisconnected가 대기를 걸지 않은 경우(룸 입장 잡이 큐에 남아 있던 경우)를 여기서 덮는다.
	// 이미 걸려 있으면 아무것도 하지 않는다.
	GSaveGate.Hold(saveData->userId);

	// 입장 불러오기와 같은 userId 큐에 넣는다. 그사이 맡겨 둔 불러오기는 저장이 끝난 뒤 이 잡에서 실행한다.
	DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(saveData->userId);
	dbQueue->Push(make_shared<Job>(
		[data = std::move(saveData.value())]()
		{
			// 연결을 빌리지 못해도 아래에서 대기를 풀어야 한다. 풀지 않으면 다음 입장이 상한까지 막힌다.
			try
			{
				DBConnectionGuard conn;
				ProgressStorage::Save(*conn, data);
			}
			catch (const exception& error)
			{
				GLogger->Error("계정 {} 접속 종료 저장 실패: {}", data.userId, error.what());
			}

			// 저장이 실패해도 대기를 푼다. 실패한 저장은 다시 시도하지 않으므로 기다려도 결과가 같다.
			if (optional<SaveGate::ParkedLoad> parked = GSaveGate.Release(data.userId))
				parked->run();
		}));
}
