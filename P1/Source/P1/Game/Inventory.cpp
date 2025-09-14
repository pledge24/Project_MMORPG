// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/Inventory.h"
#include "P1MyPlayer.h"
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

UInventory::~UInventory()
{
}

void UInventory::Init(Protocol::Inventory* Inventory_, AActor* Owner)
{
    _Owner = Owner;

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
            Protocol::Slot* Slot_ = Inventory_->mutable_gear(i);
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

    // 델리게이트 바인딩
    if (AP1MyPlayer* MyPlayer_ = Cast<AP1MyPlayer>(_Owner))
    {
        MyPlayer_->OnRefresh.AddUObject(this, &UInventory::Refresh);
    }
}

void UInventory::Refresh()
{
    TArray<Protocol::Slot*>& GearLookup = InventoryLookupMappings[Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR];
    TArray<Protocol::Slot*>& ConsumablesLookup = InventoryLookupMappings[Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE];
    TArray<Protocol::Slot*>& MiscellaneousLookup = InventoryLookupMappings[Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC];

    OnInventoryRefreshed.Broadcast(GearLookup);
    OnInventoryRefreshed.Broadcast(ConsumablesLookup);
    OnInventoryRefreshed.Broadcast(MiscellaneousLookup);
}

void UInventory::SetSlot(const Protocol::Slot& Slot_)
{  
    TArray<Protocol::Slot*>& InvenLookup = InventoryLookupMappings[Slot_.type()];
    InvenLookup[Slot_.slot_id()]->CopyFrom(Slot_);
}
