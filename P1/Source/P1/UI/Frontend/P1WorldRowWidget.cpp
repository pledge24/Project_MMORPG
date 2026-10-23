#include "UI/Frontend/P1WorldRowWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Auth/P1World.h"

void UP1WorldRowWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    RowButton->OnClicked.AddDynamic(this, &UP1WorldRowWidget::OnRowButtonClicked);
    ApplyState();
}

void UP1WorldRowWidget::ShowWorld(const FP1World& World)
{
    WorldId = World.WorldId;
    Congestion = World.Congestion;
    NameText->SetText(FText::FromString(World.Name));
    ApplyState();
}

void UP1WorldRowWidget::SetHighlighted(bool bInHighlighted)
{
    bHighlighted = bInHighlighted;
    ApplyState();
}

void UP1WorldRowWidget::ApplyState()
{
    const auto ShowIf = [](UWidget* Widget, bool bShow)
    {
        Widget->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    };

    ShowIf(SelectedBackground, bHighlighted);
    ShowIf(NormalBackground, bHighlighted == false);

    // 혼잡도는 고를 수 있는지와 무관하다. 혼잡한 월드도 같은 버튼으로 고른다.
    ShowIf(SmoothBadge, Congestion == EP1WorldCongestion::Smooth);
    ShowIf(NormalBadge, Congestion == EP1WorldCongestion::Normal);
    ShowIf(CrowdedBadge, Congestion == EP1WorldCongestion::Crowded);
}

void UP1WorldRowWidget::OnRowButtonClicked()
{
    OnRowClicked.ExecuteIfBound(WorldId);
}
