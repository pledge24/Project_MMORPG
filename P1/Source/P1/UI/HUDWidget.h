// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Protocol.pb.h"
#include "HUDWidget.generated.h"

class AP1MyPlayer;
class UTextBlock;
class UProgressBarWidget;

/**
 * 
 */
UCLASS()
class P1_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

public:
    void UpdateLevel(int32 Value);

    /** HP Bar*/
    void UpdateMaxHp(int64 Value);
    void UpdateCurHp(int64 Value);
    void UpdateHpBar(TOptional<int64> CurValue, TOptional<int64> MaxValue);

    /** MP Bar*/
    void UpdateMaxMp(int64 Value);
    void UpdateCurMp(int64 Value);
    void UpdateMpBar(TOptional<int64> CurValue, TOptional<int64> MaxValue);

    /** EXP Bar*/
    void UpdateMaxExp(int64 Value);
    void UpdateCurExp(int64 Value);
    void UpdateExpBar(TOptional<int64> CurValue, TOptional<int64> MaxValue);

    void SetBattleModeTxt(bool battleMode);

protected:
    UPROPERTY(meta = (BindWidget))
    UTextBlock* Name_txt;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Level_txt;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* BattleMode_txt;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBarWidget> HpBar;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBarWidget> MpBar;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBarWidget> ExpBar;
};
