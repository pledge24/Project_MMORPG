#include "pch.h"
#include "Inventory.h"
#include "Global.h"
#include "Player.h"

Inventory::Inventory(PlayerRef player) : _player(player)
{
    Protocol::Inventory* inventory = player->_possession->mutable_inventory();
    
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

    _inventoryLookupMappings = {
        {Protocol::ItemType::ITEM_TYPE_GEAR, inventory->mutable_gear()},
        {Protocol::ItemType::ITEM_TYPE_CONSUMABLE, inventory->mutable_consumables()},
        {Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS, inventory->mutable_miscellaneous()}
    };

    _dirtyFlagsMappings = {
        {Protocol::ItemType::ITEM_TYPE_GEAR, vector<bool>(MAX_SLOTS)},
        {Protocol::ItemType::ITEM_TYPE_CONSUMABLE, vector<bool>(MAX_SLOTS)},
        {Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS, vector<bool>(MAX_SLOTS)}
    };

    _slotTypeToItemTypeMappings = {
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR, Protocol::ItemType::ITEM_TYPE_GEAR},
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE, Protocol::ItemType::ITEM_TYPE_CONSUMABLE},
        {Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC, Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS}
    };

    _itemTypeMappings = {
        {"armor", Protocol::ItemType::ITEM_TYPE_GEAR},
        {"weapon", Protocol::ItemType::ITEM_TYPE_GEAR},
        {"consumption", Protocol::ItemType::ITEM_TYPE_CONSUMABLE},
        {"miscellaneous", Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS},
    };
}

Inventory::~Inventory()
{
}

bool Inventory::AddItem(OUT Protocol::Slot* replicatingSlot, const Protocol::Item& itemInstance, int32 count, optional<int32> setSlotId)
{
    //if (itemInstance.template_id() == 0)
    //    return false;

    const Json& itemData = Gamedata::s_itemDataTable[itemInstance.template_id()];

    if (_itemTypeMappings.find(itemData[JsonProperty::Item::ItemType]) == _itemTypeMappings.end())
        return false;

    Protocol::ItemType itemType = _itemTypeMappings[itemData[JsonProperty::Item::ItemType]];
    if (_inventoryLookupMappings.find(itemType) == _inventoryLookupMappings.end())
        return false;

    int32 availableSlotId = setSlotId.has_value() ? setSlotId.value() : FindFirstAvailableSlotId(itemType, itemInstance.template_id());
    if (availableSlotId == -1)
        return false;

    // 들어갈 슬롯 찾았으니 이제 진짜 추가해야함
    RepeatedPtrField<Protocol::Slot>* lookupTable = _inventoryLookupMappings[itemType];
    Protocol::Slot* targetSlot = lookupTable->Mutable(availableSlotId);
    if (targetSlot == nullptr)
        return false;

    _dirtyFlagsMappings[itemType][availableSlotId] = true;
    if (targetSlot->has_item())
    {
        // Modifiy slot data
        targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_MODIFIED);

        Protocol::Item* item = targetSlot->mutable_item();
        int32 updatedCount = item->count() + count;
        item->set_count(updatedCount);

        if(replicatingSlot != nullptr)
            replicatingSlot->CopyFrom(*targetSlot);
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
        
        if (replicatingSlot != nullptr)
            replicatingSlot->CopyFrom(*targetSlot);
    }

    return true;
}

bool Inventory::AddItem(OUT Protocol::Slot* replicatingSlot, int32 templateId, int32 count)
{
    // -> 아직 인스턴스화된 아이템이 아닐때 진입(ex. 구매한 아이템)
    Protocol::Item itemInstance;
    itemInstance.set_template_id(templateId);
    itemInstance.set_count(count);

    // TODO: generate inital instance data.
    // itemInstance.set_gear_info(); 초기 랜덤 데이터 넣을 때 사용(지금은 안 씀)

    if (AddItem(replicatingSlot, itemInstance, count) == false)
        return false;

    return true; 
}

