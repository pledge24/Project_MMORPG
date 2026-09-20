// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/P1ProgressBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UP1ProgressBarWidget::Init(int32 CurValue, int32 MaxValue, bool IsPercentFormat)
{
    _CurValue = CurValue;
    _MaxValue = MaxValue;
    bIsPercentFormat = IsPercentFormat;

    UpdateBar();
}

void UP1ProgressBarWidget::SetCurValue(int32 Value)
{
    _CurValue = Value;
    UpdateBar();
}

void UP1ProgressBarWidget::SetMaxValue(int32 Value)
{
    _MaxValue = Value;
    UpdateBar();
}

void UP1ProgressBarWidget::SetBoth(int32 CurValue, int32 MaxValue)
{
    _CurValue = CurValue;
    _MaxValue = MaxValue;

    UpdateBar();
}

void UP1ProgressBarWidget::UpdateBar()
{
    float Percent = _CurValue / (float)_MaxValue;
    ProgressBar->SetPercent(Percent);

    FString ProgressText = !bIsPercentFormat ? FString::Printf(TEXT("%d/%d"), _CurValue, _MaxValue)
        : FString::Printf(TEXT("%.2f%%"), Percent);;

    SetProgressBarText(ProgressText);
}
