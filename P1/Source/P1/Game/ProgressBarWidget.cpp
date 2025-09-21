// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/ProgressBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UProgressBarWidget::Init(int32 CurValue, int32 MaxValue, bool IsPercentFormat)
{
    _CurValue = CurValue;
    _MaxValue = MaxValue;
    bIsPercentFormat = IsPercentFormat;

    UpdateBar();
}

void UProgressBarWidget::SetCurValue(int32 Value)
{
    _CurValue = Value;
    UpdateBar();
}

void UProgressBarWidget::SetMaxValue(int32 Value)
{
    _MaxValue = Value;
    UpdateBar();
}

void UProgressBarWidget::SetBoth(int32 CurValue, int32 MaxValue)
{
    _CurValue = CurValue;
    _MaxValue = MaxValue;

    UpdateBar();
}

void UProgressBarWidget::UpdateBar()
{
    float Percent = _CurValue / (float)_MaxValue;
    ProgressBar->SetPercent(Percent);

    FString ProgressText = !bIsPercentFormat ? FString::Printf(TEXT("%d/%d"), _CurValue, _MaxValue)
        : FString::Printf(TEXT("%.2f%%"), Percent);;

    TextBlock->SetText(FText::FromString(ProgressText));
}
