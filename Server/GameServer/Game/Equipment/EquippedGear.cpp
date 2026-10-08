#include "Core/pch.h"
#include "Game/Equipment/EquippedGear.h"
#include "Game/Entities/Player.h"

EquippedGear::EquippedGear(PlayerRef player) : _player(player)
{
    _equippedGearLookup = player->_possession->mutable_equipped_gear();

    for (int32 slotId = 0; slotId <= Protocol::GearType_MAX; slotId++)
    {
        Protocol::Slot slot;
        slot.set_slot_id(slotId);
        slot.set_type(Protocol::SlotType::SLOT_TYPE_EQUIPPED);
        slot.set_state(Protocol::UpdateState::UPDATE_STATE_NONE);

        _equippedGearLookup->emplace(slotId, std::move(slot));
    }

    _gearTypeMappings = {
        {JsonProperty::Item::GearSubtype_Helmet, Protocol::GearType::GEAR_TYPE_HELMET},
        {JsonProperty::Item::GearSubtype_Chest, Protocol::GearType::GEAR_TYPE_CHEST},
        {JsonProperty::Item::GearSubtype_Legs, Protocol::GearType::GEAR_TYPE_LEGS},
        {JsonProperty::Item::GearSubtype_Arms, Protocol::GearType::GEAR_TYPE_ARMS},
        {JsonProperty::Item::GearSubtype_Boots, Protocol::GearType::GEAR_TYPE_BOOTS},
        {JsonProperty::Item::GearSubtype_Sword, Protocol::GearType::GEAR_TYPE_WEAPON}
    };
}

EquippedGear::~EquippedGear()
{
}

bool EquippedGear::EquipGear(OUT Protocol::Slot* replicatingSlot, OUT RepeatedPtrField<Protocol::Stat>* updatedStatList, const Protocol::Item& itemInstance, optional<int32> setSlotId)
{
    const Json* itemDataPtr = Gamedata::FindItemData(itemInstance.template_id());
    if (itemDataPtr == nullptr)
        return false;

    const Json& itemData = *itemDataPtr;

    // 장비 타입 아이템인지 체크
    optional<Protocol::GearType> gearType = FindGearType(itemData);
    if (gearType.has_value() == false)
        return false;
    
    Protocol::GearType type = setSlotId.has_value() ? (Protocol::GearType)setSlotId.value() : gearType.value();
    Protocol::Slot* targetSlot = _equippedGearLookup->find(type) != _equippedGearLookup->end() ? &(*_equippedGearLookup)[type] : nullptr;

    // 장착 중인 상태에서 다른 장비 장착 금지
    if (targetSlot == nullptr || targetSlot->has_item() == true)
        return false;

    _dirtyFlagMappings[type] = true;

    targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_ADDED);
    targetSlot->mutable_item()->CopyFrom(itemInstance);

    if(replicatingSlot != nullptr)
        replicatingSlot->CopyFrom(*targetSlot);

    // 스텟 반영(반드시 증가함)
    // DB에서 불러올 때는 updatedStatList가 없다. 그때 스텟은 Player::CalculateFinalStat이 장비까지 한 번에 계산한다.
    if (updatedStatList == nullptr)
        return true;

    if (PlayerRef ownerPlayer = _player.lock())
    {
        const string_view& maxHpProperty = JsonProperty::Item::Hp;
        if (itemData.contains(maxHpProperty) && itemData[maxHpProperty] > 0)
        {
            int64 updatedMaxHp = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAX_HP) + itemData[maxHpProperty];
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAX_HP, updatedMaxHp);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAX_HP));
        }

        const string_view& maxMpProperty = JsonProperty::Item::Mp;
        if (itemData.contains(maxMpProperty) && itemData[maxMpProperty] > 0)
        {
            int64 updatedMaxMp = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAX_MP) + itemData[maxMpProperty];
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAX_MP, updatedMaxMp);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAX_MP));
        }

        const string_view& paProperty = JsonProperty::Item::PhysicalAttack;
        if (itemData.contains(paProperty) && itemData[paProperty] > 0)
        {
            int64 updatedPA = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK) + itemData[paProperty];
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK, updatedPA);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_PHYSICAL_ATTACK));
        }

        const string_view& maProperty = JsonProperty::Item::MagicalAttack;
        if (itemData.contains(maProperty) && itemData[maProperty] > 0)
        {
            int64 updatedMA = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK) + itemData[maProperty];
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK, updatedMA);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAGICAL_ATTACK));
        }
    }
  
    return true;
}

bool EquippedGear::UnequipGear(int32 gearType, OUT Protocol::Slot* replicatingSlot, OUT RepeatedPtrField<Protocol::Stat>* updatedStatList)
{
    auto slotIt = _equippedGearLookup->find(gearType);
    if (slotIt == _equippedGearLookup->end() || slotIt->second.has_item() == false)
        return false;

    // 스텟은 그 부위에 실제로 든 장비의 수치로 뺀다.
    const Json* itemDataPtr = Gamedata::FindItemData(slotIt->second.item().template_id());
    if (itemDataPtr == nullptr)
        return false;

    const Json& itemData = *itemDataPtr;
    Protocol::Slot* targetSlot = &slotIt->second;

    _dirtyFlagMappings[gearType] = true;

    targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_REMOVED);
    targetSlot->clear_item();

    if (replicatingSlot != nullptr)
        replicatingSlot->CopyFrom(*targetSlot);

    // 스텟 반영(반드시 감소함)
    if (updatedStatList == nullptr)
        return true;

    if (PlayerRef ownerPlayer = _player.lock())
    {
        const string_view& maxHpProperty = JsonProperty::Item::Hp;
        if (itemData.contains(maxHpProperty) && itemData[maxHpProperty] > 0)
        {
            int64 updatedMaxHp = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAX_HP) - itemData[maxHpProperty];
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAX_HP, updatedMaxHp);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAX_HP));
        }

        const string_view& maxMpProperty = JsonProperty::Item::Mp;
        if (itemData.contains(maxMpProperty) && itemData[maxMpProperty] > 0)
        {
            int64 updatedMaxMp = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAX_MP) - itemData[maxMpProperty];
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAX_MP, updatedMaxMp);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAX_MP));
        }

        const string_view& paProperty = JsonProperty::Item::PhysicalAttack;
        if (itemData.contains(paProperty) && itemData[paProperty] > 0)
        {
            int64 updatedPA = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK) - itemData[paProperty];
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK, updatedPA);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_PHYSICAL_ATTACK));
        }

        const string_view& maProperty = JsonProperty::Item::MagicalAttack;
        if (itemData.contains(maProperty) && itemData[maProperty] > 0)
        {
            int64 updatedMA = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK) - itemData[maProperty];
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK, updatedMA);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAGICAL_ATTACK));
        }
    }

    return true;
}

const Protocol::Slot* EquippedGear::GetSlot(int32 gearType) const
{
    auto it = _equippedGearLookup->find(gearType);
    if (it == _equippedGearLookup->end())
        return nullptr;

    return &it->second;
}

void EquippedGear::ClearDirtyFlag()
{
    for (auto& dirtyFlagPair : _dirtyFlagMappings)
        dirtyFlagPair.second = false;
}

optional<Protocol::GearType> EquippedGear::FindGearType(const Json& itemData) const
{
    auto subtypeIt = itemData.find(JsonProperty::Item::ItemSubtype);
    if (subtypeIt == itemData.end() || subtypeIt->is_string() == false)
        return nullopt;

    auto it = _gearTypeMappings.find(subtypeIt->get_ref<const string&>());
    if (it == _gearTypeMappings.end())
        return nullopt;

    return it->second;
}
