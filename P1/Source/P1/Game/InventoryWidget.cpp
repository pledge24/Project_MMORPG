// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/InventoryWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/TextBlock.h"
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

void UInventoryWidget::UpdateGold(int32 Gold)
{
    Gold_txt->SetText(FText::AsNumber(Gold));
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

void UInventoryWidget::SendSellPacket(USlotWidget* _Slot)
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("OnSell!")));
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("OnSell! template_id: %d"), _Slot->ItemData.TemplateId));
}

void UInventoryWidget::SendEquipPacket(USlotWidget* _Slot)
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("OnEquip!")));
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("OnBuy! template_id: %d"), _Slot->ItemData.TemplateId));
}
