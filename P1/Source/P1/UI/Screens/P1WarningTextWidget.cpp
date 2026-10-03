#include "UI/Screens/P1WarningTextWidget.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

void UP1WarningTextWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 위젯 루트는 화면 서브시스템이 보이게 둔다. 보이고 숨기는 것은 문구 하나다.
    WarningText->SetText(FText::GetEmpty());
    WarningText->SetVisibility(ESlateVisibility::Collapsed);
}

void UP1WarningTextWidget::DisplayWarningMessage(const FText& WarningMessage)
{
    WarningText->SetText(WarningMessage);
    WarningText->SetVisibility(ESlateVisibility::HitTestInvisible);

    // 같은 핸들로 다시 걸면 남은 시간을 버리고 처음부터 센다. 블루프린트의 RetriggerableDelay와 같다.
    if (UWorld* World = GetWorld())
        World->GetTimerManager().SetTimer(HideTimerHandle, this, &UP1WarningTextWidget::HideWarningMessage, DisplaySeconds, false);
}

void UP1WarningTextWidget::HideWarningMessage()
{
    WarningText->SetVisibility(ESlateVisibility::Collapsed);
}
