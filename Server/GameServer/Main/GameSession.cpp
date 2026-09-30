#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"
#include "ProgressPersistence.h"
#include "Player.h"
#include "Room.h"

void GameSession::OnConnected()
{
	GSessionManager.Add(static_pointer_cast<GameSession>(shared_from_this()));
}

// 룸 퇴장과 저장은 이유와 무관하게 여기서만 시작한다. C_LEAVE_GAME도 연결을 끊어 이 경로로 온다.
void GameSession::OnDisconnected()
{
	GSessionManager.Remove(static_pointer_cast<GameSession>(shared_from_this()));

	PlayerRef player = _player.load();
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

void GameSession::LeaveGame(RoomRef room, PlayerRef player)
{
	optional<PlayerSaveData> saveData = room->HandleDisconnect(player);
	if (saveData.has_value() == false)
		return;

	// 입장 불러오기와 같은 userId 큐에 넣는다. 곧바로 다시 접속해도 저장이 끝난 뒤에 불러온다.
	DBQueueRef dbQueue = GDBManager->GetDBQueueFromId(saveData->userId);
	dbQueue->Push(make_shared<Job>(
		[data = std::move(saveData.value())]()
		{
			ProgressPersistence::Save(data);
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
