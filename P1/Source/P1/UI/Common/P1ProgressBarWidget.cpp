#include "UI/Common/P1ProgressBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UP1ProgressBarWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    // 브러시의 크기와 그리기 방식은 WBP 디자이너의 막대 스타일이 정한다. 여기서는 막대마다 다른 텍스처만 끼운다.
    if (ProgressBar)
    {
        FProgressBarStyle Style = ProgressBar->GetWidgetStyle();
        Style.BackgroundImage.SetResourceObject(BackgroundTexture);
        Style.FillImage.SetResourceObject(FillTexture);
        ProgressBar->SetWidgetStyle(Style);
    }

    if (GridImage)
        GridImage->SetBrushFromTexture(GridTexture, false);

    // 위젯 컴포넌트에 붙은 막대는 값이 먼저 들어온 뒤에 슬레이트 위젯이 만들어져 이 함수가 다시 불린다.
    // 그때 자리 문구로 덮지 않고 들어온 값을 다시 적는다.
    if (bHasValue)
        UpdateBar();
    else if (TextBlock)
        TextBlock->SetText(PlaceholderText);
}

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
    bHasValue = true;

    const float Percent = _MaxValue > 0 ? static_cast<float>(static_cast<double>(_CurValue) / _MaxValue) : 0.f;
    ProgressBar->SetPercent(Percent);

    FString ProgressText = !bIsPercentFormat ? FString::Printf(TEXT("%lld/%lld"), _CurValue, _MaxValue)
        : FString::Printf(TEXT("%.2f%%"), Percent * 100.f);

    if (TextBlock)
        TextBlock->SetText(FText::FromString(ProgressText));
}
