#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "Core/P1EntryFlowSubsystem.h"
#include "P1LoadingScreenSubsystem.generated.h"

class SWidget;
class UP1EntryProgressWidget;

/**
 * 로딩 화면 관리자.
 *
 * 접속 흐름의 진행(EP1EntryProgress)을 받아 접속 진행 표시를 게임 뷰포트 맨 위에 덮는다. 위젯을 레벨이 아니라 게임
 * 인스턴스에 두고 뷰포트에 직접 붙이므로, 맵을 바꾸는 동안에도 사라지지 않고 새 맵에서 내 플레이어가 스폰될 때까지 남는다.
 * 짧게 끝나는 월드 접속은 띄우지 않고, 한 번 띄우면 정해진 시간 동안 남겼다가 서서히 사라진다.
 * 모든 멤버 함수는 게임 스레드 전용이다.
 */
UCLASS()
class P1_API UP1LoadingScreenSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

    //~ Begin USubsystem Interface
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

    //~ Loading Screen
private:
    void HandleProgressChanged(EP1EntryProgress NewProgress);

    /** 위젯을 뷰포트에 붙인다. bFadeIn이 아니면 곧바로 불투명하게 덮는다. 위젯을 만들지 못하면 false. */
    bool Show(bool bFadeIn);
    void Hide();

    /** 띄우기를 기다리는 동안과 띄워 둔 동안 매 프레임 불린다. 더 돌 필요가 없으면 false. */
    bool Tick(float DeltaTime);
    void StartTicking();

    /** 월드 접속이 이보다 빨리 끝나면 띄우지 않는다. 한 프레임만 번쩍이는 것을 막는다. */
    static constexpr double SHOW_DELAY_SECONDS = 0.3;

    /** 한 번 띄우면 진행이 먼저 끝나도 이만큼은 남긴다. */
    static constexpr double MIN_VISIBLE_SECONDS = 0.8;

    static constexpr float FADE_SECONDS = 0.25f;

    /** 다른 화면보다 위에 그린다. 인게임 화면과 창은 이보다 낮은 순서로 붙는다. */
    static constexpr int32 VIEWPORT_Z_ORDER = 10000;

    UPROPERTY()
    TObjectPtr<UP1EntryProgressWidget> Widget;

    /** 뷰포트에 붙인 위젯. 붙이지 않았으면 비어 있다. */
    TSharedPtr<SWidget> ViewportContent;

    FTSTicker::FDelegateHandle TickerHandle;

    /** 지금 접속 흐름이 알린 진행. */
    EP1EntryProgress Progress = EP1EntryProgress::None;

    /** 진행이 시작된 시각(FPlatformTime::Seconds). 띄우기 지연을 잰다. */
    double ProgressStartedAt = 0.0;

    /** 화면을 덮기 시작한 시각(FPlatformTime::Seconds). 최소 표시 시간을 잰다. */
    double ShownAt = 0.0;

    float Opacity = 0.0f;
};
