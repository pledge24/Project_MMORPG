#include "Network/ClientPacketHandler.h"
#include "Network/PacketSession.h"
#include "Network/P1ConnectionSubsystem.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Sync/P1StatefulEntityManager.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Game/Chat/P1ChatSubsystem.h"
#include "Game/Channel/P1ChannelSubsystem.h"
#include "Utils/LogCategory.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

namespace
{
    /** 세션이 속한 월드의 엔티티 관리자. 월드가 없으면 nullptr. */
    UP1StatefulEntityManager* GetEntityManager(const PacketSessionRef& session)
    {
        if (UWorld* World = session->GetWorld())
            return World->GetSubsystem<UP1StatefulEntityManager>();

        return nullptr;
    }

    /** 세션을 연 게임 인스턴스의 내 플레이어 데이터. 월드나 게임 인스턴스가 없으면 nullptr. */
    UP1MyPlayerData* GetMyPlayerData(const PacketSessionRef& session)
    {
        if (UWorld* World = session->GetWorld())
        {
            if (UGameInstance* GameInstance = World->GetGameInstance())
                return GameInstance->GetSubsystem<UP1MyPlayerData>();
        }

        return nullptr;
    }

    /** 세션을 연 게임 인스턴스의 채팅 서브시스템. 월드나 게임 인스턴스가 없으면 nullptr. */
    UP1ChatSubsystem* GetChat(const PacketSessionRef& session)
    {
        if (UWorld* World = session->GetWorld())
        {
            if (UGameInstance* GameInstance = World->GetGameInstance())
                return GameInstance->GetSubsystem<UP1ChatSubsystem>();
        }

        return nullptr;
    }

    /** 세션을 연 게임 인스턴스의 채널 서브시스템. 월드나 게임 인스턴스가 없으면 nullptr. */
    UP1ChannelSubsystem* GetChannels(const PacketSessionRef& session)
    {
        if (UWorld* World = session->GetWorld())
        {
            if (UGameInstance* GameInstance = World->GetGameInstance())
                return GameInstance->GetSubsystem<UP1ChannelSubsystem>();
        }

        return nullptr;
    }
}

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

// 로비 계열(S_CONNECT_LOBBY, S_CHARACTER_LIST, S_CREATE_CHARACTER, S_DELETE_CHARACTER, S_ENTER_GAME)의 정의는
// Lobby/P1LobbyPacketHandlers.cpp에 있다. Network/는 Lobby/를 부르지 않는다.

bool Handle_S_DISCONNECT(PacketSessionRef& session, Protocol::S_DISCONNECT& pkt)
{
    if (auto* Connection = session->GetConnection())
    {
        Connection->HandleDisconnect(pkt.reason());
        return true;
    }

    return false;
}

bool Handle_S_ENTER_ROOM(PacketSessionRef& session, Protocol::S_ENTER_ROOM& pkt)
{
    UP1StatefulEntityManager* EntityManager = GetEntityManager(session);
    UP1MyPlayerData* MyPlayerData = GetMyPlayerData(session);
    if (EntityManager == nullptr || MyPlayerData == nullptr)
        return false;

    // 내 엔티티 번호는 내 플레이어 데이터가, 엔티티 정리와 내 플레이어 배치는 엔티티 관리자가, 지금 채널은 채널 서브시스템이 맡는다.
    // 엔티티 관리자가 그 번호로 내 플레이어를 가리므로 내 플레이어 데이터가 먼저다.
    MyPlayerData->HandleEnterRoom(pkt);
    EntityManager->HandleEnterRoom(pkt);
    if (UP1ChannelSubsystem* Channels = GetChannels(session))
        Channels->HandleEnterRoom(pkt);
    return true;
}

bool Handle_S_USE_PORTAL(PacketSessionRef& session, Protocol::S_USE_PORTAL& pkt)
{
    if (auto* MyPlayerData = GetMyPlayerData(session))
    {
        MyPlayerData->HandleUsePortal(pkt);
        return true;
    }

    return false;
}

bool Handle_S_SPAWN(PacketSessionRef& session, Protocol::S_SPAWN& pkt)
{
    if (auto* EntityManager = GetEntityManager(session))
    {
        EntityManager->HandleSpawn(pkt);
        return true;
    }

    return false;
}

bool Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt)
{
    if (auto* EntityManager = GetEntityManager(session))
    {
        EntityManager->HandleDespawn(pkt);
        return true;
    }

    return false;
}

bool Handle_S_MOVE(PacketSessionRef& session, Protocol::S_MOVE& pkt)
{
    if (auto* EntityManager = GetEntityManager(session))
    {
        EntityManager->HandleMove(pkt);
        return true;
    }

    return false;
}

bool Handle_S_NORMAL_ATTACK(PacketSessionRef& session, Protocol::S_NORMAL_ATTACK& pkt)
{
    // 내 공격은 누를 때 이미 재생했다. 거절되어도 되돌릴 연출이 없으므로 남기기만 한다.
    if (pkt.result() != Protocol::COMBAT_RESULT_SUCCESS)
        UE_LOG(LogP1Network, Warning, TEXT("일반 공격이 거절됐다: %s"), UTF8_TO_TCHAR(Protocol::CombatResult_Name(pkt.result()).c_str()));

    return true;
}

