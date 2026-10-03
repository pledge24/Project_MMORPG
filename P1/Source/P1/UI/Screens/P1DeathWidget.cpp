#include "UI/Screens/P1DeathWidget.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"
#include "Network/P1PacketSender.h"
#include "Core/P1GameInstance.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "UI/P1ScreenSubsystem.h"
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
            MyPlayerData->OnTownRespawnRejected.AddUObject(this, &UP1DeathWidget::HandleTownRespawnRejected);
        }
    }
}

void UP1DeathWidget::BindMyPlayerSpawned(AP1MyPlayer* MyPlayer)
{
    UE_LOG(LogP1UI, Log, TEXT("BindMyPlayerSpawned"));

    // 바인딩 셋업
    MyPlayer->OnDie.AddDynamic(this, &UP1DeathWidget::OnMyPlayerDie);
    MyPlayer->OnRespawn.AddDynamic(this, &UP1DeathWidget::OnMyPlayerRespawn);
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

void UP1DeathWidget::OnMyPlayerRespawn(AActor* RespawnedCreature)
{
    SetVisibility(ESlateVisibility::Collapsed);

    if (UWorld* World = GetWorld())
        World->GetTimerManager().ClearTimer(RetryTimerHandle);

    // 사망 중에 열려 있던 창이 있으면 UI 모드로 돌아가야 하므로 판단을 화면 서브시스템에 맡긴다.
    if (UP1ScreenSubsystem* Screens = ULocalPlayer::GetSubsystem<UP1ScreenSubsystem>(GetOwningLocalPlayer()))
    {
        Screens->RefreshInputMode();
    }
}

void UP1DeathWidget::StartCountdown()
{
    RemainingSeconds = TownRespawnDelaySeconds;
    ShowRemainingSeconds();

    if (UWorld* World = GetWorld())
        World->GetTimerManager().SetTimer(CountdownTimerHandle, this, &UP1DeathWidget::TickCountdown, CountdownIntervalSeconds, true);
}

void UP1DeathWidget::TickCountdown()
{
    // 실제 시각이 아니라 타이머가 울린 횟수로 센다. 블루프린트와 같다.
    --RemainingSeconds;
    ShowRemainingSeconds();

    if (RemainingSeconds > 0)
        return;

    if (UWorld* World = GetWorld())
        World->GetTimerManager().ClearTimer(CountdownTimerHandle);

    HandleCountdownFinished();
}

void UP1DeathWidget::HandleCountdownFinished()
{
    ReturnText->SetText(FText::FromString(TEXT("마을에서 리스폰하는 중...")));
    RequestTownRespawn();
}

void UP1DeathWidget::ShowRemainingSeconds()
{
    ReturnText->SetText(FText::FromString(FString::Printf(TEXT("%d초 뒤에 마을에서 리스폰합니다."), RemainingSeconds)));
}

void UP1DeathWidget::HandleTownRespawnRejected()
{
    // 살아 있는 동안 온 거절에 다시 요청하면 서버가 또 거절해 요청이 끝없이 오간다.
    if (GetVisibility() == ESlateVisibility::Collapsed)
        return;

    // 거절은 잘못된 상황이라 플레이어에게 사유를 알리지 않는다. 요청 중 문구를 둔 채 대기 시간 뒤에 다시 요청한다.
    // 리스폰하면 OnMyPlayerRespawn이 대기를 거둔다.
    if (UWorld* World = GetWorld())
        World->GetTimerManager().SetTimer(RetryTimerHandle, this, &UP1DeathWidget::RequestTownRespawn, TownRespawnDelaySeconds, false);
}

void UP1DeathWidget::RequestTownRespawn()
{
    Protocol::C_RESPAWN RespawnPkt;
    RespawnPkt.set_respawn_type(Protocol::RESPAWN_TYPE_TOWN);
    FP1PacketSender::Send(this, RespawnPkt);
}
