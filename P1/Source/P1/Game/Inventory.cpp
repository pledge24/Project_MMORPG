// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/Inventory.h"
#include "P1.h"

UInventory::UInventory()
{
    _Gear.SetNum(MAX_SLOTS);
    _Consumables.SetNum(MAX_SLOTS);
    _Miscellaneous.SetNum(MAX_SLOTS);
}

UInventory::~UInventory()
{
}

void UInventory::Init(const Protocol::Inventory& Inventory_)
{
    if (AInGamePlayerController* InGamePlayerController = Cast<AInGamePlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
    {
        // 장비 창
        for (const Protocol::Slot& Slot_ : Inventory_.gear())
        {
            int32 SlotId = Slot_.slot_id();
            _Gear[SlotId].CopyFrom(Slot_);
            InGamePlayerController->OnUpdateInventorySlot(Slot_);
        }

        // 소비 창
        for (const Protocol::Slot& Slot_ : Inventory_.consumables())
        {
            int32 SlotId = Slot_.slot_id();
            _Consumables[SlotId].CopyFrom(Slot_);
            InGamePlayerController->OnUpdateInventorySlot(Slot_);
        }

        // 기타 창
        for (const Protocol::Slot& Slot_ : Inventory_.miscellaneous())
        {
            int32 SlotId = Slot_.slot_id();
            _Miscellaneous[SlotId].CopyFrom(Slot_);
            InGamePlayerController->OnUpdateInventorySlot(Slot_);
        }
    }
    
}

void UInventory::UpdateSlots(const google::protobuf::RepeatedPtrField<Protocol::Slot>& Slots)
{
    if (AInGamePlayerController* InGamePlayerController = Cast<AInGamePlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
    {
        for (const Protocol::Slot& _Slot : Slots)
        {
            /*int32 _SlotId = _Slot.slot_id();
            Protocol::ItemType _ItemType = _Slot.item().type();

            switch (_ItemType)
            {
            case Protocol::ItemType::ITEM_TYPE_GEAR:
                _Gear[_SlotId] = _Slot.item();
                InGamePlayerController->UpdateInventorySlot(Protocol::ItemType::ITEM_TYPE_GEAR, _Slot);
                break;
            case Protocol::ItemType::ITEM_TYPE_CONSUMABLE:
                _Consumables[_SlotId] = _Slot.item();
                InGamePlayerController->UpdateInventorySlot(Protocol::ItemType::ITEM_TYPE_CONSUMABLE, _Slot);
                break;
            case Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS:
                _Miscellaneous[_SlotId] = _Slot.item();
                InGamePlayerController->UpdateInventorySlot(Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS, _Slot);
                break;
            }*/
        }
    }
}

