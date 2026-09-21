#include "Game/Equipment/P1EquippedGear.h"
#include "Game/Entities/P1MyPlayer.h"
#include "P1.h"

void UP1EquippedGear::Init(Map<int32, Protocol::Slot>* EquippedGear_)
{
    EquippedGearLookup = EquippedGear_;
}

void UP1EquippedGear::Rep_SlotChanged(const Protocol::Slot& Slot_)
{
    int32 SlotId_ = Slot_.slot_id();
    Protocol::SlotType SlotType_ = Slot_.type();

    if (SlotType_ != Protocol::SlotType::SLOT_TYPE_EQUIPPED)
        return;

    (*EquippedGearLookup)[SlotId_].CopyFrom(Slot_);
}