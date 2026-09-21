#include "UI/Screens/P1DeathWidget.h"
#include "P1.h"
#include "Core/P1GameInstance.h"
#include "Core/P1MyPlayerData.h"
#include "Game/Entities/P1MyPlayer.h"
#include "Utils/LogCategory.h"

void UP1DeathWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (auto* GameInstance = GetP1GameInstance())
    {
        if (UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>())
        {
            MyPlayerData->OnMyPlayerSpawned.AddUObject(this, &UP1DeathWidget::BindMyPlayerSpawned);
        }
    }
}

void UP1DeathWidget::BindMyPlayerSpawned(AP1MyPlayer* MyPlayer)
{
    UE_LOG(LogP1UI, Log, TEXT("BindMyPlayerSpawned"));

    // 바인딩 셋업
    MyPlayer->OnDie.AddDynamic(this, &UP1DeathWidget::OnMyPlayerDie);
}

void UP1DeathWidget::OnMyPlayerDie(AActor* KilledCreature)
{
    if (APlayerController* PC = GetP1PlayerController())
    {
        SetVisibility(ESlateVisibility::SelfHitTestInvisible);

        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;

        // 카운트 다운 시작.
        StartCountdown();
    }

}

void UP1DeathWidget::SendRespawnInTownPacket()
{
    SendRespawnPacket(Protocol::RESPAWN_TYPE_TOWN);
}

void UP1DeathWidget::SendRespawnPacket(Protocol::RespawnType respawnType)
{
    Protocol::C_RESPAWN RespawnPkt; 
    {
        RespawnPkt.set_respawn_type(respawnType);
        SEND_PACKET(RespawnPkt);
    }
}

void UP1DeathWidget::Test()
{
    if (APlayerController* PC = GetP1PlayerController())
    {
        SetVisibility(ESlateVisibility::SelfHitTestInvisible);

        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
        UE_LOG(LogP1UI, Log, TEXT("?????"));
    }
}
