// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/EquippedGear.h"
#include "P1MyPlayer.h"
#include "P1.h"

UEquippedGear::UEquippedGear()
{
    EquippedGearLookup.SetNum(MAX_EQUIPPED_SLOTS + 1);
}

UEquippedGear::~UEquippedGear()
{
}

void UEquippedGear::Init(Protocol::PlayerInfo* PlayerInfo_, AActor* Owner)
{
    _Owner = Owner;

    int32 size = PlayerInfo_->equipped_gear_size();
    EquippedGearLookup.SetNum(size);
    for (int32 i = 0; i < size; ++i)
    {
        Protocol::Slot* Slot_ = PlayerInfo_->mutable_equipped_gear(i);
        EquippedGearLookup[Slot_->slot_id()] = Slot_;
    }
}

void UEquippedGear::SetSlot(const Protocol::Slot& Slot_)
{
    int32 SlotId_ = Slot_.slot_id();
    Protocol::SlotType SlotType_ = Slot_.type();

    if (SlotType_ != Protocol::SlotType::SLOT_TYPE_EQUIPPED)
        return;

    EquippedGearLookup[SlotId_]->CopyFrom(Slot_);
}
