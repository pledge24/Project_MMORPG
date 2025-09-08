#include "pch.h"
#include "Inventory.h"
#include "Global.h"
#include "Player.h"

Inventory::Inventory()
{
    lookupTableMappings = {
        {Protocol::ItemType::ITEM_TYPE_GEAR, vector<Protocol::Slot*>(MAX_SLOTS, nullptr)},
        {Protocol::ItemType::ITEM_TYPE_CONSUMABLE, vector<Protocol::Slot*>(MAX_SLOTS, nullptr)},
        {Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS, vector<Protocol::Slot*>(MAX_SLOTS, nullptr)}
    };

    dirtyFlagsMappings = {
        {Protocol::ItemType::ITEM_TYPE_GEAR, vector<bool>(MAX_SLOTS)},
        {Protocol::ItemType::ITEM_TYPE_CONSUMABLE, vector<bool>(MAX_SLOTS)},
        {Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS, vector<bool>(MAX_SLOTS)}
    };

    slotTypeToItemTypeMappings = {
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR, Protocol::ItemType::ITEM_TYPE_GEAR},
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE, Protocol::ItemType::ITEM_TYPE_CONSUMABLE},
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC, Protocol::ItemType::ITEM_TYPE_GEAR}
    };
}

Inventory::~Inventory()
{
}

void Inventory::Init(Protocol::PlayerInfo* info)
{
    Protocol::Inventory* inven = info->mutable_inventory();

    // 1. 장비 아이템
    RepeatedPtrField<Protocol::Slot>* gear = inven->mutable_gear();
    for (int32 i = 0; i < gear->size(); ++i)
    {
        Protocol::Slot* slot = gear->Mutable(i);
        int32 slotId = slot->slot_id();
        lookupTableMappings[Protocol::ItemType::ITEM_TYPE_GEAR][slotId] = slot;
    }

    // 2. 소비 아이템
    RepeatedPtrField<Protocol::Slot>* consumables = inven->mutable_consumables();
    for (int32 i = 0; i < consumables->size(); ++i)
    {
        Protocol::Slot* slot = consumables->Mutable(i);
        int32 slotId = slot->slot_id();
        lookupTableMappings[Protocol::ItemType::ITEM_TYPE_CONSUMABLE][slotId] = slot;
    }

    // 3. 기타 아이템
    RepeatedPtrField<Protocol::Slot>* misc = inven->mutable_miscellaneous();
    for (int32 i = 0; i < misc->size(); ++i)
    {
        Protocol::Slot* slot = misc->Mutable(i);
        int32 slotId = slot->slot_id();
        lookupTableMappings[Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS][slotId] = slot;
    }
}

bool Inventory::addItem(OUT Protocol::Slot* updatedSlot, Protocol::Item& itemInstance, int32 count)
{
    const Json& itemData = Gamedata::ItemDataTable[itemInstance.template_id()];
    Protocol::ItemType itemType = Protocol::ItemType::ITEM_TYPE_NONE;
    Protocol::SlotType slotType = Protocol::SlotType::SLOT_TYPE_NONE;
    RepeatedPtrField<Protocol::Slot>* categoryInven = nullptr;

    if (itemData["itemType"] == "armor" || itemData["itemType"] == "weapon")
    {
        itemType = Protocol::ItemType::ITEM_TYPE_GEAR;
        slotType = Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR;
        if (PlayerRef playerRef = player.lock())
            categoryInven = playerRef->playerInfo->mutable_inventory()->mutable_gear();
    }
    else if (itemData["itemType"] == "consumption")
    {
        itemType = Protocol::ItemType::ITEM_TYPE_CONSUMABLE;
        slotType = Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE;
        if (PlayerRef playerRef = player.lock())
            categoryInven = playerRef->playerInfo->mutable_inventory()->mutable_consumables();
    }
    else if (itemData["itemType"] == "miscellaneous")
    {
        itemType = Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS;
        slotType = Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC;
        if (PlayerRef playerRef = player.lock())
            categoryInven = playerRef->playerInfo->mutable_inventory()->mutable_miscellaneous();
    }
    else
    {
        return false;
    }

    if (categoryInven == nullptr)
        return false;

    int32 availableSlotId = findFirstAvailableSlotId(itemType, itemInstance.template_id());

    // 들어갈 슬롯 찾았으니 이제 진짜 추가해야함
    vector<Protocol::Slot*>& lookupTable = lookupTableMappings[itemType];
    Protocol::Slot* targetSlot = lookupTable[availableSlotId];
    dirtyFlagsMappings[itemType][availableSlotId] = true;

    if (targetSlot != nullptr)
    {
        // Modifiy slot data
        int32 updatedCount = targetSlot->count() + count;
        targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_MODIFIED);
        targetSlot->set_count(updatedCount);

        updatedSlot->CopyFrom(*targetSlot);
    }
    else
    {
        // Add new slot data
        targetSlot = categoryInven->Add();
        targetSlot->set_slot_id(availableSlotId);
        targetSlot->set_type(slotType);
        targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_ADDED);
        targetSlot->set_count(count);

        Protocol::Item* item = targetSlot->mutable_item();
        item->CopyFrom(itemInstance);
        if (itemType == Protocol::ItemType::ITEM_TYPE_GEAR && itemInstance.has_item_uid() == false)
            item->set_item_uid(GNextItemUID.fetch_add(1));
        
        updatedSlot->CopyFrom(*targetSlot);
    }

    return true;
}

