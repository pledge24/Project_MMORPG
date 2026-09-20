// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProgressBarWidget.generated.h"

/**
 * 
 */
UCLASS()
class P1_API UProgressBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    void Init(int32 CurValue, int32 MaxValue, bool IsPercentFormat = false);

    void SetCurValue(int32 Value);
    void SetMaxValue(int32 Value);
    void SetBoth(int32 CurValue, int32 MaxValue);

protected:
    UFUNCTION(BlueprintImplementableEvent, Category="ProgressBar")
    void SetProgressBarText(const FString& ProgressText);

    void UpdateBar();

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category="ProgressBar")
    class UProgressBar* ProgressBar;

    int32 _CurValue;
    int32 _MaxValue;
    bool bIsPercentFormat = false;
};
