#include "UI/Loading/P1LoadingScreenSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "HAL/IConsoleManager.h"
#include "UI/P1UISettings.h"
#include "UI/Loading/P1EntryProgressWidget.h"
#include "Utils/LogCategory.h"

namespace
{
    /** 로컬 서버에서는 진행이 너무 빨리 끝나 사라지는 모습을 볼 수 없어서, 확인할 때 표시 시간을 늘린다. */
    TAutoConsoleVariable<float> CVarExtraHoldSeconds(
        TEXT("p1.LoadingScreen.ExtraHoldSeconds"),
        0.0f,
        TEXT("로딩 화면을 최소 표시 시간보다 이만큼 더 남긴다(초). 페이드를 눈으로 확인할 때 쓴다."),
        ECVF_Cheat);

    /** 프레임이 한 번 크게 밀려도 페이드가 한 번에 끝나지 않게 한 프레임에 반영할 시간을 자른다. */
    constexpr float MAX_FADE_STEP_SECONDS = 1.0f / 30.0f;
}

void UP1LoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 진행 구독. 접속 흐름이 먼저 초기화돼 있어야 한다.
    if (UP1EntryFlowSubsystem* EntryFlow = Collection.InitializeDependency<UP1EntryFlowSubsystem>())
        EntryFlow->OnProgressChanged.AddUObject(this, &UP1LoadingScreenSubsystem::HandleProgressChanged);
}

void UP1LoadingScreenSubsystem::Deinitialize()
{
    if (UP1EntryFlowSubsystem* EntryFlow = GetGameInstance()->GetSubsystem<UP1EntryFlowSubsystem>())
        EntryFlow->RemoveListener(this);

    if (TickerHandle.IsValid())
    {
        FTSTicker::RemoveTicker(TickerHandle);
        TickerHandle.Reset();
    }

    Hide();
    Super::Deinitialize();
}

void UP1LoadingScreenSubsystem::HandleProgressChanged(EP1EntryProgress NewProgress)
{
    if (Progress == EP1EntryProgress::None)
        ProgressStartedAt = FPlatformTime::Seconds();

    Progress = NewProgress;

    if (Progress != EP1EntryProgress::None && ViewportContent.IsValid())
        Widget->SetProgress(Progress);

    // 맵을 여는 동안은 화면이 멈춰 페이드가 돌지 않는다. 멈추기 전의 마지막 프레임에 이미 덮여 있어야 하므로 기다리지 않는다.
    // 월드 접속만 레벨을 열지 않으므로 지연 표시를 쓴다. 로비 복귀는 저장을 기다린 뒤 같은 진행으로 로그인 맵을 연다.
    if (Progress != EP1EntryProgress::None && Progress != EP1EntryProgress::ConnectingWorld && ViewportContent.IsValid() == false)
        Show(false);

    // 띄우기 지연, 페이드, 숨기기는 모두 틱이 한다.
    StartTicking();
}

bool UP1LoadingScreenSubsystem::Show(bool bFadeIn)
{
    UGameInstance* GameInstance = GetGameInstance();
    UGameViewportClient* ViewportClient = GameInstance->GetGameViewportClient();
    if (ViewportClient == nullptr)
        return false;

    if (Widget == nullptr)
    {
        const TSubclassOf<UP1EntryProgressWidget> WidgetClass = GetDefault<UP1UISettings>()->LoadingScreenWidgetClass.LoadSynchronous();
        if (WidgetClass == nullptr)
        {
            UE_LOG(LogP1UI, Warning, TEXT("로딩 화면 위젯 클래스가 프로젝트 설정(P1 UI)에 없어 로딩 화면을 띄우지 않는다"));
            return false;
        }

        // 레벨이 아니라 게임 인스턴스가 갖게 해야 맵이 바뀌어도 위젯이 남는다.
        Widget = CreateWidget<UP1EntryProgressWidget>(GameInstance, WidgetClass);
    }

    Widget->SetProgress(Progress);
    Opacity = bFadeIn ? 0.0f : 1.0f;
    Widget->SetRenderOpacity(Opacity);

    // 덮는 동안 아래 화면의 버튼이 눌리지 않게 입력을 받는다.
    Widget->SetVisibility(ESlateVisibility::Visible);

    // AddToViewport로 붙인 위젯은 월드가 정리될 때 함께 내려간다. 뷰포트에 직접 붙인 Slate 위젯은 LoadMap이 건드리지 않는다.
    ViewportContent = Widget->TakeWidget();
    ViewportClient->AddViewportWidgetContent(ViewportContent.ToSharedRef(), VIEWPORT_Z_ORDER);
    ShownAt = FPlatformTime::Seconds();
    return true;
}

void UP1LoadingScreenSubsystem::Hide()
{
    if (ViewportContent.IsValid() == false)
        return;

    if (UGameViewportClient* ViewportClient = GetGameInstance()->GetGameViewportClient())
        ViewportClient->RemoveViewportWidgetContent(ViewportContent.ToSharedRef());

    ViewportContent.Reset();
}

bool UP1LoadingScreenSubsystem::Tick(float DeltaTime)
{
    const double Now = FPlatformTime::Seconds();

    if (ViewportContent.IsValid() == false)
    {
        // 띄우기 전에 끝난 진행이다.
        if (Progress == EP1EntryProgress::None)
        {
            TickerHandle.Reset();
            return false;
        }

        if (Now - ProgressStartedAt < SHOW_DELAY_SECONDS)
            return true;

        if (Show(true) == false)
        {
            TickerHandle.Reset();
            return false;
        }

        return true;
    }

    // 진행이 끝나도 최소 표시 시간을 채우기 전에는 덮어 둔다. 숨기려던 중에 진행이 다시 오면 다시 덮는다.
    const double HoldSeconds = MIN_VISIBLE_SECONDS + FMath::Max(0.0f, CVarExtraHoldSeconds.GetValueOnGameThread());
    const bool bKeepShown = Progress != EP1EntryProgress::None || Now - ShownAt < HoldSeconds;

    const float Step = FMath::Min(DeltaTime, MAX_FADE_STEP_SECONDS) / FADE_SECONDS;
    Opacity = bKeepShown ? FMath::Min(1.0f, Opacity + Step) : FMath::Max(0.0f, Opacity - Step);
    Widget->SetRenderOpacity(Opacity);

    if (bKeepShown == false && Opacity <= 0.0f)
    {
        Hide();
        TickerHandle.Reset();
        return false;
    }

    return true;
}

void UP1LoadingScreenSubsystem::StartTicking()
{
    if (TickerHandle.IsValid())
        return;

    // 위젯은 Tick을 끄고, 레벨이 바뀌면 월드 타이머도 사라진다. 그래서 레벨과 상관없이 도는 코어 티커로 잰다.
    TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UP1LoadingScreenSubsystem::Tick));
}
