#include "Core/pch.h"
#include "Network/GameSession.h"
#include "Network/GameSessionManager.h"
#include "Network/ProgressCoordinator.h"

void GameSession::OnDisconnected()
{
	// 조율자가 저장 대기를 건 뒤에 계정 등록을 지운다. 순서의 이유는 ProgressCoordinator::OnDisconnected에 있다.
	GProgressCoordinator->OnDisconnected(_userId, _player.load());

	GSessionManager->UnregisterUser(static_pointer_cast<GameSession>(shared_from_this()));
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
