#include "UI/Frontend/P1EntryProgressWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Core/P1EntryFlowSubsystem.h"

namespace
{
    /** 서서히 바꾸는 동안 불투명도를 고치는 간격. 60Hz면 눈에 끊겨 보이지 않는다. */
    constexpr float FADE_STEP_SECONDS = 1.0f / 60.0f;
}

void UP1EntryProgressWidget::NativeDestruct()
{
    // 로그인 맵이 닫히면 월드와 함께 타이머도 사라지지만, 같은 월드에서 위젯만 내려가는 경우를 위해 먼저 끊는다.
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ShowDelayTimer);
        World->GetTimerManager().ClearTimer(HideDelayTimer);
        World->GetTimerManager().ClearTimer(FadeTimer);
    }

    Super::NativeDestruct();
}

void UP1EntryProgressWidget::ShowProgress(EP1EntryProgress Progress)
{
    switch (Progress)
    {
    case EP1EntryProgress::ConnectingWorld:
        MessageText->SetText(FText::FromString(TEXT("월드에 접속하는 중…")));
        break;
    case EP1EntryProgress::LoadingMap:
        MessageText->SetText(FText::FromString(TEXT("맵을 불러오는 중…")));
        break;
    case EP1EntryProgress::None:
        RequestHide();
        return;
    }

    UWorld* World = GetWorld();
    if (World == nullptr)
        return;

    // 숨기려던 중이면 그만두고 다시 덮는다.
    World->GetTimerManager().ClearTimer(HideDelayTimer);
    if (bShown)
    {
        FadeTo(1.0f, nullptr);
        return;
    }

    // 다음 틱에 레벨을 동기로 열면 그동안 화면이 멈춘다. 멈추기 전의 마지막 프레임에 덮여 있어야 하므로 기다리지 않는다.
    if (Progress == EP1EntryProgress::LoadingMap)
    {
        World->GetTimerManager().ClearTimer(ShowDelayTimer);
        ShowNow(false);
        return;
    }

    if (World->GetTimerManager().IsTimerActive(ShowDelayTimer) == false)
    {
        World->GetTimerManager().SetTimer(ShowDelayTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            ShowNow(true);
        }), SHOW_DELAY_SECONDS, false);
    }
}

void UP1EntryProgressWidget::ShowNow(bool bFadeIn)
{
    bShown = true;
    ShownAt = FPlatformTime::Seconds();

    // 덮는 동안 아래 화면의 버튼이 눌리지 않게 입력을 받는다.
    SetVisibility(ESlateVisibility::Visible);

    if (bFadeIn)
    {
        SetRenderOpacity(0.0f);
        FadeTo(1.0f, nullptr);
        return;
    }

    StopFade();
    SetRenderOpacity(1.0f);
}

void UP1EntryProgressWidget::RequestHide()
{
    UWorld* World = GetWorld();
    if (World)
        World->GetTimerManager().ClearTimer(ShowDelayTimer);

    // 띄우기 전에 끝난 진행이다.
    if (bShown == false)
    {
        StopFade();
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    const double Remaining = MIN_VISIBLE_SECONDS - (FPlatformTime::Seconds() - ShownAt);
    if (Remaining <= 0.0 || World == nullptr)
    {
        BeginFadeOut();
        return;
    }

    World->GetTimerManager().SetTimer(HideDelayTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
    {
        BeginFadeOut();
    }), static_cast<float>(Remaining), false);
}

void UP1EntryProgressWidget::BeginFadeOut()
{
    FadeTo(0.0f, [this]()
    {
        bShown = false;
        SetVisibility(ESlateVisibility::Collapsed);
    });
}

void UP1EntryProgressWidget::FadeTo(float Target, TFunction<void()> OnFinished)
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        SetRenderOpacity(Target);
        if (OnFinished)
            OnFinished();
        return;
    }

    FadeFrom = GetRenderOpacity();
    FadeTarget = Target;
    FadeStartedAt = FPlatformTime::Seconds();
    OnFadeFinished = MoveTemp(OnFinished);

    // 위젯의 틱은 꺼 두므로 타이머로 불투명도를 고친다. 경과 시간은 실제 시각으로 재서 프레임이 밀려도 길이가 같다.
    World->GetTimerManager().SetTimer(FadeTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
    {
        const float Alpha = FMath::Clamp(static_cast<float>((FPlatformTime::Seconds() - FadeStartedAt) / FADE_SECONDS), 0.0f, 1.0f);
        SetRenderOpacity(FMath::Lerp(FadeFrom, FadeTarget, Alpha));
        if (Alpha < 1.0f)
            return;

        // StopFade가 완료 함수를 비우므로 먼저 꺼내 둔다.
        TFunction<void()> Finished = MoveTemp(OnFadeFinished);
        StopFade();
        if (Finished)
            Finished();
    }), FADE_STEP_SECONDS, true);
}

void UP1EntryProgressWidget::StopFade()
{
    if (UWorld* World = GetWorld())
        World->GetTimerManager().ClearTimer(FadeTimer);

    OnFadeFinished = nullptr;
}
