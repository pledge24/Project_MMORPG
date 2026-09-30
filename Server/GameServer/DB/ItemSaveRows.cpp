#include "pch.h"
#include "ItemSaveRows.h"

namespace
{
    GearSaveRow MakeGearRow(int64 characterId, const Protocol::Slot& slot, bool isEquipped)
    {
        const Protocol::Item& item = slot.item();

        GearSaveRow row;
        {
            row.characterId = characterId;
            row.slotId = slot.slot_id();
            row.itemUid = item.item_uid();
            row.templateId = item.template_id();
            row.isEquipped = isEquipped;
            row.enhance = item.gearinfo().enhance_level();
            row.durability = item.gearinfo().durability();
            row.additionalPhysicalAttack = item.gearinfo().additional_physical_attack();
            row.additionalMagicalAttack = item.gearinfo().additional_magical_attack();
        }
        return row;
    }
}

optional<vector<GearSaveRow>> ItemSaveRows::BuildGearRows(const PlayerSaveData& data)
{
    // 플래그가 없을 때 빈 결과를 내면 뒤의 착용 장비만 반영된 채 성공으로 끝난다.
    if (data.gearDirtyFlags.has_value() == false)
        return nullopt;

    const int64 characterId = data.playerInfo.character_id();
    const Protocol::Possession& possession = data.possession;
    const vector<bool>& gearDirtyFlags = data.gearDirtyFlags.value();

    vector<GearSaveRow> rows;

    // 인벤토리에 들어 있는 장비
    const auto& inventoryGear = possession.inventory().gear();
    for (int32 i = 0; i < inventoryGear.size(); i++)
    {
        if (gearDirtyFlags[i])
            rows.push_back(MakeGearRow(characterId, inventoryGear.Get(i), false));
    }

    // 착용 중인 장비
    for (const auto& [slotId, isDirty] : data.equippedGearDirtyFlags)
    {
        if (isDirty)
            rows.push_back(MakeGearRow(characterId, possession.equipped_gear().at(slotId), true));
    }

    return rows;
}

optional<vector<StackableItemSaveRow>> ItemSaveRows::BuildStackableRows(const PlayerSaveData& data, Protocol::ItemType itemType)
{
    const Protocol::Inventory& inven = data.possession.inventory();

    const RepeatedPtrField<Protocol::Slot>* slots = nullptr;
    const optional<vector<bool>>* dirtyFlags = nullptr;
    switch (itemType)
    {
    case Protocol::ITEM_TYPE_CONSUMABLE:
        slots = &inven.consumables();
        dirtyFlags = &data.consumableDirtyFlags;
        break;
    case Protocol::ITEM_TYPE_MISCELLANEOUS:
        slots = &inven.miscellaneous();
        dirtyFlags = &data.miscDirtyFlags;
        break;
    default:
        return nullopt;
    }

    // 플래그가 없을 때 빈 결과를 내면 아무것도 반영하지 않고 성공으로 끝난다.
    if (dirtyFlags->has_value() == false)
        return nullopt;

    const int64 characterId = data.playerInfo.character_id();
    const vector<bool>& flags = dirtyFlags->value();

    vector<StackableItemSaveRow> rows;
    for (int32 i = 0; i < slots->size(); i++)
    {
        if (flags[i] == false)
            continue;

        const Protocol::Slot& slot = slots->Get(i);
        StackableItemSaveRow row;
        {
            row.characterId = characterId;
            row.slotId = slot.slot_id();
            row.templateId = slot.item().template_id();
            row.count = slot.item().count();
        }
        rows.push_back(row);
    }

    return rows;
}
