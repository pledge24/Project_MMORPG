// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/EquippedGear.h"
#include "P1MyPlayer.h"
#include "P1.h"

void UEquippedGear::Init(Protocol::PlayerInfo* PlayerInfo_)
{
    EquippedGearLookup = PlayerInfo_->mutable_equipped_gear();
}

void UEquippedGear::SetSlot(const Protocol::Slot& Slot_)
{
    int32 SlotId_ = Slot_.slot_id();
    Protocol::SlotType SlotType_ = Slot_.type();

    if (SlotType_ != Protocol::SlotType::SLOT_TYPE_EQUIPPED)
        return;

    EquippedGearLookup->at(SlotId_).CopyFrom(Slot_);
}
