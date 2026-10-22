#include "Core/pch.h"
#include "Network/ServerPacketHandler.h"
#include "Game/Entities/Player.h"
#include "Game/Room/Room.h"
#include "Network/ItemRequests.h"

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

// TODO: 클라이언트 맵 로드 완료 처리. Protocol.proto에 선언만 되어 있어
// 생성기가 만드는 핸들러 선언을 채우기 위한 스텁이다.
bool Handle_C_MAP_LOAD_COMPLETE(PacketSessionRef& session, Protocol::C_MAP_LOAD_COMPLETE& pkt)
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

bool Handle_C_ENTER_MAP(PacketSessionRef& session, Protocol::C_ENTER_MAP& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->GetPlayer();
    int32 roomId = pkt.room_id();

    if (player == nullptr)
    {
        // 플레이어가 없으면 잡을 올릴 룸 큐도 없다. 룸 소유 상태를 건드리지 않는
        // 실패 응답만 이 자리에서 돌려준다.
        Protocol::S_ENTER_MAP enterMapPkt;
        {
            enterMapPkt.set_success(false);
            enterMapPkt.set_map_id(pkt.map_id());
            enterMapPkt.set_room_id(roomId);

            SendPacket(session, enterMapPkt);
        }

        return false;
    }

    // 플레이어 상태를 소유한 룸의 큐로 넘긴다. 아직 어떤 룸에도 속하지 않았다면 요청한 룸의 큐로 넘긴다.
    // 그 경우 판정은 불러온 룸과 같은지만 보므로, 다른 룸을 요청하면 그 룸의 큐에서 거절된다.
    RoomRef room = player->GetRoom();
    if (room == nullptr)
        room = GRoomManager->FindRoom(roomId);

    if (room == nullptr)
    {
        // 넘길 큐가 없어도 실패 응답은 보낸다. 보내지 않으면 클라이언트가 응답을 기다리며 멈춘다.
        GLogger->Warning("C_ENTER_MAP을 넘길 Room을 찾지 못함. roomId: {}", roomId);

        Protocol::S_ENTER_MAP enterMapPkt;
        {
            enterMapPkt.set_success(false);
            enterMapPkt.set_map_id(pkt.map_id());
            enterMapPkt.set_room_id(roomId);

            SendPacket(session, enterMapPkt);
        }

        return false;
    }

    // 여기서 기록한 enteringRoomId를 뒤이어 C_HandleEnterRoom이 같은 큐에서 읽는다.
    room->DoAsync(&Room::C_HandleEnterMap, pkt, player);

    return true;
}

bool Handle_C_ENTER_ROOM(PacketSessionRef& session, Protocol::C_ENTER_ROOM& pkt)
{
    auto gameSession = static_pointer_cast<GameSession>(session);

    PlayerRef player = gameSession->GetPlayer();
    if (player == nullptr)
        return false;

    RoomRef curRoom = player->GetRoom();
    if (curRoom == nullptr)
    {
        // 최초 입장. 클라이언트가 무엇을 보냈든 서버가 INITIAL로 판정하고 입장할 룸의 큐로 넘긴다.
        pkt.set_enter_type(Protocol::ENTER_TYPE_INITIAL);

        int32 roomId = pkt.has_room_id() ? pkt.room_id() : player->GetEnteringRoomId();
        RoomRef enterRoom = GRoomManager->FindRoom(roomId);
        if (enterRoom == nullptr)
        {
            GLogger->Warning("최초 입장할 Room을 찾지 못함. roomId: {}", roomId);
            return false;
        }

        enterRoom->DoAsync(&Room::C_HandleEnterRoom, pkt, player);

        return true;
    }

    curRoom->DoAsync(&Room::C_HandleEnterRoom, pkt, player);

    return true;
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

