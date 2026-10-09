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

bool EquipmentComponent::EquipGear(OUT Protocol::Slot* replicatingSlot, OUT RepeatedPtrField<Protocol::Stat>* updatedStatList, const Protocol::Item& itemInstance, optional<int32> setSlotId)
{
    const ItemTemplate* itemTemplate = Gamedata::FindItem(itemInstance.template_id());
    if (itemTemplate == nullptr)
        return false;

    // 장비 타입 아이템인지 체크
    const optional<Protocol::GearType>& gearType = itemTemplate->gearType;
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

    if (PlayerRef ownerPlayer = static_pointer_cast<Player>(_owner.lock()))
    {
        if (itemTemplate->hp > 0)
        {
            int64 updatedMaxHp = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAX_HP) + itemTemplate->hp;
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAX_HP, updatedMaxHp);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAX_HP));
        }

        if (itemTemplate->mp > 0)
        {
            int64 updatedMaxMp = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAX_MP) + itemTemplate->mp;
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAX_MP, updatedMaxMp);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAX_MP));
        }

        if (itemTemplate->physicalAttack > 0)
        {
            int64 updatedPA = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK) + itemTemplate->physicalAttack;
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK, updatedPA);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_PHYSICAL_ATTACK));
        }

        if (itemTemplate->magicalAttack > 0)
        {
            int64 updatedMA = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK) + itemTemplate->magicalAttack;
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK, updatedMA);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAGICAL_ATTACK));
        }
    }
  
    return true;
}

bool EquipmentComponent::UnequipGear(int32 gearType, OUT Protocol::Slot* replicatingSlot, OUT RepeatedPtrField<Protocol::Stat>* updatedStatList)
{
    auto slotIt = _equippedGearLookup->find(gearType);
    if (slotIt == _equippedGearLookup->end() || slotIt->second.has_item() == false)
        return false;

    // 스텟은 그 부위에 실제로 든 장비의 수치로 뺀다.
    const ItemTemplate* itemTemplate = Gamedata::FindItem(slotIt->second.item().template_id());
    if (itemTemplate == nullptr)
        return false;
    Protocol::Slot* targetSlot = &slotIt->second;

    _dirtyFlagMappings[gearType] = true;

    targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_REMOVED);
    targetSlot->clear_item();

    if (replicatingSlot != nullptr)
        replicatingSlot->CopyFrom(*targetSlot);

    // 스텟 반영(반드시 감소함)
    if (updatedStatList == nullptr)
        return true;

    if (PlayerRef ownerPlayer = static_pointer_cast<Player>(_owner.lock()))
    {
        if (itemTemplate->hp > 0)
        {
            int64 updatedMaxHp = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAX_HP) - itemTemplate->hp;
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAX_HP, updatedMaxHp);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAX_HP));
        }

        if (itemTemplate->mp > 0)
        {
            int64 updatedMaxMp = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAX_MP) - itemTemplate->mp;
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAX_MP, updatedMaxMp);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAX_MP));
        }

        if (itemTemplate->physicalAttack > 0)
        {
            int64 updatedPA = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK) - itemTemplate->physicalAttack;
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK, updatedPA);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_PHYSICAL_ATTACK));
        }

        if (itemTemplate->magicalAttack > 0)
        {
            int64 updatedMA = ownerPlayer->GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK) - itemTemplate->magicalAttack;
            ownerPlayer->SetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK, updatedMA);
            updatedStatList->Add()->CopyFrom(ownerPlayer->GetStat(Protocol::STAT_TYPE_MAGICAL_ATTACK));
        }
    }

    return true;
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
