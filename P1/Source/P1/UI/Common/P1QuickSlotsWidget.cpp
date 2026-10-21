#include "UI/Common/P1QuickSlotsWidget.h"
#include "UI/Common/P1SlotWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Core/P1GameInstance.h"
#include "Game/Items/P1ItemContainer.h"
#include "Game/Progress/P1ItemActions.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Game/QuickSlots/P1QuickSlots.h"
#include "Utils/LogCategory.h"

void UP1QuickSlotsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // 슬롯 위젯 대응과 끌어 놓기 연결. 위젯이 사는 동안 한 번만 한다.
    WidgetTree->ForEachWidget([this](UWidget* Widget)
        {
            if (UP1SlotWidget* SlotWidget = Cast<UP1SlotWidget>(Widget))
                SlotWidgets.Add(SlotWidget);
        });

    for (UP1SlotWidget* SlotWidget : SlotWidgets)
    {
        SlotWidget->OnDragRequested.BindUObject(this, &UP1QuickSlotsWidget::MakeDragPayload);
        SlotWidget->OnDropped.BindUObject(this, &UP1QuickSlotsWidget::HandleDrop);
        SlotWidget->OnDragCancelled.AddUObject(this, &UP1QuickSlotsWidget::HandleDragCancelled);
    }
}

void UP1QuickSlotsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 지금 퀵슬롯으로 그리고 이후 바뀌는 퀵슬롯과 소모품 구역을 구독. 떼는 것은 UP1UserWidget::NativeDestruct가 한다.
    UP1MyPlayerData* MyPlayerData = GetMyPlayerData();
    UP1QuickSlots* QuickSlots = MyPlayerData ? MyPlayerData->GetQuickSlots() : nullptr;
    if (QuickSlots == nullptr)
    {
        UE_LOG(LogP1UI, Warning, TEXT("%s: 그릴 퀵슬롯이 없다."), *GetName());
        return;
    }

    // 서버가 알려 준 퀵슬롯이 슬롯 위젯보다 많으면 남는 퀵슬롯은 화면에 없다. 기획의 퀵슬롯 수를 바꾸면 WBP도 고친다.
    if (QuickSlots->GetQuickSlots().Num() > SlotWidgets.Num())
        UE_LOG(LogP1UI, Warning, TEXT("%s: 퀵슬롯 %d개 중 %d개만 그린다."), *GetName(), QuickSlots->GetQuickSlots().Num(), SlotWidgets.Num());

    Redraw();
    QuickSlots->OnChanged.AddUObject(this, &UP1QuickSlotsWidget::HandleQuickSlotChanged);
    QuickSlots->OnReset.AddUObject(this, &UP1QuickSlotsWidget::HandleQuickSlotsReset);

    // 개수는 소모품 구역 전체의 합이므로 소모품 칸이 바뀔 때마다 다시 센다.
    if (UP1ItemContainer* Consumables = MyPlayerData->GetContainer(EP1ContainerType::InventoryConsumable))
    {
        Consumables->OnSlotChanged.AddUObject(this, &UP1QuickSlotsWidget::HandleConsumableSlotChanged);
        Consumables->OnReset.AddUObject(this, &UP1QuickSlotsWidget::HandleConsumablesReset);
    }
}

void UP1QuickSlotsWidget::HandleQuickSlotsReset(const UP1QuickSlots& QuickSlots)
{
    Redraw();
}

void UP1QuickSlotsWidget::HandleQuickSlotChanged(const FP1QuickSlot& Changed)
{
    Redraw();
}

void UP1QuickSlotsWidget::HandleConsumableSlotChanged(const FP1Slot& Changed)
{
    Redraw();
}

void UP1QuickSlotsWidget::HandleConsumablesReset(const UP1ItemContainer& Container)
{
    Redraw();
}

