#include "UI/Common/P1QuickSlotsWidget.h"
#include "UI/Common/P1SlotWidget.h"
#include "Components/PanelWidget.h"
#include "Core/P1GameInstance.h"
#include "Game/Items/P1ItemContainer.h"
#include "Game/Progress/P1ItemActions.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Game/QuickSlots/P1QuickSlots.h"
#include "Utils/LogCategory.h"

void UP1QuickSlotsWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    // 디자이너 미리보기에서 퀵슬롯의 배치를 보인다. 실행 중에는 서버가 알려 준 개수로 NativeConstruct에서 만든다.
    if (IsDesignTime())
        BuildSlots(PreviewSlotCount);
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

    BuildSlots(QuickSlots->GetQuickSlots().Num());
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

void UP1QuickSlotsWidget::BuildSlots(int32 Count)
{
    if (SlotBox == nullptr || SlotWidgetClass == nullptr)
        return;

    if (SlotWidgets.Num() == Count && SlotBox->GetChildrenCount() == Count)
        return;

    SlotBox->ClearChildren();
    SlotWidgets.Reset(Count);
    for (int32 i = 0; i < Count; ++i)
    {
        UP1SlotWidget* SlotWidget = CreateWidget<UP1SlotWidget>(this, SlotWidgetClass);
        SlotBox->AddChild(SlotWidget);
        SlotWidgets.Add(SlotWidget);

        // 디자이너 미리보기의 슬롯은 끌어 놓기를 받지 않는다.
        if (IsDesignTime())
            continue;

        SlotWidget->OnDragRequested.BindUObject(this, &UP1QuickSlotsWidget::MakeDragPayload);
        SlotWidget->OnDropped.BindUObject(this, &UP1QuickSlotsWidget::HandleDrop);
        SlotWidget->OnDragCancelled.AddUObject(this, &UP1QuickSlotsWidget::HandleDragCancelled);
    }
}

void UP1QuickSlotsWidget::HandleQuickSlotsReset(const UP1QuickSlots& QuickSlots)
{
    // 다른 캐릭터로 들어오면 퀵슬롯 개수가 바뀌었을 수 있다.
    BuildSlots(QuickSlots.GetQuickSlots().Num());
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
    const TArray<FP1QuickSlot>& All = QuickSlots->GetQuickSlots();
    for (int32 i = 0; i < SlotWidgets.Num(); ++i)
    {
        if (SlotWidgets[i] == nullptr)
            continue;

        SlotWidgets[i]->SetDisplay(All.IsValidIndex(i) ? FP1SlotDisplay::FromQuickSlotRef(All[i].Ref, Consumables) : FP1SlotDisplay());
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
    if (WidgetIndex == INDEX_NONE || QuickSlots == nullptr || QuickSlots->GetQuickSlots().IsValidIndex(WidgetIndex) == false)
        return INDEX_NONE;

    return QuickSlots->GetQuickSlots()[WidgetIndex].SlotIndex;
}

UP1MyPlayerData* UP1QuickSlotsWidget::GetMyPlayerData() const
{
    UP1GameInstance* GameInstance = GetP1GameInstance();
    return GameInstance ? GameInstance->GetSubsystem<UP1MyPlayerData>() : nullptr;
}
