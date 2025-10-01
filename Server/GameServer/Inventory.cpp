#include "pch.h"
#include "Inventory.h"
#include "Global.h"
#include "Player.h"

Inventory::Inventory(PlayerRef player) : _player(player)
{
    Protocol::Inventory* inventory = player->playerInfo->mutable_inventory();
    
    for (int32 slotId = 0; slotId < MAX_SLOTS; slotId++)
    {
        Protocol::Slot* slotGear = inventory->add_gear();
        Protocol::Slot* slotConsumables = inventory->add_consumables();
        Protocol::Slot* slotMisc = inventory->add_miscellaneous();

        slotGear->set_slot_id(slotId);
        slotGear->set_type(Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR);
        slotGear->set_state(Protocol::UpdateState::UPDATE_STATE_NONE);

        slotConsumables->set_slot_id(slotId);
        slotConsumables->set_type(Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE);
        slotConsumables->set_state(Protocol::UpdateState::UPDATE_STATE_NONE);

        slotMisc->set_slot_id(slotId);
        slotMisc->set_type(Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC);
        slotMisc->set_state(Protocol::UpdateState::UPDATE_STATE_NONE);
    }

    inventorylookupMappings = {
        {Protocol::ItemType::ITEM_TYPE_GEAR, inventory->mutable_gear()},
        {Protocol::ItemType::ITEM_TYPE_CONSUMABLE, inventory->mutable_consumables()},
        {Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS, inventory->mutable_miscellaneous()}
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

    itemTypeMappings = {
        {"armor", Protocol::ItemType::ITEM_TYPE_GEAR},
        {"weapon", Protocol::ItemType::ITEM_TYPE_GEAR},
        {"consumption", Protocol::ItemType::ITEM_TYPE_CONSUMABLE},
        {"miscellaneous", Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS},
    };
}

Inventory::~Inventory()
{
}

bool Inventory::addItem(OUT Protocol::Slot* repSlot, Protocol::Item& itemInstance, int32 count, optional<int32> setSlotId)
{
    //if (itemInstance.template_id() == 0)
    //    return false;

    const Json& itemData = Gamedata::ItemDataTable[itemInstance.template_id()];

    if (itemTypeMappings.find(itemData["itemType"]) == itemTypeMappings.end())
        return false;

    Protocol::ItemType itemType = itemTypeMappings[itemData["itemType"]];
    if (inventorylookupMappings.find(itemType) == inventorylookupMappings.end())
        return false;

    int32 availableSlotId = setSlotId.has_value() ? setSlotId.value() : findFirstAvailableSlotId(itemType, itemInstance.template_id());
    if (availableSlotId == -1)
        return false;

    // 들어갈 슬롯 찾았으니 이제 진짜 추가해야함
    RepeatedPtrField<Protocol::Slot>* lookupTable = inventorylookupMappings[itemType];
    Protocol::Slot* targetSlot = lookupTable->Mutable(availableSlotId);
    if (targetSlot == nullptr)
        return false;

    dirtyFlagsMappings[itemType][availableSlotId] = true;
    if (targetSlot->has_item())
    {
        // Modifiy slot data
        targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_MODIFIED);

        Protocol::Item* item = targetSlot->mutable_item();
        int32 updatedCount = item->count() + count;
        item->set_count(updatedCount);

        if(repSlot != nullptr)
            repSlot->CopyFrom(*targetSlot);
    }
    else
    {
        // Add new slot data
        targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_ADDED);

        Protocol::Item* item = targetSlot->mutable_item();
        item->CopyFrom(itemInstance);
        item->set_count(count);

        if (itemType == Protocol::ItemType::ITEM_TYPE_GEAR && itemInstance.has_item_uid() == false)
            item->set_item_uid(GNextItemUID.fetch_add(1));
        
        if (repSlot != nullptr)
            repSlot->CopyFrom(*targetSlot);
    }

    return true;
}

bool Inventory::addItem(OUT Protocol::Slot* repSlot, int32 templateId, int32 count)
{
    // -> 아직 인스턴스화된 아이템이 아닐때 진입(ex. 구매한 아이템)
    Protocol::Item itemInstance;
    itemInstance.set_template_id(templateId);
    itemInstance.set_count(count);

    // TODO: generate inital instance data.
    // itemInstance.set_gear_info(); 초기 랜덤 데이터 넣을 때 사용(지금은 안 씀)

    if (addItem(repSlot, itemInstance, count) == false)
        return false;

    return true; 
}

bool Inventory::removeItem(OUT Protocol::Slot* repSlot, Protocol::Slot* slot, int32 count)
{
    Protocol::ItemType itemType = slotTypeToItemTypeMappings[slot->type()];
    int32 slotId = slot->slot_id();
    Protocol::Slot* targetSlot = inventorylookupMappings[itemType]->Mutable(slotId);
    Protocol::Item* item = targetSlot->mutable_item();

    dirtyFlagsMappings[itemType][slotId] = true;

    if (targetSlot->has_item() == false || item->count() < count)
        return false;

    int32 updatedCount = item->count() - count;
    if (updatedCount > 0)
    {
        targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_MODIFIED);
        item->set_count(updatedCount);

        repSlot->CopyFrom(*targetSlot);
    }
    else
    {
        targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_REMOVED);
        targetSlot->clear_item();
        
        repSlot->CopyFrom(*targetSlot);
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
 
    RepeatedPtrField<Protocol::Slot>* lookupTable = inventorylookupMappings[type];
    int32 availableSlotId = -1;
    if (type == Protocol::ItemType::ITEM_TYPE_GEAR)
    {
        // leftmost 빈 슬롯 찾기.
        auto it = std::find_if(lookupTable->begin(), lookupTable->end(), [templateId](const Protocol::Slot& slot)
            {
                return slot.has_item() == false;
            });

        if(it != lookupTable->end())
            availableSlotId = (int32)(it - lookupTable->begin());
    }
    else
    {
        bool found = false;

        // 같은 아이템이 있는지 확인
        {
            auto it = std::find_if(lookupTable->begin(), lookupTable->end(), [templateId](const Protocol::Slot& slot)
                {
                    return slot.item().template_id() == templateId;
                });

            if (it != lookupTable->end())
            {
                availableSlotId = (int32)(it - lookupTable->begin());
                found = true;
            }
        }

        if (!found)
        {
            // leftmost 빈 슬롯 찾기.
            auto it = std::find_if(lookupTable->begin(), lookupTable->end(), [templateId](const Protocol::Slot& slot)
                {
                    return slot.has_item() == false;
                });

            if (it != lookupTable->end())
                availableSlotId = (int32)(it - lookupTable->begin());
        }
            
    }

    return availableSlotId;
}

void Inventory::ClearDirtyFlags()
{
    for (auto& mappingsPair : dirtyFlagsMappings)
    {
        vector<bool>& dirtyFlag = mappingsPair.second;
        std::fill(dirtyFlag.begin(), dirtyFlag.end(), false);
    }
}

