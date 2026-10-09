#include "Core/pch.h"
#include "Game/Equipment/EquipmentComponent.h"
#include "Game/Entities/Player.h"

EquipmentComponent::EquipmentComponent(PlayerRef owner) : EntityComponent(owner)
{
    _equippedGearLookup = owner->_possession->mutable_equipped_gear();

    for (int32 slotId = 0; slotId <= Protocol::GearType_MAX; slotId++)
    {
        Protocol::Slot slot;
        slot.set_slot_id(slotId);
        slot.set_type(Protocol::SlotType::SLOT_TYPE_EQUIPPED);
        slot.set_state(Protocol::UpdateState::UPDATE_STATE_NONE);

        _equippedGearLookup->emplace(slotId, std::move(slot));
    }
}

EquipmentComponent::~EquipmentComponent()
{
}

bool EquipmentComponent::Equip(OUT Protocol::Slot* replicatingSlot, const Protocol::Item& itemInstance)
{
    const ItemTemplate* itemTemplate = Gamedata::FindItem(itemInstance.template_id());
    if (itemTemplate == nullptr || itemTemplate->gearType.has_value() == false)
        return false;

    return PlaceItem(replicatingSlot, itemInstance, itemTemplate->gearType.value());
}

bool EquipmentComponent::LoadEquipped(const Protocol::Item& itemInstance, int32 gearType)
{
    const ItemTemplate* itemTemplate = Gamedata::FindItem(itemInstance.template_id());
    if (itemTemplate == nullptr || itemTemplate->gearType.has_value() == false)
        return false;

    return PlaceItem(nullptr, itemInstance, gearType);
}

bool EquipmentComponent::Unequip(int32 gearType, OUT Protocol::Slot* replicatingSlot)
{
    auto slotIt = _equippedGearLookup->find(gearType);
    if (slotIt == _equippedGearLookup->end() || slotIt->second.has_item() == false)
        return false;

    Protocol::Slot* targetSlot = &slotIt->second;

    _dirtyFlagMappings[gearType] = true;

    targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_REMOVED);
    targetSlot->clear_item();

    if (replicatingSlot != nullptr)
        replicatingSlot->CopyFrom(*targetSlot);

    return true;
}

CombatStats EquipmentComponent::SumStatDelta() const
{
    CombatStats sum;
    for (const auto& [gearType, slot] : *_equippedGearLookup)
    {
        if (slot.has_item() == false || slot.item().template_id() == 0)
            continue;

        // 장비 칸에는 표에 있는 아이템만 들어간다(Equip과 LoadEquipped가 거른다).
        const ItemTemplate* itemTemplate = Gamedata::FindItem(slot.item().template_id());
        if (itemTemplate == nullptr)
            continue;

        sum += CombatStats{ itemTemplate->hp, itemTemplate->mp, itemTemplate->physicalAttack, itemTemplate->magicalAttack };
    }

    return sum;
}

const Protocol::Slot* EquipmentComponent::GetSlot(int32 gearType) const
{
    auto it = _equippedGearLookup->find(gearType);
    if (it == _equippedGearLookup->end())
        return nullptr;

    return &it->second;
}

void EquipmentComponent::ClearDirtyFlags()
{
    for (auto& dirtyFlagPair : _dirtyFlagMappings)
        dirtyFlagPair.second = false;
}

bool EquipmentComponent::PlaceItem(OUT Protocol::Slot* replicatingSlot, const Protocol::Item& itemInstance, int32 gearType)
{
    auto slotIt = _equippedGearLookup->find(gearType);

    // 장착 중인 부위에 다른 장비를 겹쳐 입지 않는다.
    if (slotIt == _equippedGearLookup->end() || slotIt->second.has_item())
        return false;

    Protocol::Slot* targetSlot = &slotIt->second;

    _dirtyFlagMappings[gearType] = true;

    targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_ADDED);
    targetSlot->mutable_item()->CopyFrom(itemInstance);

    if (replicatingSlot != nullptr)
        replicatingSlot->CopyFrom(*targetSlot);

    return true;
}
