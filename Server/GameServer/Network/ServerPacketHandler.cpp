#include "Core/pch.h"
#include "Network/ServerPacketHandler.h"
#include "Game/Entities/Player.h"
#include "Game/Room/Room.h"
#include "Network/ItemRequests.h"
#include "Network/ProgressCoordinator.h"
#include "Lobby/LobbyReturn.h"

namespace
{
    // 보낸 세션의 플레이어가 속한 룸의 큐에 job(room, player)을 넣는다. 플레이어나 룸이 없으면 넣지 않고 false.
    // 보낸 사람은 패킷 안의 id가 아니라 세션의 플레이어로 정한다. 패킷 안의 id를 믿으면 다른 플레이어 행세를 할 수 있다.
    template<typename RoomJob>
    bool DispatchToPlayerRoom(const PacketSessionRef& session, RoomJob&& job)
    {
        PlayerRef player = static_pointer_cast<GameSession>(session)->GetPlayer();
        if (player == nullptr)
            return false;

        RoomRef room = player->GetRoom();
        if (room == nullptr)
            return false;

        room->DoAsync([room, player, job = std::forward<RoomJob>(job)]()
            {
                job(room, player);
            });

        return true;
    }
}

// 로비 패킷(로비 접속, 캐릭터 생성, 삭제, 선택)의 핸들러는 Lobby/LobbyPacketHandlers.cpp에 있다.
// 세션 단계는 GameSession::OnRecvPacket이 확인하므로, 여기 오는 패킷은 그 패킷을 받는 단계의 세션이 보낸 것이다.
// 핸들러는 모두 IOCP 워커 스레드에서 불린다. C_ 패킷의 값은 클라이언트가 보낸 것이므로 전부 검증 대상이다.
// 룸 소유 상태는 여기서 건드리지 않는다. 룸 일은 room->DoAsync로 넘기고 바로 리턴한다.
// 여기서 룸 상태를 건드리면 락 없이 도는 룸 큐와 경합한다.
PacketHandlerFunc GPacketHandler[UINT16_MAX];

// false를 돌려주면 GameSession::OnRecvPacket이 패킷 id와 함께 로그를 남긴다.
bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

bool Handle_C_PING(PacketSessionRef& session, Protocol::C_PING& pkt)
{
	return false;
}

bool Handle_C_LEAVE_GAME(PacketSessionRef& session, Protocol::C_LEAVE_GAME& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    // 룸 퇴장과 저장은 GameSession::OnDisconnected가 한다. 비정상 종료와 같은 경로를 탄다.
    gameSession->Disconnect("Exit Game");

    return true;
}

bool Handle_C_MAP_LOAD_COMPLETE(PacketSessionRef& session, Protocol::C_MAP_LOAD_COMPLETE& pkt)
{
    // 맵 로드를 기다리는 플레이어의 소속 룸은 목적지 룸이다. 첫 입장도 게임 입장이 목적지 룸을 소속 룸으로 정해 둔다.
    return DispatchToPlayerRoom(session, [](const RoomRef& room, const PlayerRef& player)
        {
            room->C_HandleMapLoadComplete(player);
        });
}

bool Handle_C_USE_PORTAL(PacketSessionRef& session, Protocol::C_USE_PORTAL& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            room->C_HandleUsePortal(pkt, player);
        });
}

bool Handle_C_MOVE(PacketSessionRef& session, Protocol::C_MOVE& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            room->C_HandleMove(pkt, player);
        });
}

bool Handle_C_CHAT(PacketSessionRef& session, Protocol::C_CHAT& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            // 맵 로드를 기다리는 플레이어는 소속 룸에 아직 없다. 그 룸에 중계하면 들어가지 않은 룸에 말이 남는다.
            if (room->Contains(player->GetEntityId()) == false)
                return;

            // 같은 룸의 모든 플레이어에게 그대로 중계한다(본인 포함).
            Protocol::S_CHAT chatPkt;
            chatPkt.set_entity_id(player->GetEntityId());
            chatPkt.set_msg(pkt.msg());

            room->Broadcast(ServerPacketHandler::MakeSerializedPacket(chatPkt));
        });
}

bool Handle_C_NORMAL_ATTACK(PacketSessionRef& session, Protocol::C_NORMAL_ATTACK& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            room->C_HandleNormalAttack(pkt, player);
        });
}

bool Handle_C_BUY_ITEM(PacketSessionRef& session, Protocol::C_BUY_ITEM& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            ItemRequests::HandleBuyItem(*room, player, pkt);
        });
}

bool Handle_C_SELL_ITEM(PacketSessionRef& session, Protocol::C_SELL_ITEM& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            ItemRequests::HandleSellItem(*room, player, pkt);
        });
}

bool Handle_C_EQUIP_GEAR(PacketSessionRef& session, Protocol::C_EQUIP_GEAR& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            ItemRequests::HandleEquipGear(*room, player, pkt);
        });
}


bool Handle_C_UNEQUIP_GEAR(PacketSessionRef& session, Protocol::C_UNEQUIP_GEAR& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            ItemRequests::HandleUnequipGear(*room, player, pkt);
        });
}

bool Handle_C_USE_ITEM(PacketSessionRef& session, Protocol::C_USE_ITEM& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            ItemRequests::HandleUseItem(*room, player, pkt);
        });
}

bool Handle_C_SET_QUICK_SLOT(PacketSessionRef& session, Protocol::C_SET_QUICK_SLOT& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            ItemRequests::HandleSetQuickSlot(*room, player, pkt);
        });
}

bool Handle_C_USE_QUICK_SLOT(PacketSessionRef& session, Protocol::C_USE_QUICK_SLOT& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            ItemRequests::HandleUseQuickSlot(*room, player, pkt);
        });
}

bool Handle_C_RESPAWN(PacketSessionRef& session, Protocol::C_RESPAWN& pkt)
{
    return DispatchToPlayerRoom(session, [pkt](const RoomRef& room, const PlayerRef& player)
        {
            room->C_HandleRespawn(pkt, player);
        });
}

bool Handle_C_RETURN_TO_LOBBY(PacketSessionRef& session, Protocol::C_RETURN_TO_LOBBY& pkt)
{
    GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

    // 계정 번호는 여기서 한 번 읽어 잡에 넘긴다. 잡이 세션에서 다시 읽으면 그사이 바뀐 계정으로 일한다.
    const int64 userId = gameSession->GetUserId();
    PlayerRef player = gameSession->GetPlayer();
    if (player == nullptr)
        return false;

    // 받는 순간 단계를 바꾼다. 저장이 끝날 때까지 게임 요청도 로비 요청도 받지 않고, 두 번째 복귀도 여기서 막는다.
    if (gameSession->BeginLobbyReturn() == false)
        return false;

    // 접속 종료와 같은 절차로 룸에서 퇴장하고 저장한다. 목록은 저장이 끝난 뒤 로비 복귀 창구가 보낸다.
    GProgressCoordinator->OnReturnToLobby(userId, player, [gameSession, userId]()
        {
            LobbyReturn::Accept(gameSession, userId);
        });

    return true;
}
