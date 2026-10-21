#include "UI/Common/P1SlotGridWidget.h"
#include "UI/Common/P1SlotWidget.h"
#include "Components/UniformGridPanel.h"

void UP1SlotGridWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    // 디자이너 미리보기에서 칸 수와 열 수를 바꾸면 격자를 다시 만든다.
    // 실행 중에는 초기화 때 만든 슬롯 위젯에 클릭이 묶여 있으므로 다시 만들지 않는다.
    if (IsDesignTime())
        BuildSlots();
}

void UP1SlotGridWidget::CollectSlotWidgets(TMap<int32, TObjectPtr<UP1SlotWidget>>& OutSlotWidgets)
{
    BuildSlots();

    for (int32 SlotIndex = 0; SlotIndex < SlotGrid->GetChildrenCount(); ++SlotIndex)
        OutSlotWidgets.Add(SlotIndex, Cast<UP1SlotWidget>(SlotGrid->GetChildAt(SlotIndex)));
}

void UP1SlotGridWidget::BuildSlots()
{
    if (SlotGrid == nullptr || SlotWidgetClass == nullptr)
        return;

    if (SlotGrid->GetChildrenCount() == SlotCount)
        return;

    SlotGrid->ClearChildren();
    for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
    {
        UP1SlotWidget* SlotWidget = CreateWidget<UP1SlotWidget>(this, SlotWidgetClass);
        SlotGrid->AddChildToUniformGrid(SlotWidget, SlotIndex / Columns, SlotIndex % Columns);
    }
}
