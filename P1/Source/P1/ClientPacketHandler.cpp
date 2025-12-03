#include "ClientPacketHandler.h"
#include "LoginMenuMode.h"
#include "LoginWidget.h"
#include "LoginMenuPlayerController.h"
#include "LoginManager.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "P1.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	return false;
}

bool Handle_S_PONG(PacketSessionRef& session, Protocol::S_PONG& pkt)
{
	return false;
}

bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt)
{
	if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
	{
		if (ALoginMenuPlayerController* Controller = Cast<ALoginMenuPlayerController>(UGameplayStatics::GetPlayerController(GameInstance->GetWorld(), 0)))
		{
			if (ULoginManager* Manager = Controller->GetLoginManager())
			{
				if (ULoginWidget* LoginWidget = Manager->GetLoginWidget())
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
	
	if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
	{
        if (ALoginMenuPlayerController* Controller = Cast<ALoginMenuPlayerController>(UGameplayStatics::GetPlayerController(GameInstance->GetWorld(), 0)))
		{
			if (ULoginManager* Manager = Controller->GetLoginManager())
			{
				if (ULoginWidget* LoginWidget = Manager->GetLoginWidget())
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

    if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
    {
        if (ALoginMenuPlayerController* Controller = Cast<ALoginMenuPlayerController>(UGameplayStatics::GetPlayerController(GameInstance->GetWorld(), 0)))
        {
            if (ULoginManager* Manager = Controller->GetLoginManager())
            {
                if (ULoginWidget* LoginWidget = Manager->GetLoginWidget())
                {
                    LoginWidget->RemoveCharacterOverview(pkt);
                    return true;
                }
            }
        }
    }

	return true;
}

bool Handle_S_ENTER_GAME(PacketSessionRef& session, Protocol::S_ENTER_GAME& pkt)
{
    if (GWorld)
    {
        if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
        {
            GameInstance->HandleEnterGame(pkt);
            UGameplayStatics::OpenLevel(GWorld, FName("InGameMap"));
        }
    }

	return true;
}

bool Handle_S_MOVE_ROOM(PacketSessionRef& session, Protocol::S_MOVE_ROOM& pkt)
{
    if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
    {
        GameInstance->HandleMove(pkt.info());   // Teleporting My Character
        GameInstance->HandleDespawnAll(true);

        for (auto& Object : pkt.objects())
        {
            GameInstance->HandleSpawn(Object);
        }

        return true;
    }

    return false;
}

bool Handle_S_LEAVE_GAME(PacketSessionRef& session, Protocol::S_LEAVE_GAME& pkt)
{
	if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
	{
		// 연결을 곧바로 끊음
        if (FSocket* Socket = GameInstance->Socket)
        {
        	//ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get();
        	//SocketSubsystem->DestroySocket(Socket);
        	//Socket = nullptr;

            Socket->Close();
        }
	}

	return true;
}

bool Handle_S_SPAWN(PacketSessionRef& session, Protocol::S_SPAWN& pkt)
{
	if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleSpawn(pkt);
        return true;
	}

	return false;
}

bool Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt)
{
	if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleDespawn(pkt);
        return true;
	}

	return false;
}

bool Handle_S_MOVE(PacketSessionRef& session, Protocol::S_MOVE& pkt)
{
	if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleMove(pkt);
        return true;
	}

	return false;
}

bool Handle_S_NORMAL_ATTACK(PacketSessionRef& session, Protocol::S_NORMAL_ATTACK& pkt)
{
    if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
    {
        GameInstance->HandleNormalAttack(pkt);
        return true;
    }

    return false;
}

bool Handle_S_HIT(PacketSessionRef& session, Protocol::S_HIT& pkt)
{
    if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
    {
        GameInstance->HandleHit(pkt);
        return true;
    }

    return false;
}

bool Handle_S_BUY_ITEM(PacketSessionRef& session, Protocol::S_BUY_ITEM& pkt)
{
    if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
    {
        GameInstance->HandleBuyItem(pkt);
        return true;
    }

    return false;
}

bool Handle_S_SELL_ITEM(PacketSessionRef& session, Protocol::S_SELL_ITEM& pkt)
{
    if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
    {
        GameInstance->HandleSellItem(pkt);
        return true;
    }

    return false;
}

bool Handle_S_EQUIP_GEAR(PacketSessionRef& session, Protocol::S_EQUIP_GEAR& pkt)
{
    if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
    {
        GameInstance->HandleEquipGear(pkt);
        return true;
    }

    return false;
}

bool Handle_S_UNEQUIP_GEAR(PacketSessionRef& session, Protocol::S_UNEQUIP_GEAR& pkt)
{
    if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
    {
        GameInstance->HandleUnequipGear(pkt);
        return true;
    }

    return false;
}

bool Handle_S_USE_ITEM(PacketSessionRef& session, Protocol::S_USE_ITEM& pkt)
{
    if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
    {
        GameInstance->HandleUseItem(pkt);
        return true;
    }

    return false;
}

bool Handle_S_DIE(PacketSessionRef& session, Protocol::S_DIE& pkt)
{
    if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
    {
        GameInstance->HandleDie(pkt);
        return true;
    }

    return false;
}

bool Handle_S_MONSTER_KILL_RESULT(PacketSessionRef& session, Protocol::S_MONSTER_KILL_RESULT& pkt)
{
    if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
    {
        GameInstance->HandleMonsterKillResult(pkt);
        return true;
    }

    return false;
}

bool Handle_S_RETURN_BY_DEATH(PacketSessionRef& session, Protocol::S_RETURN_BY_DEATH& pkt)
{
    if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
    {
        GameInstance->HandleReturnByDeath(pkt);
        return true;
    }

    return false;
}
