#include "pch.h"
#include "EquippedGear.h"
#include "Player.h"

EquippedGear::EquippedGear(PlayerRef player) : _player(player)
{
    equippedGearLookup = player->playerInfo->mutable_equipped_gear();

    for (int32 slotId = 0; slotId <= Protocol::GearType_MAX; slotId++)
    {
        Protocol::Slot slot;
        slot.set_slot_id(slotId);
        slot.set_type(Protocol::SlotType::SLOT_TYPE_EQUIPPED);
        slot.set_state(Protocol::UpdateState::UPDATE_STATE_NONE);

        equippedGearLookup->emplace(slotId, std::move(slot));
    }

    gearTypeMappings = {
        {"helmet", Protocol::GearType::GEAR_TYPE_HELMET},
        {"chest", Protocol::GearType::GEAR_TYPE_CHEST},
        {"legs", Protocol::GearType::GEAR_TYPE_LEGS},
        {"arms", Protocol::GearType::GEAR_TYPE_ARMS},
        {"boots", Protocol::GearType::GEAR_TYPE_BOOTS},
        {"sword", Protocol::GearType::GEAR_TYPE_WEAPON},
        {"weapon", Protocol::GearType::GEAR_TYPE_WEAPON}
    };
}

EquippedGear::~EquippedGear()
{
}

bool EquippedGear::EquipGear(OUT Protocol::Slot* reflectSlot, OUT Protocol::StatInfo* statInfo, Protocol::Item& itemInstance, optional<int32> setSlotId)
{
    int32 templateId = itemInstance.template_id();
    const Json& ItemData = Gamedata::ItemDataTable[templateId];

    // 장비 타입 아이템인지 체크
    if (gearTypeMappings.find(ItemData["itemSubtype"]) == gearTypeMappings.end())
        return false;
    
    Protocol::GearType type = setSlotId.has_value() ? (Protocol::GearType)setSlotId.value() : gearTypeMappings[ItemData["itemSubtype"]];
    Protocol::Slot* targetSlot = equippedGearLookup->find(type) != equippedGearLookup->end() ? &(*equippedGearLookup)[type] : nullptr;

    // 장착 중인 상태에서 다른 장비 장착 금지
    if (targetSlot == nullptr || targetSlot->has_item() == true)
        return false;

    gearDirtyFlags[type] = true;

    targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_ADDED);
    targetSlot->mutable_item()->CopyFrom(itemInstance);

    if(reflectSlot != nullptr)
        reflectSlot->CopyFrom(*targetSlot);

    // 스텟 반영
    if (statInfo != nullptr)
    {
        if (ItemData.count("hp") && ItemData["hp"] > 0)
            statInfo->set_max_hp(statInfo->max_hp() + ItemData["hp"]);
        if (ItemData.count("mp") && ItemData["mp"] > 0)
            statInfo->set_max_mp(statInfo->max_mp() + ItemData["mp"]);
        if (ItemData.count("physicalAttack") && ItemData["physicalAttack"] > 0)
            statInfo->set_physical_attack(statInfo->physical_attack() + ItemData["physicalAttack"]);
        if (ItemData.count("magicalAttack") && ItemData["magicalAttack"] > 0)
            statInfo->set_magical_attack(statInfo->magical_attack() + ItemData["magicalAttack"]);
    }
  
    return true;
}

bool EquippedGear::UnequipGear(OUT Protocol::Slot* reflectSlot, OUT Protocol::StatInfo* statInfo, Protocol::Slot* slot)
{
    if (slot->has_item() == false)
        return false;

    const Protocol::Item& item = slot->item();
    int32 templateId = item.template_id();
    const Json& ItemData = Gamedata::ItemDataTable[templateId];

    // 장착 반영
    if (gearTypeMappings.find(ItemData["itemSubtype"]) == gearTypeMappings.end())
        return false;

    Protocol::GearType type = gearTypeMappings[ItemData["itemSubtype"]];
    Protocol::Slot* targetSlot = &(*equippedGearLookup)[type];

    if (targetSlot->has_item() == false)
        return false;

    gearDirtyFlags[type] = true;

    targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_REMOVED);
    targetSlot->clear_item();

    if (reflectSlot != nullptr)
        reflectSlot->CopyFrom(*targetSlot);

    // 스텟 반영
    if (ItemData.count("hp") && ItemData["hp"] > 0)
        statInfo->set_max_hp(statInfo->max_hp() - ItemData["hp"]);
    if (ItemData.count("mp") && ItemData["mp"] > 0)
        statInfo->set_max_mp(statInfo->max_mp() - ItemData["mp"]);
    if (ItemData.count("physicalAttack") && ItemData["physicalAttack"] > 0)
        statInfo->set_physical_attack(statInfo->physical_attack() - ItemData["physicalAttack"]);
    if (ItemData.count("magicalAttack") && ItemData["magicalAttack"] > 0)
        statInfo->set_magical_attack(statInfo->magical_attack() - ItemData["magicalAttack"]);

    return true;
}
