// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/DeathWidget.h"
#include "P1.h"
#include "P1GameInstance.h"
#include "MyPlayerData.h"
#include "P1MyPlayer.h"

void UDeathWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance()))
    {
        if (UMyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UMyPlayerData>())
        {
            MyPlayerData->OnMyPlayerSpawned.AddUObject(this, &UDeathWidget::BindMyPlayerSpawned);
        }
    }
}

void UDeathWidget::BindMyPlayerSpawned(AP1MyPlayer* MyPlayer)
{
    UE_LOG(LogTemp, Log, TEXT("BindMyPlayerSpawned"));

    // 바인딩 셋업
    MyPlayer->OnDie.AddDynamic(this, &UDeathWidget::OnMyPlayerDie);
}

void UDeathWidget::OnMyPlayerDie(AActor* KilledCreature)
{
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
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

void UDeathWidget::SendRespawnInTownPacket()
{
    SendRespawnPacket(Protocol::RESPAWN_TYPE_TOWN);
}

void UDeathWidget::SendRespawnPacket(Protocol::RespawnType respawnType)
{
    Protocol::C_RESPAWN RespawnPkt; 
    {
        RespawnPkt.set_respawn_type(respawnType);
        SEND_PACKET(RespawnPkt);
    }
}

void UDeathWidget::Test()
{
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        SetVisibility(ESlateVisibility::SelfHitTestInvisible);

        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
        UE_LOG(LogTemp, Log, TEXT("?????"));
    }
}
