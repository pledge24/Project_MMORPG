#include "UI/Common/P1ProgressBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

namespace
{
    // 블루프린트의 Make Slate Brush 기본값과 같은 브러시다. 32x32 이미지, 타일 없음, 흰색 틴트.
    FSlateBrush MakeBarBrush(UTexture2D* Texture)
    {
        FSlateBrush Brush;
        Brush.SetResourceObject(Texture);
        Brush.ImageSize = FVector2D(32.f, 32.f);
        Brush.DrawAs = ESlateBrushDrawType::Image;
        Brush.Tiling = ESlateBrushTileType::NoTile;
        Brush.Mirroring = ESlateBrushMirrorType::NoMirror;
        Brush.Margin = FMargin(0.f);
        Brush.TintColor = FSlateColor(FLinearColor::White);
        return Brush;
    }
}

void UP1ProgressBarWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    if (ProgressBar)
    {
        FProgressBarStyle Style;
        Style.SetBackgroundImage(MakeBarBrush(BackgroundTexture));
        Style.SetFillImage(MakeBarBrush(FillTexture));
        Style.SetEnableFillAnimation(false);
        ProgressBar->SetWidgetStyle(Style);
    }

    if (TextBlock)
        TextBlock->SetText(PlaceholderText);

    if (GridImage)
        GridImage->SetBrushFromTexture(GridTexture, false);
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
    const float Percent = _MaxValue > 0 ? static_cast<float>(static_cast<double>(_CurValue) / _MaxValue) : 0.f;
    ProgressBar->SetPercent(Percent);

    FString ProgressText = !bIsPercentFormat ? FString::Printf(TEXT("%lld/%lld"), _CurValue, _MaxValue)
        : FString::Printf(TEXT("%.2f%%"), Percent * 100.f);

    if (TextBlock)
        TextBlock->SetText(FText::FromString(ProgressText));
}
