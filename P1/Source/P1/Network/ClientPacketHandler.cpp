#include "Network/ClientPacketHandler.h"
#include "Network/PacketSession.h"
#include "Core/P1LoginMenuMode.h"
#include "UI/Frontend/P1LoginWidget.h"
#include "Core/P1LoginMenuPlayerController.h"
#include "Online/P1LoginManager.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Core/P1GameInstance.h"
#include "Sync/P1StatefulEntityManager.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Utils/LogCategory.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

namespace
{
    /** 세션이 속한 월드의 엔티티 관리자다. 월드가 없으면 nullptr. */
    UP1StatefulEntityManager* GetEntityManager(const PacketSessionRef& session)
    {
        if (UWorld* World = session->GetWorld())
            return World->GetSubsystem<UP1StatefulEntityManager>();

        return nullptr;
    }

    /** 세션을 연 게임 인스턴스의 내 플레이어 데이터다. 게임 인스턴스가 없으면 nullptr. */
    UP1MyPlayerData* GetMyPlayerData(const PacketSessionRef& session)
    {
        if (UWorld* World = session->GetWorld())
        {
            if (UGameInstance* GameInstance = World->GetGameInstance())
                return GameInstance->GetSubsystem<UP1MyPlayerData>();
        }

        return nullptr;
    }
}

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

bool Handle_S_PONG(PacketSessionRef& session, Protocol::S_PONG& pkt)
{
	return true;
}

bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt)
{
	if (auto* GameInstance = session->GetGameInstance())
	{
		if (AP1LoginMenuPlayerController* Controller = Cast<AP1LoginMenuPlayerController>(UGameplayStatics::GetPlayerController(GameInstance->GetWorld(), 0)))
		{
			if (UP1LoginManager* Manager = Controller->GetLoginManager())
			{
				if (UP1LoginWidget* LoginWidget = Manager->GetLoginWidget())
				{
                    LoginWidget->FetchCharacterOverviews(pkt);
                    return true;
				}
			}
		}
	}

	return false;
}

bool Handle_S_CREATE_CHARACTER(PacketSessionRef& session, Protocol::S_CREATE_CHARACTER& pkt) {
	
	if (auto* GameInstance = session->GetGameInstance())
	{
        if (AP1LoginMenuPlayerController* Controller = Cast<AP1LoginMenuPlayerController>(UGameplayStatics::GetPlayerController(GameInstance->GetWorld(), 0)))
		{
			if (UP1LoginManager* Manager = Controller->GetLoginManager())
			{
				if (UP1LoginWidget* LoginWidget = Manager->GetLoginWidget())
				{
                    LoginWidget->AddCharacterOverview(pkt);
                    return true;
				}
			}
		}
	}

	return false;
}

bool Handle_S_DELETE_CHARACTER(PacketSessionRef& session, Protocol::S_DELETE_CHARACTER& pkt) {

    if (auto* GameInstance = session->GetGameInstance())
    {
        if (AP1LoginMenuPlayerController* Controller = Cast<AP1LoginMenuPlayerController>(UGameplayStatics::GetPlayerController(GameInstance->GetWorld(), 0)))
        {
            if (UP1LoginManager* Manager = Controller->GetLoginManager())
            {
                if (UP1LoginWidget* LoginWidget = Manager->GetLoginWidget())
                {
                    LoginWidget->RemoveCharacterOverview(pkt);
                    return true;
                }
            }
        }
    }

	return false;
}

bool Handle_S_ENTER_GAME(PacketSessionRef& session, Protocol::S_ENTER_GAME& pkt)
{
    if (auto* GameInstance = session->GetGameInstance())
    {
        // 거절되면 아직 캐릭터 선택 화면이므로 그 화면에 알린다.
        if (pkt.success() == false)
        {
            if (AP1LoginMenuPlayerController* Controller = Cast<AP1LoginMenuPlayerController>(UGameplayStatics::GetPlayerController(GameInstance->GetWorld(), 0)))
            {
                if (UP1LoginManager* Manager = Controller->GetLoginManager())
                {
                    if (UP1LoginWidget* LoginWidget = Manager->GetLoginWidget())
                        LoginWidget->ShowEnterGameFailed();
                }
            }

            return true;
        }

        GameInstance->HandleEnterGame(pkt);
    }

	return true;
}

bool Handle_S_LEAVE_GAME(PacketSessionRef& session, Protocol::S_LEAVE_GAME& pkt)
{
    if (auto* GameInstance = session->GetGameInstance())
    {
        GameInstance->HandleLeaveGame(pkt);
    }

    return true;
}

bool Handle_S_ENTER_MAP(PacketSessionRef& session, Protocol::S_ENTER_MAP& pkt)
{
    if (auto* GameInstance = session->GetGameInstance())
    {
        GameInstance->HandleEnterMap(pkt);
        return true;
    }

    return false;
}

bool Handle_S_ENTER_ROOM(PacketSessionRef& session, Protocol::S_ENTER_ROOM& pkt)
{
    if (auto* GameInstance = session->GetGameInstance())
    {
        GameInstance->HandleEnterRoom(pkt);
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
    if (auto* EntityManager = GetEntityManager(session))
    {
        EntityManager->HandleNormalAttack(pkt);
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
    UP1StatefulEntityManager* EntityManager = GetEntityManager(session);
    UP1MyPlayerData* MyPlayerData = GetMyPlayerData(session);
    if (EntityManager == nullptr || MyPlayerData == nullptr)
        return false;

    // 외형은 모든 플레이어에 적용하고, 슬롯과 스탯은 내 플레이어일 때만 반영한다.
    EntityManager->HandleEquipGear(pkt);
    MyPlayerData->HandleEquipGear(pkt);
    return true;
}

bool Handle_S_UNEQUIP_GEAR(PacketSessionRef& session, Protocol::S_UNEQUIP_GEAR& pkt)
{
    UP1StatefulEntityManager* EntityManager = GetEntityManager(session);
    UP1MyPlayerData* MyPlayerData = GetMyPlayerData(session);
    if (EntityManager == nullptr || MyPlayerData == nullptr)
        return false;

    // 외형은 모든 플레이어에 적용하고, 슬롯과 스탯은 내 플레이어일 때만 반영한다.
    EntityManager->HandleUnequipGear(pkt);
    MyPlayerData->HandleUnequipGear(pkt);
    return true;
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

bool Handle_S_REWARD_RESULT(PacketSessionRef& session, Protocol::S_REWARD_RESULT& pkt)
{
    if (auto* GameInstance = session->GetGameInstance())
    {
        GameInstance->HandleRewardResult(pkt);
        return true;
    }

    return false;
}

bool Handle_S_RESPAWN(PacketSessionRef& session, Protocol::S_RESPAWN& pkt)
{
    if (auto* EntityManager = GetEntityManager(session))
    {
        EntityManager->HandleRespawn(pkt);
        return true;
    }

    return false;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
    /* 채팅 UI가 아직 없다. DummyClient 부하 테스트에서 서버 중계가 도는지
       확인할 수 있게 로그만 남긴다. */
    FString Msg = UTF8_TO_TCHAR(pkt.msg().c_str());
    UE_LOG(LogP1Network, Log, TEXT("S_CHAT entity_id: %lld, msg: %s"), pkt.entity_id(), *Msg);

    return true;
}

