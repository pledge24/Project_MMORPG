// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/InventoryWidget.h"
#include "Components\UniformGridPanel.h"
#include "SlotWidget.h"

void UInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UInventoryWidget::Init(TArray<FSlotData>& Items)
{
    UpdateMultipleSlots(Items);
}

void UInventoryWidget::Clear()
{
    for (UWidget* GridSlot : GridPanel->GetAllChildren())
    {
        USlotWidget* slot = Cast<USlotWidget>(GridSlot);
        if (slot)
            slot->ClearSlot();
    }
}

void UInventoryWidget::UpdateSingleSlot(FSlotData slotItem)
{
    USlotWidget* slot = GetSlotFromSlotId(slotItem.SlotId);

    if (slot)
        slot->InitSlot(slotItem.Item, slotItem.Quantity);
}

void UInventoryWidget::UpdateMultipleSlots(TArray<FSlotData>& Items)
{
    for (FSlotData& slotItem : Items)
    {
        USlotWidget* slot = GetSlotFromSlotId(slotItem.SlotId);

        if(slot)
            slot->InitSlot(slotItem.Item, slotItem.Quantity);
    }
}

USlotWidget* UInventoryWidget::GetSlotFromSlotId(int32 SlotId)
{
    if (!GridPanel || SlotId < 0 || SlotId >= GridPanel->GetChildrenCount())
    {
        return nullptr;
    }

    UWidget* ChildWidget = GridPanel->GetChildAt(SlotId);

    if (ChildWidget)
    {
        return Cast<USlotWidget>(ChildWidget);
    }

    return nullptr;
}
