// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Protocol.pb.h"
#include "HUDWidget.generated.h"

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
    void UpdateAllHUDData(const Protocol::PlayerInfo& PlayerInfo_);
    void UpdateAllStatsChanged(const Protocol::StatInfo& StatInfo_);
    void UpdateCurLevel(int32 Level);
    void UpdateCurHp(int32 Hp);
    void UpdateCurMp(int32 Mp);
    void UpdateExp(int32 CurExp, int32 MaxExp);

protected:
    UPROPERTY(meta = (BindWidget))
    UTextBlock* Level_txt;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBarWidget> HpBar;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBarWidget> MpBar;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBarWidget> ExpBar;
};
