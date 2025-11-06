// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/EquippedGear.h"
#include "P1MyPlayer.h"
#include "P1.h"

void UEquippedGear::Init(Protocol::PlayerInfo* PlayerInfo_)
{
    EquippedGearLookup = PlayerInfo_->equipped_gear();
}

void UEquippedGear::Rep_SlotChanged(const Protocol::Slot& Slot_)
{
    int32 SlotId_ = Slot_.slot_id();
    Protocol::SlotType SlotType_ = Slot_.type();

    if (SlotType_ != Protocol::SlotType::SLOT_TYPE_EQUIPPED)
        return;

    EquippedGearLookup[SlotId_].CopyFrom(Slot_);
}