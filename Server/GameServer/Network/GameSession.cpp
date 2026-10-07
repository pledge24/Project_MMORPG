#include "Core/pch.h"
#include "Network/GameSession.h"
#include "Network/GameSessionManager.h"
#include "Network/SaveGate.h"
#include "Network/ServerPacketHandler.h"
#include "DB/ProgressStorage.h"
#include "Game/Entities/Player.h"
#include "Game/Room/Room.h"

void GameSession::OnConnected()
{
	GSessionManager.Add(static_pointer_cast<GameSession>(shared_from_this()));
}

// 룸 퇴장과 저장은 이유와 무관하게 여기서만 시작한다. C_LEAVE_GAME도 연결을 끊어 이 경로로 온다.
void GameSession::OnDisconnected()
{
	PlayerRef player = _player.load();

	// 저장 대기는 Remove보다 먼저 건다. 반대로 하면 그 사이에 온 새 로그인이 대기 없이 입장해 저장 전의 진행을 불러온다.
	// 룸이 없으면 저장하지 않으므로 걸지 않는다. 걸면 풀어 줄 저장이 없어 다음 입장이 만료까지 막힌다.
	// 룸 입장 잡이 큐에 남아 있다가 저장하는 경우는 LeaveGame이 건다.
	if (IsPlayerInRoom())
		GSaveGate.Hold(_userId);

	GSessionManager.Remove(static_pointer_cast<GameSession>(shared_from_this()));

	if (player == nullptr)
		return;

	// 표시를 먼저 쓰고 룸을 읽는다. 순서는 Room::EnterPlayer의 주석을 본다.
	player->_disconnected.store(true);

	// 룸에 들어간 적이 없으면 이 세션에서 바뀐 것이 없으므로 저장하지 않는다.
	// 불러오기가 도중에 실패했다면 절반만 채워진 상태를 덮어쓰게 된다.
	RoomRef room = player->_room.load().lock();
	if (room == nullptr)
		return;

	room->DoAsync([room, player]()
		{
			GameSession::LeaveGame(room, player);
		});
}

bool GameSession::IsPlayerInRoom()
{
	PlayerRef player = _player.load();
	return player != nullptr && player->_room.load().lock() != nullptr;
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
			ProgressStorage::Save(data);

			// 저장이 실패해도 대기를 푼다. 실패한 저장은 다시 시도하지 않으므로 기다려도 결과가 같다.
			if (optional<SaveGate::ParkedLoad> parked = GSaveGate.Release(data.userId))
				parked->run();
		}));
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();

	// 게임 서버가 아닌 다른 서버(ex. DB 서버)에 넘겨줄때 id 대역 체크용
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
	// TODO: packetId 대역 체크...

	ServerPacketHandler::HandlePacket(session, buffer, len);
}

void GameSession::OnSend(int32 len)
{
}
