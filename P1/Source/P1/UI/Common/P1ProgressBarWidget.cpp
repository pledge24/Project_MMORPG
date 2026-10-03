#include "UI/Common/P1ProgressBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UP1ProgressBarWidget::Init(int64 CurValue, int64 MaxValue, bool IsPercentFormat)
{
    _CurValue = CurValue;
    _MaxValue = MaxValue;
    bIsPercentFormat = IsPercentFormat;

    UpdateBar();
}

void UP1ProgressBarWidget::SetCurValue(int64 Value)
{
    _CurValue = Value;
    UpdateBar();
}

void UP1ProgressBarWidget::SetMaxValue(int64 Value)
{
    _MaxValue = Value;
    UpdateBar();
}

void UP1ProgressBarWidget::UpdateBar()
{
    const float Percent = _MaxValue > 0 ? static_cast<float>(static_cast<double>(_CurValue) / _MaxValue) : 0.f;
    ProgressBar->SetPercent(Percent);

    FString ProgressText = !bIsPercentFormat ? FString::Printf(TEXT("%lld/%lld"), _CurValue, _MaxValue)
        : FString::Printf(TEXT("%.2f%%"), Percent * 100.f);

    SetProgressBarText(ProgressText);
}
