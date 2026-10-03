#include "UI/WorldSpace/P1NameTagWidget.h"
#include "Components/TextBlock.h"

void UP1NameTagWidget::SetNameTag(const FText& Text, const FLinearColor& Color)
{
    if (NameTag == nullptr)
        return;

    NameTag->SetText(Text);
    NameTag->SetColorAndOpacity(FSlateColor(Color));
}
