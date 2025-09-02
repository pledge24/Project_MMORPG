// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/InventoryWidget.h"
#include "Components\UniformGridPanel.h"
#include "SlotWidget.h"

void UInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UInventoryWidget::Clear()
{
    for (UWidget* GridSlot : Gear_Inven->GetAllChildren())
    {
        USlotWidget* slot = Cast<USlotWidget>(GridSlot);
        if (slot)
            slot->ClearSlot();
    }

    for (UWidget* GridSlot : Consumables_Inven->GetAllChildren())
    {
        USlotWidget* slot = Cast<USlotWidget>(GridSlot);
        if (slot)
            slot->ClearSlot();
    }

    for (UWidget* GridSlot : Misc_Inven->GetAllChildren())
    {
        USlotWidget* slot = Cast<USlotWidget>(GridSlot);
        if (slot)
            slot->ClearSlot();
    }
}

void UInventoryWidget::UpdateSlot(const Protocol::Slot& _Slot)
{
    USlotWidget* SlotWidget = GetSlotWidgetFromSlot(_Slot);

    if (SlotWidget)
        SlotWidget->SetSlot(_Slot);
}

USlotWidget* UInventoryWidget::GetSlotWidgetFromSlot(const Protocol::Slot& _Slot)
{
    UUniformGridPanel* Inven = nullptr;
    int32 SlotId = _Slot.slot_id();

    switch (_Slot.type())
    {
    case Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR:
        Inven = Gear_Inven;
        break;
    case Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE:
        Inven = Consumables_Inven;
        break;
    case Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC:
        Inven = Misc_Inven;
        break;
    }

    if (!Inven || SlotId < 0 || SlotId >= Inven->GetChildrenCount())
    {
        return nullptr;
    }

    UWidget* ChildWidget = Inven->GetChildAt(SlotId);

    if (ChildWidget)
    {
        return Cast<USlotWidget>(ChildWidget);
    }

    return nullptr;
}