bool Inventory::addItem(OUT Protocol::Slot* updatedSlot, int32 templateId, int32 count)
{
    // -> 아직 인스턴스화된 아이템이 아닐때 진입(ex. 구매한 아이템)
    Protocol::Item itemInstance;
    itemInstance.set_template_id(templateId);

    // TODO: generate inital instance data.
    // itemInstance.set_gear_info(); 초기 랜덤 데이터 넣을 때 사용(지금은 안 씀)

    if (addItem(updatedSlot, itemInstance, count) == false)
        return false;

    return true; 
}

bool Inventory::removeItem(OUT Protocol::Slot* updatedSlot, Protocol::Slot* slot, int32 count)
{
    Protocol::ItemType itemType = slotTypeToItemTypeMappings[slot->type()];
    int32 slotId = slot->slot_id();
    Protocol::Slot* targetSlot = lookupTableMappings[itemType][slotId];
    dirtyFlagsMappings[itemType][slotId] = true;

    if (!targetSlot || targetSlot->count() < count)
        return false;

    int32 updatedCount = targetSlot->count() - count;
    if (updatedCount > 0)
    {
        targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_MODIFIED);
        targetSlot->set_count(updatedCount);

        updatedSlot->CopyFrom(*targetSlot);
    }
    else
    {
        targetSlot->Clear();

        updatedSlot->set_state(Protocol::UpdateState::UPDATE_STATE_REMOVED);
    }
    
    return true;
}

int32 Inventory::findFirstAvailableSlotId(Protocol::ItemType type, int32 templateId)
{
    if (type == Protocol::ItemType::ITEM_TYPE_NONE)
    {
        cout << "findFirstAvailableSlotId() Error: Invalid ItemType" << endl;
        return -1;
    }

    const Json& itemData = Gamedata::ItemDataTable[templateId];
 
    vector<Protocol::Slot*>& categoryInven = lookupTableMappings[type];
    int32 availableSlotId = -1;
    if (type == Protocol::ItemType::ITEM_TYPE_GEAR)
    {
        // leftmost 빈 슬롯 찾기.
        auto it = std::find_if(categoryInven.begin(), categoryInven.end(), [templateId](const Protocol::Slot* slot)
            {
                return slot == nullptr;
            });

        if(it != categoryInven.end())
            availableSlotId = it - categoryInven.begin();
    }
    else
    {
        bool found = false;

        // 같은 아이템이 있는지 확인
        {
            auto it = std::find_if(categoryInven.begin(), categoryInven.end(), [templateId](const Protocol::Slot* slot)
                {
                    return slot->item().template_id() == templateId;
                });

            if (it != categoryInven.end())
            {
                availableSlotId = it - categoryInven.begin();
                found = true;
            }
        }

        if (!found)
        {
            // leftmost 빈 슬롯 찾기.
            auto it = std::find_if(categoryInven.begin(), categoryInven.end(), [templateId](const Protocol::Slot* slot)
                {
                    return slot == nullptr;
                });

            if (it != categoryInven.end())
                availableSlotId = it - categoryInven.begin();
        }
            
    }

    return availableSlotId;
}

