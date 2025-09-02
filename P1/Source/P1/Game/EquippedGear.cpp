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

void UEquippedGear::Init(const Protocol::ObjectInfo& Info)
{
    auto& _EquippedGear = Info.player_info().equipped_gear();

    if (AInGamePlayerController* InGamePlayerController = Cast<AInGamePlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
    {
        // 장비 창
        for (const Protocol::Slot& _Slot : _EquippedGear)
        {
            int32 _SlotId = _Slot.slot_id();
            _Gear[_SlotId] = _Slot;
            InGamePlayerController->UpdateEquippedGearSlot(_Slot);
        }
    }
}