#include "UI/Common/P1FixedSlotsWidget.h"
#include "UI/Common/P1SlotWidget.h"
#include "Utils/LogCategory.h"

void UP1FixedSlotsWidget::CollectSlotWidgets(TMap<int32, TObjectPtr<UP1SlotWidget>>& OutSlotWidgets)
{
    for (const auto& Pair : SlotIndexByWidgetName)
    {
        UP1SlotWidget* SlotWidget = Cast<UP1SlotWidget>(GetWidgetFromName(Pair.Key));
        if (SlotWidget == nullptr)
        {
            UE_LOG(LogP1UI, Warning, TEXT("%s: 슬롯 위젯 %s가 없다."), *GetName(), *Pair.Key.ToString());
            continue;
        }

        OutSlotWidgets.Add(Pair.Value, SlotWidget);
    }
}
