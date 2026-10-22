#include "Core/pch.h"
#include "Network/GameSession.h"
#include "Network/GameSessionManager.h"
#include "Network/ProgressCoordinator.h"

void GameSession::OnDisconnected()
{
	// 조율자가 저장 대기를 건 뒤에 계정 등록을 지운다. 반대로 하면 그사이 같은 계정의 새 로그인이 대기 없이 입장해 저장 전의 진행을 불러온다.
	GProgressCoordinator->OnDisconnected(_userId, _player.load());

	GSessionManager->UnregisterUser(static_pointer_cast<GameSession>(shared_from_this()));
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef self = GetPacketSessionRef();

	// 단계에 맞지 않는 패킷은 핸들러에 넘기지 않는다. 헤더보다 짧은 입력은 HandlePacket이 헤더를 읽기 전에 거절한다.
	if (len >= static_cast<int32>(sizeof(PacketHeader)))
	{
		const PacketHeader* header = reinterpret_cast<const PacketHeader*>(buffer);
		const SessionPhase phase = GetPhase();
		if (SessionPhaseTable::Accepts(header->id, phase) == false)
		{
			GLogger->Warning("{} 단계의 세션이 받지 않는 패킷 {}을 버렸다(계정 {})", ToString(phase), header->id, GetUserId());
			return;
		}
	}

	// 핸들러가 거절했거나 본문을 풀지 못한 패킷이다. 로그 없이 버리면 프로토콜이 어긋나도 원인이 남지 않는다.
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

bool GameSession::CompleteLobbyConnect()
{
	return TryChangePhase(SessionPhase::BEFORE_LOBBY, SessionPhase::LOBBY);
}

bool GameSession::BeginGameEntry()
{
	return TryChangePhase(SessionPhase::LOBBY, SessionPhase::ENTERING_GAME);
}

bool GameSession::CompleteGameEntry()
{
	return TryChangePhase(SessionPhase::ENTERING_GAME, SessionPhase::IN_GAME);
}

bool GameSession::CancelGameEntry()
{
	return TryChangePhase(SessionPhase::ENTERING_GAME, SessionPhase::LOBBY);
}

bool GameSession::TryChangePhase(SessionPhase from, SessionPhase to)
{
	// 단계는 IOCP 스레드(핸들러)와 DB 스레드(로비 접속, 입장 잡)가 함께 바꾼다. 확인과 변경을 나누면 둘 다 같은 단계를 보고 넘어간다.
	SessionPhase expected = from;
	return _phase.compare_exchange_strong(expected, to);
}

bool GameSession::TryRegisterPlayer(const PlayerRef& player)
{
	// 같은 세션의 입장 잡 둘이 서로 다른 DB 큐에서 동시에 돌 수 있다. 확인과 등록을 나누면 둘 다 빈 세션을 보고,
	// 먼저 등록된 플레이어가 덮여 룸에 남은 채 끊길 때 퇴장하지 않는다.
	PlayerRef expected = nullptr;
	return _player.compare_exchange_strong(expected, player);
}
