#include "ClientPacketHandler.h"
#include "LoginMenuMode.h"
#include "LoginWidget.h"
#include "LoginMenuPlayerController.h"
#include "LoginManager.h"
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
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Handle_S_ENTER_GAME")));
	
    // 1. S_ENTER_GAME을 분해해서 MyPlayer에 저장한다.
    // 2. 로그인 레벨을 언로드(Unload)한다.
    // 3. 다음 레벨을 연다.
    
    if (GWorld)
    {
        UGameplayStatics::OpenLevel(GWorld, FName("DevMap"));
        if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
        {
            GameInstance->HandleSpawn(pkt);
        }
    }

	return true;
}

bool Handle_S_LEAVE_GAME(PacketSessionRef& session, Protocol::S_LEAVE_GAME& pkt)
{
	if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
	{
		// TODO: 게임 종료? 로비로?(연결을 곧바로 끊을지 선택해야함)

	}

	return true;
}

bool Handle_S_SPAWN(PacketSessionRef& session, Protocol::S_SPAWN& pkt)
{
	if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleSpawn(pkt);
	}

	return true;
}

bool Handle_S_DESPAWN(PacketSessionRef& session, Protocol::S_DESPAWN& pkt)
{
	if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleDespawn(pkt);
	}

	return true;
}

bool Handle_S_MOVE(PacketSessionRef& session, Protocol::S_MOVE& pkt)
{
	if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
	{
		GameInstance->HandleMove(pkt);
	}

	return false;
}

bool Handle_S_CHAT(PacketSessionRef& session, Protocol::S_CHAT& pkt)
{
	auto Msg = pkt.msg();


	return true;
}

