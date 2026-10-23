#include "UI/WorldSpace/P1ChatBubbleWidget.h"
#include "Components/TextBlock.h"

void UP1ChatBubbleWidget::SetBubbleMessage(const FText& Message)
{
    MessageText->SetText(Message);
}

void UP1ChatBubbleWidget::SetBubbleVisible(bool bVisible)
{
    SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
