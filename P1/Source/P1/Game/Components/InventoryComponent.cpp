// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"
#include "P1GameInstance.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

    InventoryLookupMappings =
    {
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR, TArray<Protocol::Slot*>()},
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE, TArray<Protocol::Slot*>()},
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC, TArray<Protocol::Slot*>()}
    };
}


void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

    //AActor* Owner = GetOwner();
    //if (Owner->IsA<AP1MyPlayer>() == false)
    //    return;

    //if (UP1GameInstance* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance()))
    //{
    //    Protocol::ObjectInfo* PlayerInfo = GameInstance->CachedMyPlayerInfo;
    //    if (PlayerInfo == nullptr)
    //        return;

    //    Protocol::Inventory* Inventory = PlayerInfo->mutable_player_info()->mutable_inventory();
    //    Init(Inventory);
    //}

}

void UInventoryComponent::Init(Protocol::Inventory* Inventory_)
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
}

void UInventoryComponent::HandleSlotChanged(const Protocol::Slot& InSlot, bool OnUse)
{
    if (InventoryLookupMappings.Contains(InSlot.type()))
    {
        TArray<Protocol::Slot*>& InvenLookup = InventoryLookupMappings[InSlot.type()];
        InvenLookup[InSlot.slot_id()]->CopyFrom(InSlot);
    }
    OnSlotChanged.Broadcast(InSlot, OnUse);
}
