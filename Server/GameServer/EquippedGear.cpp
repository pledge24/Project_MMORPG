#include "pch.h"
#include "EquippedGear.h"

EquippedGear::EquippedGear()
{
    _gear.resize(MAX_EQUIPPABLE_SLOT_ID + 1);
    _gearDirtyFlags.resize(MAX_EQUIPPABLE_SLOT_ID + 1);
}

EquippedGear::~EquippedGear()
{
}

void EquippedGear::Init(Protocol::PlayerInfo* info)
{
    // 1. 장착 중인 장비 아이템
    for (const auto& slot : info->equipped_gear())
    {
        int32 slotId = slot.slot_id();
        _gear[slotId] = slot.item();
    }
}

bool EquippedGear::EquipGear(Protocol::Slot* slot, OUT Protocol::Slot* updatedSlot)
{
    return false;
}

bool EquippedGear::UnEquipGear(Protocol::Slot* slot, OUT Protocol::Slot* updatedSlot)
{
    return false;
}
