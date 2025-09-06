// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/EquippedGear.h"
#include "P1.h"

UEquippedGear::UEquippedGear()
{
    _Gear.SetNum(MAX_EQUIPPED_SLOTS +1);
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
            _Gear[SlotId] = Slot_;
            InGamePlayerController->OnUpdateEquippedGearSlot(Slot_);
        }
    }
}