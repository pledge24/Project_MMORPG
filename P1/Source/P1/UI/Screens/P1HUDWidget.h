#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Protocol.pb.h"
#include "P1HUDWidget.generated.h"

class AP1MyPlayer;
class UTextBlock;
class UP1ProgressBarWidget;

UCLASS()
class P1_API UP1HUDWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
protected:
    virtual void NativeConstruct() override;
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
};
