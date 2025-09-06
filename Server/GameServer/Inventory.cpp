#include "pch.h"
#include "Inventory.h"
#include "Global.h"

Inventory::Inventory()
{
    _gear.resize(MAX_SLOTS);
    _consumables.resize(MAX_SLOTS);
    _miscellaneous.resize(MAX_SLOTS);

    _gearDirtyFlags.resize(MAX_SLOTS, Protocol::UpdateState::UPDATE_STATE_NONE);
    _consumablesDirtyFlags.resize(MAX_SLOTS, Protocol::UpdateState::UPDATE_STATE_NONE);
    _miscellaneousDirtyFlags.resize(MAX_SLOTS, Protocol::UpdateState::UPDATE_STATE_NONE);

    inventoryMappings = {
        {Protocol::ItemType::ITEM_TYPE_GEAR, _gear},
        {Protocol::ItemType::ITEM_TYPE_CONSUMABLE, _consumables},
        {Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS, _miscellaneous}
    };

    dirtyFlagsMappings = {
        {Protocol::ItemType::ITEM_TYPE_GEAR, _gearDirtyFlags},
        {Protocol::ItemType::ITEM_TYPE_CONSUMABLE, _consumablesDirtyFlags},
        {Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS, _miscellaneousDirtyFlags}
    };
}

Inventory::~Inventory()
{
}

void Inventory::Init(Protocol::PlayerInfo* info)
{
    const Protocol::Inventory& inven = info->inventory();

    // 1. 장비 아이템
    for (const auto& slot : inven.gear())
    {
        int32 slotId = slot.slot_id();
        _gear[slotId] = slot;
    }

    // 2. 소비 아이템
    for (const auto& slot : inven.consumables())
    {
        int32 slotId = slot.slot_id();
        _consumables[slotId] = slot;
    }

    // 3. 기타 아이템
    for (const auto& slot : inven.miscellaneous())
    {
        int32 slotId = slot.slot_id();
        _miscellaneous[slotId] = slot;
    }
}

bool Inventory::addItem(OUT Protocol::Slot* updatedSlot, Protocol::Item& itemInstance, int32 count)
{


    return true;
}

bool Inventory::addItem(OUT Protocol::Slot* updatedSlot, int32 templateId, int32 count)
{
    // -> 아직 인스턴스화된 아이템이 아닐때 진입(ex. 구매한 아이템)
    const Json& itemData = Gamedata::ItemDataTable[templateId];
    Protocol::ItemType type = Protocol::ItemType::ITEM_TYPE_NONE;
    if (itemData["itemType"] == "armor" || itemData["itemType"] == "weapon")
    {
        type = Protocol::ItemType::ITEM_TYPE_GEAR;
    }
    else if (itemData["itemType"] == "consumption")
    {
        type = Protocol::ItemType::ITEM_TYPE_CONSUMABLE;
    }
    else if (itemData["itemType"] == "miscellaneous")
    {
        type = Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS;
    }
    else
    {
        return false;
    }

    int32 availableSlotId = findFirstAvailableSlotId(type, templateId);

    // 슬롯 찾았으니 이제 진짜 추가해야함
    vector<Protocol::Slot>& categoryInven = inventoryMappings[type];
    vector<Protocol::UpdateState>& categoryDirtyFlags = dirtyFlagsMappings[type];
    updatedSlot = &categoryInven[availableSlotId];

    if (updatedSlot->has_item())
    {
        //categoryDirtyFlags[availableSlotId] = Protocol::UpdateState::UPDATE_STATE_UPDATE;
    }
    else
    {
        //categoryDirtyFlags[availableSlotId] = Protocol::UpdateState::UPDATE_STATE_INSERT;
    }

    return true; 
}

bool Inventory::removeItem(OUT Protocol::Slot* updatedSlot, Protocol::Slot* targetSlot, int32 count)
{


    return true;
}

int32 Inventory::findFirstAvailableSlotId(Protocol::ItemType type, int32 templateId)
{
    if (type == Protocol::ItemType::ITEM_TYPE_NONE)
    {
        cout << "findFirstAvailableSlotId: Invalid ItemType" << endl;
        return -1;
    }

    const Json& itemData = Gamedata::ItemDataTable[templateId];
 
    vector<Protocol::Slot>& categoryInven = inventoryMappings[type];
    int32 availableSlotId = -1;
    if (type == Protocol::ItemType::ITEM_TYPE_GEAR)
    {
        auto it = std::find_if(categoryInven.begin(), categoryInven.end(), [templateId](const Protocol::Slot& slot)
            {
                return slot.has_item() == false;
            });

        availableSlotId = it - categoryInven.begin();
    }
    else
    {
        auto it = std::find_if(categoryInven.begin(), categoryInven.end(), [templateId](const Protocol::Slot& slot)
            {
                return slot.item().template_id() == templateId;
            });

        availableSlotId = it - categoryInven.begin();
    }

    return availableSlotId;
}

