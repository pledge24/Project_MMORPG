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

void UInventory::Init(const Protocol::ObjectInfo& Info)
{
    const Protocol::Inventory& _Inventory = Info.player_info().inventory();

    if (AInGamePlayerController* InGamePlayerController = Cast<AInGamePlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
    {
        // 장비 창
        for (const Protocol::Slot& _Slot : _Inventory.gear())
        {
            int32 _SlotId = _Slot.slot_id();
            _Gear[_SlotId] = _Slot;
            InGamePlayerController->UpdateInventorySlot(_Slot);
        }

        // 소비 창
        for (const Protocol::Slot& _Slot : _Inventory.consumables())
        {
            int32 _SlotId = _Slot.slot_id();
            _Consumables[_SlotId] = _Slot;
            InGamePlayerController->UpdateInventorySlot(_Slot);
        }

        // 기타 창
        for (const Protocol::Slot& _Slot : _Inventory.miscellaneous())
        {
            int32 _SlotId = _Slot.slot_id();
            _Miscellaneous[_SlotId] = _Slot;
            InGamePlayerController->UpdateInventorySlot(_Slot);
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



