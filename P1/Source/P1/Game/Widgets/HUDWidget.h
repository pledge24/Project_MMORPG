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

    void BindMyPlayerSpawned(AP1MyPlayer* MyPlayer);

public:
    void UpdateAllStatsChanged(const Protocol::StatInfo& StatInfo_);
    void UpdateLevel(int32 Value);

    /** HP Bar*/
    void UpdateMaxHp(int32 Value);
    void UpdateCurHp(int32 Value);
    void UpdateHpBar(TOptional<int32> CurValue, TOptional<int32> MaxValue);

    /** MP Bar*/
    void UpdateMaxMp(int32 Value);
    void UpdateCurMp(int32 Value);
    void UpdateMpBar(TOptional<int32> CurValue, TOptional<int32> MaxValue);

    /** EXP Bar*/
    void UpdateMaxExp(int32 Value);
    void UpdateCurExp(int32 Value);
    void UpdateExpBar(TOptional<int32> CurValue, TOptional<int32> MaxValue);

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
