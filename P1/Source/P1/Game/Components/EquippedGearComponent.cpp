// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Components/EquippedGearComponent.h"
#include "P1GameInstance.h"

UEquippedGearComponent::UEquippedGearComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

void UEquippedGearComponent::BeginPlay()
{
	Super::BeginPlay();

    /*AActor* Owner = GetOwner();
    if (Owner->IsA<AP1MyPlayer>() == false)
        return;

    if (UP1GameInstance* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance()))
    {
        Protocol::ObjectInfo* PlayerInfo = GameInstance->CachedMyPlayerInfo;
        if (PlayerInfo == nullptr)
            return;

        Init(PlayerInfo->mutable_player_info());
    }*/
	
}

void UEquippedGearComponent::Init(Protocol::PlayerInfo* InPlayerInfo)
{
    EquippedGearLookup = InPlayerInfo->mutable_equipped_gear();
}

void UEquippedGearComponent::HandleSlotChanged(const Protocol::Slot& InSlot)
{
    int32 SlotId_ = InSlot.slot_id();
    Protocol::SlotType SlotType_ = InSlot.type();

    if (SlotType_ != Protocol::SlotType::SLOT_TYPE_EQUIPPED)
        return;

    (*EquippedGearLookup)[SlotId_] = InSlot;

    OnSlotChanged.Broadcast(InSlot);
}
