#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1EntryProgressWidget.generated.h"

class UTextBlock;
enum class EP1EntryProgress : uint8;

/**
 * 접속 진행 표시.
 *
 * 월드에 접속하거나 맵을 불러오는 동안 화면 전체를 덮어 진행 문구를 보여 준다.
 * 짧게 끝나는 진행은 띄우지 않고, 한 번 띄우면 정해진 시간 동안 남겼다가 서서히 사라진다.
 */
UCLASS()
class P1_API UP1EntryProgressWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeDestruct() override;
    //~ End UUserWidget Interface

    //~ Entry Progress
public:
    /**
     * 진행이 없으면 최소 표시 시간을 채운 뒤 서서히 숨기고, 있으면 그 진행의 문구로 화면을 덮는다.
     * 월드 접속은 SHOW_DELAY_SECONDS 안에 끝나면 띄우지 않는다. 맵 로드는 곧바로 덮는다.
     */
    void ShowProgress(EP1EntryProgress Progress);

private:
    void ShowNow(bool bFadeIn);
    void RequestHide();
    void BeginFadeOut();

    /** 지금의 불투명도에서 Target까지 FADE_SECONDS에 걸쳐 바꾸고, 다 바꾸면 OnFinished를 부른다. */
    void FadeTo(float Target, TFunction<void()> OnFinished);
    void StopFade();

    /** 진행이 이보다 빨리 끝나면 띄우지 않는다. 한 프레임만 번쩍이는 것을 막는다. */
    static constexpr float SHOW_DELAY_SECONDS = 0.3f;

    /** 한 번 띄우면 진행이 먼저 끝나도 이만큼은 남긴다. */
    static constexpr float MIN_VISIBLE_SECONDS = 0.8f;

    static constexpr float FADE_SECONDS = 0.25f;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> MessageText;

    FTimerHandle ShowDelayTimer;
    FTimerHandle HideDelayTimer;
    FTimerHandle FadeTimer;

    /** 화면을 덮고 있으면 true. 서서히 사라지는 동안에도 true. */
    bool bShown = false;

    /** 화면을 덮기 시작한 시각(FPlatformTime::Seconds). */
    double ShownAt = 0.0;

    float FadeFrom = 0.0f;
    float FadeTarget = 0.0f;

    /** 불투명도를 바꾸기 시작한 시각(FPlatformTime::Seconds). */
    double FadeStartedAt = 0.0;

    TFunction<void()> OnFadeFinished;
};
