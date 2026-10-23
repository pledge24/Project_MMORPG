#include "UI/Screens/P1DeathWidget.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"
#include "Network/P1PacketSender.h"
#include "Core/P1GameInstance.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "UI/P1ScreenSubsystem.h"
#include "Game/Entities/P1MyPlayer.h"
#include "Utils/LogCategory.h"

void UP1DeathWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // 사망 화면이 떠 있는 동안 입력을 이 화면만 받으려면 키 포커스를 받을 수 있어야 한다.
    SetIsFocusable(true);
}

void UP1DeathWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 내 플레이어의 스폰과 마을 리스폰 거절을 구독.
    if (auto* GameInstance = GetP1GameInstance())
    {
        if (UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>())
        {
            MyPlayerData->OnMyPlayerSpawned.AddUObject(this, &UP1DeathWidget::BindMyPlayerSpawned);
            MyPlayerData->OnTownRespawnRejected.AddUObject(this, &UP1DeathWidget::HandleTownRespawnRejected);
        }
    }
}

FReply UP1DeathWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // 사망 화면이 떠 있으면 입력이 플레이어 컨트롤러에 가지 않으므로, 사망한 채 로비로 돌아갈 게임 메뉴는 이 화면이 연다.
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (UP1ScreenSubsystem* Screens = ULocalPlayer::GetSubsystem<UP1ScreenSubsystem>(GetOwningLocalPlayer()))
        {
            Screens->ToggleGameMenu();
            return FReply::Handled();
        }
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UP1DeathWidget::BindMyPlayerSpawned(AP1MyPlayer* MyPlayer)
{
    UE_LOG(LogP1UI, Log, TEXT("BindMyPlayerSpawned"));

    MyPlayer->OnDie.AddDynamic(this, &UP1DeathWidget::OnMyPlayerDie);
    MyPlayer->OnRespawn.AddDynamic(this, &UP1DeathWidget::OnMyPlayerRespawn);
}

void UP1DeathWidget::OnMyPlayerDie(AActor* KilledCreature)
{
    if (GetP1PlayerController())
    {
        // 사망 화면을 띄우고 입력을 이 화면으로 돌린다. 게임 메뉴가 열려 있으면 메뉴가 계속 입력을 받는다.
        SetVisibility(ESlateVisibility::SelfHitTestInvisible);

        if (UP1ScreenSubsystem* Screens = ULocalPlayer::GetSubsystem<UP1ScreenSubsystem>(GetOwningLocalPlayer()))
            Screens->RefreshInputMode();

        // 카운트 다운 시작.
        StartCountdown();
    }

}

void UP1DeathWidget::OnMyPlayerRespawn(AActor* RespawnedCreature)
{
    SetVisibility(ESlateVisibility::Collapsed);

    if (UWorld* World = GetWorld())
        World->GetTimerManager().ClearTimer(RetryTimerHandle);

    // 사망 중에 열려 있던 창이나 게임 메뉴가 있으면 그에 맞는 입력 모드로 돌아가야 하므로 판단을 화면 서브시스템에 맡긴다.
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
    // 실제 시각이 아니라 타이머가 울린 횟수로 센다.
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