bool Handle_S_ATTACK(PacketSessionRef& session, Protocol::S_ATTACK& pkt)
{
    if (auto* EntityManager = GetEntityManager(session))
    {
        EntityManager->HandleAttack(pkt);
        return true;
    }

    return false;
}

bool Handle_S_HIT(PacketSessionRef& session, Protocol::S_HIT& pkt)
{
    if (auto* EntityManager = GetEntityManager(session))
    {
        EntityManager->HandleHit(pkt);
        return true;
    }

    return false;
}

bool Handle_S_BUY_ITEM(PacketSessionRef& session, Protocol::S_BUY_ITEM& pkt)
{
    if (auto* MyPlayerData = GetMyPlayerData(session))
    {
        MyPlayerData->HandleBuyItem(pkt);
        return true;
    }

    return false;
}

bool Handle_S_SELL_ITEM(PacketSessionRef& session, Protocol::S_SELL_ITEM& pkt)
{
    if (auto* MyPlayerData = GetMyPlayerData(session))
    {
        MyPlayerData->HandleSellItem(pkt);
        return true;
    }

    return false;
}

bool Handle_S_EQUIP_GEAR(PacketSessionRef& session, Protocol::S_EQUIP_GEAR& pkt)
{
    // 외형은 뒤따르는 S_GEAR_APPEARANCE가 바꾼다.
    if (auto* MyPlayerData = GetMyPlayerData(session))
    {
        MyPlayerData->HandleEquipGear(pkt);
        return true;
    }

    return false;
}

bool Handle_S_UNEQUIP_GEAR(PacketSessionRef& session, Protocol::S_UNEQUIP_GEAR& pkt)
{
    // 외형은 뒤따르는 S_GEAR_APPEARANCE가 바꾼다.
    if (auto* MyPlayerData = GetMyPlayerData(session))
    {
        MyPlayerData->HandleUnequipGear(pkt);
        return true;
    }

    return false;
}

bool Handle_S_GEAR_APPEARANCE(PacketSessionRef& session, Protocol::S_GEAR_APPEARANCE& pkt)
{
    if (auto* EntityManager = GetEntityManager(session))
    {
        EntityManager->HandleGearAppearance(pkt);
        return true;
    }

    return false;
}

bool Handle_S_SET_QUICK_SLOT(PacketSessionRef& session, Protocol::S_SET_QUICK_SLOT& pkt)
{
    if (auto* MyPlayerData = GetMyPlayerData(session))
    {
        MyPlayerData->HandleSetQuickSlot(pkt);
        return true;
    }

    return false;
}

bool Handle_S_USE_ITEM(PacketSessionRef& session, Protocol::S_USE_ITEM& pkt)
{
    if (auto* MyPlayerData = GetMyPlayerData(session))
    {
        MyPlayerData->HandleUseItem(pkt);
        return true;
    }

    return false;
}

bool Handle_S_DIE(PacketSessionRef& session, Protocol::S_DIE& pkt)
{
    if (auto* EntityManager = GetEntityManager(session))
    {
        EntityManager->HandleDie(pkt);
        return true;
    }

    return false;
}

bool Handle_S_REWARD(PacketSessionRef& session, Protocol::S_REWARD& pkt)
{
    if (auto* MyPlayerData = GetMyPlayerData(session))
    {
        MyPlayerData->HandleReward(pkt);
        return true;
    }

    return false;
}

bool Handle_S_RESPAWN(PacketSessionRef& session, Protocol::S_RESPAWN& pkt)
{
    if (auto* MyPlayerData = GetMyPlayerData(session))
    {
        MyPlayerData->HandleRespawn(pkt);
        return true;
    }

    return false;
}

bool Handle_S_CHANNEL_LIST(PacketSessionRef& session, Protocol::S_CHANNEL_LIST& pkt)
{
    UP1ChannelSubsystem* Channels = GetChannels(session);
    if (Channels == nullptr)
        return false;

    Channels->HandleChannelList(pkt);
    return true;
}

bool Handle_S_CHANGE_CHANNEL(PacketSessionRef& session, Protocol::S_CHANGE_CHANNEL& pkt)
{
    UP1ChannelSubsystem* Channels = GetChannels(session);
    if (Channels == nullptr)
        return false;

    Channels->HandleChangeChannel(pkt);
    return true;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
    UP1ChatSubsystem* Chat = GetChat(session);
    if (Chat == nullptr)
        return false;

    Chat->HandleChatResult(pkt);
    return true;
}

bool Handle_S_CHAT_MESSAGE(PacketSessionRef& session, Protocol::S_CHAT_MESSAGE& pkt)
{
    UP1ChatSubsystem* Chat = GetChat(session);
    UP1MyPlayerData* MyPlayerData = GetMyPlayerData(session);
    if (Chat == nullptr || MyPlayerData == nullptr)
        return false;

    Chat->HandleChatMessage(pkt, MyPlayerData->GetPlayerId());

    // 말풍선은 엔티티 관리자가 띄운다. 맵을 바꾸는 중이라 관리자가 없어도 채팅창 기록은 남긴다.
    if (UP1StatefulEntityManager* EntityManager = GetEntityManager(session))
        EntityManager->HandleChatMessage(pkt);

    return true;
}

