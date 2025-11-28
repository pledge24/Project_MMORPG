// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/Inventory.h"
#include "P1.h"

UInventory::UInventory()
{
    InventoryLookupMappings =
    {
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR, TArray<Protocol::Slot*>()},
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE, TArray<Protocol::Slot*>()},
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC, TArray<Protocol::Slot*>()}
    };
}

void UInventory::Init(Protocol::Inventory* Inventory_)
{
    // 장비창 룩업 저장
    {
        int32 size = Inventory_->gear_size();
        TArray<Protocol::Slot*>& GearLookup = InventoryLookupMappings[Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR];
        GearLookup.SetNum(size);
        for (int32 i = 0; i < size; ++i)
        {
            Protocol::Slot* Slot_ = Inventory_->mutable_gear(i);
            GearLookup[Slot_->slot_id()] = Slot_;
        }
    }

    // 소비창 룩업 저장
    {
        int32 size = Inventory_->consumables_size();
        TArray<Protocol::Slot*>& ConsumablesLookup = InventoryLookupMappings[Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE];
        ConsumablesLookup.SetNum(size);
        for (int32 i = 0; i < size; ++i)
        {
            Protocol::Slot* Slot_ = Inventory_->mutable_consumables(i);
            ConsumablesLookup[Slot_->slot_id()] = Slot_;
        }
    }

    // 기타창 룩업 저장
    {
        int32 size = Inventory_->miscellaneous_size();
        TArray<Protocol::Slot*>& MiscellaneousLookup = InventoryLookupMappings[Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC];
        MiscellaneousLookup.SetNum(size);
        for (int32 i = 0; i < size; ++i)
        {
            Protocol::Slot* Slot_ = Inventory_->mutable_miscellaneous(i);
            MiscellaneousLookup[Slot_->slot_id()] = Slot_;
        }
    }

}

void UInventory::Rep_SlotChanged(const Protocol::Slot& Slot_, bool OnUse)
{
    if (InventoryLookupMappings.Contains(Slot_.type()))
    {
        TArray<Protocol::Slot*>& InvenLookup = InventoryLookupMappings[Slot_.type()];
        InvenLookup[Slot_.slot_id()]->CopyFrom(Slot_);
    }
}

void UInventory::PrintInventoryData()
{
    TArray<Protocol::Slot*>& GearLookup = InventoryLookupMappings[Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR];

    for (auto Gear : GearLookup)
    {
        FString GearStr = UTF8_TO_TCHAR(Gear->DebugString().c_str());
        UE_LOG(LogTemp, Log, TEXT("%s"), *GearStr);
    }
}