void UP1QuickSlotsWidget::Redraw()
{
    const UP1MyPlayerData* MyPlayerData = GetMyPlayerData();
    const UP1QuickSlots* QuickSlots = MyPlayerData ? MyPlayerData->GetQuickSlots() : nullptr;
    const UP1ItemContainer* Consumables = MyPlayerData ? MyPlayerData->GetContainer(EP1ContainerType::InventoryConsumable) : nullptr;
    if (QuickSlots == nullptr)
        return;

    // 퀵슬롯은 몇 개 되지 않으므로 무엇이 바뀌었든 전부 다시 그린다.
    for (int32 i = 0; i < SlotWidgets.Num(); ++i)
    {
        const FP1QuickSlot* QuickSlot = QuickSlots->Find(i);
        SlotWidgets[i]->SetDisplay(QuickSlot ? FP1SlotDisplay::FromQuickSlotRef(QuickSlot->Ref, Consumables) : FP1SlotDisplay());
    }
}

TOptional<FP1SlotDragPayload> UP1QuickSlotsWidget::MakeDragPayload(const UP1SlotWidget* SlotWidget) const
{
    const int32 QuickSlotIndex = FindQuickSlotIndex(SlotWidget);
    if (QuickSlotIndex == INDEX_NONE)
        return {};

    FP1SlotDragPayload Payload;
    Payload.Origin = FP1SlotDragPayload::EOrigin::QuickSlot;
    Payload.QuickSlotIndex = QuickSlotIndex;
    Payload.Display = SlotWidget->GetDisplay();
    return Payload;
}

bool UP1QuickSlotsWidget::HandleDrop(UP1SlotWidget* Target, const FP1SlotDragPayload& Payload)
{
    UP1MyPlayerData* MyPlayerData = GetMyPlayerData();
    const int32 TargetIndex = FindQuickSlotIndex(Target);
    if (MyPlayerData == nullptr || TargetIndex == INDEX_NONE)
        return false;

    // 퀵슬롯 위에 놓은 끌기는 받아들인다. 받아들이지 않으면 끌어 온 퀵슬롯이 바깥에 놓인 것으로 보고 해제한다.
    FP1QuickSlotRef Ref;
    if (Payload.Origin == FP1SlotDragPayload::EOrigin::QuickSlot)
    {
        if (Payload.QuickSlotIndex == TargetIndex)
            return true;

        // 놓은 자리에 끌어 온 대상을 등록한다. 같은 대상은 퀵슬롯 하나에만 있으므로 서버가 두 퀵슬롯을 맞바꾼다.
        const FP1QuickSlot* Source = MyPlayerData->GetQuickSlots()->Find(Payload.QuickSlotIndex);
        if (Source == nullptr || Source->Ref.IsEmpty())
            return true;

        Ref = Source->Ref;
    }
    else
    {
        // 칸 하나가 아니라 그 칸에 든 아이템의 정의를 등록한다.
        if (Payload.Display.IsEmpty())
            return true;

        Ref = FP1QuickSlotRef::ForItem(Payload.Display.TemplateId);
    }

    MyPlayerData->GetItemActions()->RequestSetQuickSlot(TargetIndex, Ref);
    return true;
}

void UP1QuickSlotsWidget::HandleDragCancelled(UP1SlotWidget* Source, const FP1SlotDragPayload& Payload)
{
    if (Payload.Origin != FP1SlotDragPayload::EOrigin::QuickSlot)
        return;

    if (UP1MyPlayerData* MyPlayerData = GetMyPlayerData())
        MyPlayerData->GetItemActions()->RequestSetQuickSlot(Payload.QuickSlotIndex, FP1QuickSlotRef());
}

int32 UP1QuickSlotsWidget::FindQuickSlotIndex(const UP1SlotWidget* SlotWidget) const
{
    const int32 WidgetIndex = SlotWidgets.IndexOfByKey(SlotWidget);
    const UP1MyPlayerData* MyPlayerData = GetMyPlayerData();
    const UP1QuickSlots* QuickSlots = MyPlayerData ? MyPlayerData->GetQuickSlots() : nullptr;
    if (WidgetIndex == INDEX_NONE || QuickSlots == nullptr || QuickSlots->Find(WidgetIndex) == nullptr)
        return INDEX_NONE;

    return WidgetIndex;
}

UP1MyPlayerData* UP1QuickSlotsWidget::GetMyPlayerData() const
{
    UP1GameInstance* GameInstance = GetP1GameInstance();
    return GameInstance ? GameInstance->GetSubsystem<UP1MyPlayerData>() : nullptr;
}
