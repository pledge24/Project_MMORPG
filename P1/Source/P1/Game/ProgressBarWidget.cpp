// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/ProgressBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UProgressBarWidget::Init(int32 CurValue, int32 _MaxValue, bool IsPercentFormat)
{
    SetMaxValue(_MaxValue);
    UpdateBar(CurValue, IsPercentFormat);
}

void UProgressBarWidget::UpdateBar(int32 Value, bool IsPercentFormat)
{
    float Percent = Value / (float)MaxValue;
    ProgressBar->SetPercent(Percent);

    FString ProgressText = !IsPercentFormat ? FString::Printf(TEXT("%d/%d"), Value, MaxValue)
        : FString::Printf(TEXT("%f%%"), Percent);

    TextBlock->SetText(FText::FromString(ProgressText));
}
