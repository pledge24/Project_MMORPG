#include "pch.h"
#include "EquippedGear.h"
#include "Player.h"

EquippedGear::EquippedGear(PlayerRef player) : _player(player)
{
    equippedGearLookup = player->possession->mutable_equipped_gear();

    for (int32 slotId = 0; slotId <= Protocol::GearType_MAX; slotId++)
    {
        Protocol::Slot slot;
        slot.set_slot_id(slotId);
        slot.set_type(Protocol::SlotType::SLOT_TYPE_EQUIPPED);
        slot.set_state(Protocol::UpdateState::UPDATE_STATE_NONE);

        equippedGearLookup->emplace(slotId, std::move(slot));
    }

    gearTypeMappings = {
        {JsonProperty::Item::GearSubtype_Helmet, Protocol::GearType::GEAR_TYPE_HELMET},
        {JsonProperty::Item::GearSubtype_Chest, Protocol::GearType::GEAR_TYPE_CHEST},
        {JsonProperty::Item::GearSubtype_Legs, Protocol::GearType::GEAR_TYPE_LEGS},
        {JsonProperty::Item::GearSubtype_Arms, Protocol::GearType::GEAR_TYPE_ARMS},
        {JsonProperty::Item::GearSubtype_Boots, Protocol::GearType::GEAR_TYPE_BOOTS},
        {JsonProperty::Item::GearSubtype_Sword, Protocol::GearType::GEAR_TYPE_WEAPON},
        {JsonProperty::Item::GearType_Weapon, Protocol::GearType::GEAR_TYPE_WEAPON}
    };
}

EquippedGear::~EquippedGear()
{
}

bool EquippedGear::EquipGear(OUT Protocol::Slot* replicatingSlot, OUT RepeatedPtrField<Protocol::Stat>* updatedStatList, const Protocol::Item& itemInstance, optional<int32> setSlotId)
{
    int32 templateId = itemInstance.template_id();
    const Json& ItemData = Gamedata::ItemDataTable[templateId];

    // 장비 타입 아이템인지 체크
    if (gearTypeMappings.find(ItemData[JsonProperty::Item::ItemSubtype]) == gearTypeMappings.end())
        return false;
    
    Protocol::GearType type = setSlotId.has_value() ? (Protocol::GearType)setSlotId.value() : gearTypeMappings[ItemData[JsonProperty::Item::ItemSubtype]];
    Protocol::Slot* targetSlot = equippedGearLookup->find(type) != equippedGearLookup->end() ? &(*equippedGearLookup)[type] : nullptr;

    // 장착 중인 상태에서 다른 장비 장착 금지
    if (targetSlot == nullptr || targetSlot->has_item() == true)
        return false;

    dirtyFlagMappings[type] = true;

    targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_ADDED);
    targetSlot->mutable_item()->CopyFrom(itemInstance);

    if(replicatingSlot != nullptr)
        replicatingSlot->CopyFrom(*targetSlot);

    // 스텟 반영(반드시 증가함)
    if (PlayerRef ownerPlayer = _player.lock())
    {
        const string_view& maxHpProperty = JsonProperty::Item::Hp;
        if (ItemData.contains(maxHpProperty) && ItemData[maxHpProperty] > 0)
        {
            int64 updatedMaxHp = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAX_HP) + ItemData[maxHpProperty];
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAX_HP, updatedMaxHp);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAX_HP));
        }

        const string_view& maxMpProperty = JsonProperty::Item::Mp;
        if (ItemData.contains(maxMpProperty) && ItemData[maxMpProperty] > 0)
        {
            int64 updatedMaxMp = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAX_MP) + ItemData[maxMpProperty];
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAX_MP, updatedMaxMp);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAX_MP));
        }

        const string_view& paProperty = JsonProperty::Item::PhysicalAttack;
        if (ItemData.contains(paProperty) && ItemData[paProperty] > 0)
        {
            int64 updatedPA = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK) + ItemData[paProperty];
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK, updatedPA);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_PHYSICAL_ATTACK));
        }

        const string_view& maProperty = JsonProperty::Item::MagicalAttack;
        if (ItemData.contains(maProperty) && ItemData[maProperty] > 0)
        {
            int64 updatedMA = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK) + ItemData[maProperty];
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK, updatedMA);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAGICAL_ATTACK));
        }
    }
  
    return true;
}

bool EquippedGear::UnequipGear(const Protocol::Slot& requestSlot, OUT Protocol::Slot* replicatingSlot, OUT RepeatedPtrField<Protocol::Stat>* updatedStatList)
{
    if (requestSlot.has_item() == false)
        return false;

    const Protocol::Item& item = requestSlot.item();
    int32 templateId = item.template_id();
    const Json& ItemData = Gamedata::ItemDataTable[templateId];

    // 장착 반영
    if (gearTypeMappings.find(ItemData[JsonProperty::Item::ItemSubtype]) == gearTypeMappings.end())
        return false;

    Protocol::GearType type = gearTypeMappings[ItemData[JsonProperty::Item::ItemSubtype]];
    Protocol::Slot* targetSlot = &(*equippedGearLookup)[type];

    if (targetSlot->has_item() == false)
        return false;

    dirtyFlagMappings[type] = true;

    targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_REMOVED);
    targetSlot->clear_item();

    if (replicatingSlot != nullptr)
        replicatingSlot->CopyFrom(*targetSlot);

    // 스텟 반영(반드시 감소함)
    if (PlayerRef ownerPlayer = _player.lock())
    {
        const string_view& maxHpProperty = JsonProperty::Item::Hp;
        if (ItemData.contains(maxHpProperty) && ItemData[maxHpProperty] > 0)
        {
            int64 updatedMaxHp = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAX_HP) - ItemData[maxHpProperty];
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAX_HP, updatedMaxHp);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAX_HP));
        }

        const string_view& maxMpProperty = JsonProperty::Item::Mp;
        if (ItemData.contains(maxMpProperty) && ItemData[maxMpProperty] > 0)
        {
            int64 updatedMaxMp = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAX_MP) - ItemData[maxMpProperty];
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAX_MP, updatedMaxMp);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAX_MP));
        }

        const string_view& paProperty = JsonProperty::Item::PhysicalAttack;
        if (ItemData.contains(paProperty) && ItemData[paProperty] > 0)
        {
            int64 updatedPA = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK) - ItemData[paProperty];
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK, updatedPA);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_PHYSICAL_ATTACK));
        }

        const string_view& maProperty = JsonProperty::Item::MagicalAttack;
        if (ItemData.contains(maProperty) && ItemData[maProperty] > 0)
        {
            int64 updatedMA = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK) - ItemData[maProperty];
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK, updatedMA);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAGICAL_ATTACK));
        }
    }

    return true;
}

void EquippedGear::ClearDirtyFlag()
{
    for (auto& dirtyFlagPair : dirtyFlagMappings)
        dirtyFlagPair.second = false;
}
