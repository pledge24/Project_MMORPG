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
    void Init(int32 CurValue, int32 _MaxValue, bool IsPercentFormat = false);
    void SetMaxValue(int32 Value) { MaxValue = Value; };

    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateBar(int32 Value, bool IsPercentFormat = false);

    int32 MaxValue = 100;

protected:
    UPROPERTY(BlueprintReadOnly , meta = (BindWidget), Category = "UI")
    class UProgressBar* ProgressBar;
	
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "UI")
    class UTextBlock* TextBlock;
};
