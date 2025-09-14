// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/Inventory.h"
#include "P1MyPlayer.h"
#include "P1.h"

UInventory::UInventory(AActor* Owner_) : Owner(Owner_)
{
    InventoryLookupMappings =
    {
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR, GearLookup},
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE, ConsumablesLookup},
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC, MiscellaneousLookup}
    };
}

UInventory::~UInventory()
{
}

void UInventory::Init(const Protocol::Inventory& Inventory_)
{
    if (AInGamePlayerController* InGamePlayerController = Cast<AInGamePlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
    {
        // 장비 창
        GearLookup.SetNum(Inventory_.gear().size());
        for (const Protocol::Slot& Slot_ : Inventory_.gear())
        {
            // 룩업 저장
            GearLookup[Slot_.slot_id()]->CopyFrom(Slot_.item());
            InGamePlayerController->OnUpdateInventorySlot(Slot_);
        }

        // 소비 창
        ConsumablesLookup.SetNum(Inventory_.consumables().size());
        for (const Protocol::Slot& Slot_ : Inventory_.consumables())
        {
            ConsumablesLookup[Slot_.slot_id()]->CopyFrom(Slot_.item());
            InGamePlayerController->OnUpdateInventorySlot(Slot_);
        }

        // 기타 창
        MiscellaneousLookup.SetNum(Inventory_.miscellaneous().size());
        for (const Protocol::Slot& Slot_ : Inventory_.miscellaneous())
        {
            MiscellaneousLookup[Slot_.slot_id()]->CopyFrom(Slot_.item());
            InGamePlayerController->OnUpdateInventorySlot(Slot_);
        }
    }
    
    // 델리게이트 바인드
    if (AP1MyPlayer* MyPlayer = Cast<AP1MyPlayer>(Owner))
    {
        MyPlayer->OnItemAdded.AddUObject(this, &UInventory::AddItem);
        MyPlayer->OnItemRemoved.AddUObject(this, &UInventory::RemoveItem);
    }
}

void UInventory::AddItem(const Protocol::Slot& Slot_)
{
    // Validation
    if (Slot_.state() != Protocol::UpdateState::UPDATE_STATE_ADDED && 
        Slot_.state() != Protocol::UpdateState::UPDATE_STATE_MODIFIED)
        return;
    
    TArray<Protocol::Item*>& InvenLookup = InventoryLookupMappings[Slot_.type()];
    InvenLookup[Slot_.slot_id()]->CopyFrom(Slot_.item());

    OnInventoryUpdated.Broadcast(Slot_);
}

void UInventory::RemoveItem(const Protocol::Slot& Slot_)
{
    // Validation
    if (Slot_.state() != Protocol::UpdateState::UPDATE_STATE_REMOVED &&
        Slot_.state() != Protocol::UpdateState::UPDATE_STATE_MODIFIED)
        return;

    TArray<Protocol::Item*>& InvenLookup = InventoryLookupMappings[Slot_.type()];
    InvenLookup[Slot_.slot_id()]->CopyFrom(Slot_.item());

    OnInventoryUpdated.Broadcast(Slot_);
}