bool Inventory::RemoveItem(const Protocol::Slot& requestSlot, OUT Protocol::Slot* replicatingSlot, int32 count)
{
    // requestSlot의 type과 slot_id는 클라이언트가 보낸 값이 그대로 들어온다.
    // 인덱싱에 닿기 전에 거르지 않으면 널 역참조와 범위 밖 접근으로 프로세스가 죽는다.
    optional<Protocol::ItemType> requestedItemType = ToItemType(requestSlot.type());
    if (requestedItemType.has_value() == false)
    {
        cout << "RemoveItem() Error: 매핑 표에 없는 SlotType(" << requestSlot.type() << ")" << endl;
        return false;
    }

    int32 slotId = requestSlot.slot_id();
    if (IsValidSlotId(slotId) == false)
    {
        cout << "RemoveItem() Error: 범위 밖 slot_id(" << slotId << ")" << endl;
        return false;
    }

    // 슬롯 해석은 GetSlot 하나로 모은다. 두 함수가 같은 입력을 다르게 해석하면
    // 이 티켓이 막으려는 사고가 그대로 돌아온다.
    Protocol::ItemType itemType = requestedItemType.value();
    Protocol::Slot* updatedSlot = GetSlot(requestSlot.type(), slotId);
    if (updatedSlot == nullptr)
        return false;

    // 검증이 먼저다. mutable_item()은 없던 item을 만들면서 has_item()을 켜므로,
    // 검증보다 먼저 부르면 빈 슬롯이 "아이템 있음"으로 오염돼 다시는 채워지지 않는다.
    // 더티 플래그도 마찬가지 — 실패한 제거까지 더티로 만들면 불필요한 DB 저장·복제가 따라온다.
    if (updatedSlot->has_item() == false || updatedSlot->item().count() < count)
        return false;

    _dirtyFlagsMappings[itemType][slotId] = true;

    Protocol::Item* item = updatedSlot->mutable_item();
    int32 updatedCount = item->count() - count;
    if (updatedCount > 0)
    {
        updatedSlot->set_state(Protocol::UpdateState::UPDATE_STATE_MODIFIED);
        item->set_count(updatedCount);

        replicatingSlot->CopyFrom(*updatedSlot);
    }
    else
    {
        updatedSlot->set_state(Protocol::UpdateState::UPDATE_STATE_REMOVED);
        updatedSlot->clear_item();
        
        replicatingSlot->CopyFrom(*updatedSlot);
    }
    
    return true;
}

int32 Inventory::FindFirstAvailableSlotId(Protocol::ItemType type, int32 templateId)
{
    if (type == Protocol::ItemType::ITEM_TYPE_NONE)
    {
        cout << "FindFirstAvailableSlotId() Error: Invalid ItemType" << endl;
        return -1;
    }

    RepeatedPtrField<Protocol::Slot>* lookupTable = _inventoryLookupMappings[type];
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

optional<Protocol::ItemType> Inventory::ToItemType(Protocol::SlotType slotType) const
{
    // operator[]로 조회하면 없는 키를 표에 삽입하면서 ITEM_TYPE_NONE을 돌려준다.
    // 조회는 반드시 find로 한다.
    auto it = _slotTypeToItemTypeMappings.find(slotType);
    if (it == _slotTypeToItemTypeMappings.end())
        return nullopt;

    return it->second;
}

Protocol::Slot* Inventory::GetSlot(Protocol::SlotType type, int32 slot_id)
{
    // 슬롯 해석의 단일 창구다. RemoveItem도 여기로 들어온다.
    // 인덱싱 전에 거르고, 거부는 널로 알린다. Mutable()의 범위 검사는 DCHECK라
    // Release에서 빠지므로, 실제로 안전을 보장하는 것은 아래 검증들이다.
    optional<Protocol::ItemType> requestedItemType = ToItemType(type);
    if (requestedItemType.has_value() == false)
        return nullptr;

    if (IsValidSlotId(slot_id) == false)
        return nullptr;

    auto lookupIt = _inventoryLookupMappings.find(requestedItemType.value());
    if (lookupIt == _inventoryLookupMappings.end())
        return nullptr;

    return lookupIt->second->Mutable(slot_id);
}

void Inventory::ClearDirtyFlags()
{
    for (auto& mappingsPair : _dirtyFlagsMappings)
    {
        vector<bool>& dirtyFlag = mappingsPair.second;
        std::fill(dirtyFlag.begin(), dirtyFlag.end(), false);
    }
}

