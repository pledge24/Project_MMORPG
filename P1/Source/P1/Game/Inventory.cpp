// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/Inventory.h"
#include "P1MyPlayer.h"
#include "P1.h"

UInventory::UInventory(AActor* Owner_) : Owner(Owner_)
{
    GearLookup.SetNum(MAX_SLOTS);
    ConsumablesLookup.SetNum(MAX_SLOTS);
    MiscellaneousLookup.SetNum(MAX_SLOTS);
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
            // 룩업 저장
            int32 SlotId = Slot_.slot_id();
            GearLookup[SlotId]->CopyFrom(Slot_.item());

            InGamePlayerController->OnUpdateInventorySlot(Slot_);
        }

        // 소비 창
        for (const Protocol::Slot& Slot_ : Inventory_.consumables())
        {
            int32 SlotId = Slot_.slot_id();
            ConsumablesLookup[SlotId]->CopyFrom(Slot_.item());

            InGamePlayerController->OnUpdateInventorySlot(Slot_);
        }

        // 기타 창
        for (const Protocol::Slot& Slot_ : Inventory_.miscellaneous())
        {
            int32 SlotId = Slot_.slot_id();
            MiscellaneousLookup[SlotId]->CopyFrom(Slot_.item());

            InGamePlayerController->OnUpdateInventorySlot(Slot_);
        }
    }
    
    if (AP1MyPlayer* MyPlayer = Cast<AP1MyPlayer>(Owner))
    {
        MyPlayer->OnAddItemDelegate.AddUObject(this, &UInventory::UpdateSlot);
        MyPlayer->OnRemoveItemDelegate.AddUObject(this, &UInventory::UpdateSlot);
    }
}

void UInventory::UpdateSlots(const google::protobuf::RepeatedPtrField<Protocol::Slot>& Slots)
{
    if (AInGamePlayerController* InGamePlayerController = Cast<AInGamePlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
    {
        for (const Protocol::Slot& Slot_ : Slots)
        {
            UpdateSlot(Slot_);
        }
    }
}

void UInventory::UpdateSlot(const Protocol::Slot& Slot_)
{
    int32 SlotId_ = Slot_.slot_id();
    Protocol::SlotType SlotType_ = Slot_.type();
    if (AInGamePlayerController* InGamePlayerController = Cast<AInGamePlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
    {
        switch (SlotType_)
        {
        case Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR:
            GearLookup[SlotId_]->CopyFrom(Slot_.item());
            break;
        case Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE:
            ConsumablesLookup[SlotId_]->CopyFrom(Slot_.item());
            break;
        case Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC:
            MiscellaneousLookup[SlotId_]->CopyFrom(Slot_.item());
            break;
        }

        InGamePlayerController->OnUpdateInventorySlot(Slot_);
    }
}