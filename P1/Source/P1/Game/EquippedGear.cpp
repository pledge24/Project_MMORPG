// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/EquippedGear.h"
#include "P1MyPlayer.h"
#include "P1.h"

UEquippedGear::UEquippedGear(AActor* Owner_) : Owner(Owner_)
{
    _EquippedGearLookup.SetNum(MAX_EQUIPPED_SLOTS + 1);

    if (AP1MyPlayer* MyPlayer = Cast<AP1MyPlayer>(Owner))
    {
        MyPlayer->OnEquipGearDelegate.AddUObject(this, &UEquippedGear::UpdateSlot);
        MyPlayer->OnUnequipGearDelegate.AddUObject(this, &UEquippedGear::UpdateSlot);
    }
}

UEquippedGear::~UEquippedGear()
{
}

void UEquippedGear::Init(const google::protobuf::RepeatedPtrField<Protocol::Slot>& EquippedGear_)
{
    if (AInGamePlayerController* InGamePlayerController = Cast<AInGamePlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
    {
        // 장비 창
        for (const Protocol::Slot& Slot_ : EquippedGear_)
        {
            int32 SlotId = Slot_.slot_id();
            _EquippedGearLookup[SlotId]->CopyFrom(Slot_.item());
            InGamePlayerController->OnUpdateEquippedGearSlot(Slot_);
        }
    }
}

void UEquippedGear::UpdateSlot(const Protocol::Slot& Slot_)
{
    int32 SlotId_ = Slot_.slot_id();
    Protocol::SlotType SlotType_ = Slot_.type();
    if (SlotType_ != Protocol::SlotType::SLOT_TYPE_EQUIPPED)
        return;

    if (AInGamePlayerController* InGamePlayerController = Cast<AInGamePlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
    {
        _EquippedGearLookup[SlotId_]->CopyFrom(Slot_.item());
        InGamePlayerController->OnUpdateInventorySlot(Slot_);
    }
}