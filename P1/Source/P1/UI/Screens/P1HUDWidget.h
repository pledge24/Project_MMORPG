#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "P1HUDWidget.generated.h"

class AP1MyPlayer;
class UTextBlock;
class UP1ProgressBarWidget;

/**
 * 게임 화면에 늘 떠 있는 내 플레이어의 상태 표시.
 *
 * 이름, 레벨, HP·MP·경험치, 전투 모드, 지금 채널처럼 플레이 중에 계속 봐야 하는 내 정보를 보여 준다.
 */
UCLASS()
class P1_API UP1HUDWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    //~ End UUserWidget Interface

    //~ Identity
protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Name_txt;

    //~ Level
public:
    void UpdateLevel(int32 Value);

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Level_txt;

    //~ HP Bar
public:
    void UpdateMaxHp(int64 Value);
    void UpdateCurHp(int64 Value);

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UP1ProgressBarWidget> HpBar;

    //~ MP Bar
public:
    void UpdateMaxMp(int64 Value);
    void UpdateCurMp(int64 Value);

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UP1ProgressBarWidget> MpBar;

    //~ EXP Bar
public:
    void UpdateMaxExp(int64 Value);
    void UpdateCurExp(int64 Value);

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UP1ProgressBarWidget> ExpBar;

    //~ Battle Mode
public:
    void SetBattleModeTxt(bool battleMode);

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> BattleMode_txt;

    //~ Channel
public:
    /** 0이면 아직 룸에 들어가지 않은 것이라 비운다. */
    void UpdateChannel(int32 Channel);

protected:
    /** WBP에 없으면 채널을 보이지 않는다. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Channel_txt;

    //~ Interaction Prompt
public:
    /** 조준한 NPC로 할 수 있는 일. 빈 문구면 내린다. */
    void SetInteractionPrompt(const FText& Prompt);

protected:
    /** 화면 중앙 아래의 상호작용 표시. WBP에 없으면 보이지 않는다. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> InteractionPrompt_txt;
};
