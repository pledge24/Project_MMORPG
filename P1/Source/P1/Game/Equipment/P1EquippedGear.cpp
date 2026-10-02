#include "Game/Equipment/P1EquippedGear.h"

void UP1EquippedGear::Init(google::protobuf::Map<int32, Protocol::Slot>* EquippedGear_)
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