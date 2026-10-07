#include "Core/pch.h"
#include "Game/Inventory/Inventory.h"
#include "Game/Entities/Player.h"

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

    _bags = {
        {Protocol::ItemType::ITEM_TYPE_GEAR, Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR, inventory->mutable_gear(), vector<bool>(MAX_SLOTS)},
        {Protocol::ItemType::ITEM_TYPE_CONSUMABLE, Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE, inventory->mutable_consumables(), vector<bool>(MAX_SLOTS)},
        {Protocol::ItemType::ITEM_TYPE_MISCELLANEOUS, Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC, inventory->mutable_miscellaneous(), vector<bool>(MAX_SLOTS)}
    };
}

Inventory::~Inventory()
{
}

bool Inventory::AddItem(OUT Protocol::Slot* replicatingSlot, const Protocol::Item& itemInstance, int32 count, optional<int32> setSlotId)
{
    const Json* itemData = Gamedata::FindItemData(itemInstance.template_id());
    if (itemData == nullptr)
        return false;

    optional<Protocol::ItemType> itemType = ToItemType(*itemData);
    if (itemType.has_value() == false)
        return false;

    Bag* bag = FindBag(itemType.value());
    if (bag == nullptr)
        return false;

    int32 availableSlotId = setSlotId.has_value() ? setSlotId.value() : FindFirstAvailableSlotId(bag->itemType, itemInstance.template_id());
    if (availableSlotId == -1)
        return false;

    Protocol::Slot* targetSlot = bag->slots->Mutable(availableSlotId);
    if (targetSlot == nullptr)
        return false;

    bag->dirtyFlags[availableSlotId] = true;
    if (targetSlot->has_item())
    {
        // 이미 든 아이템에 수량을 더한다.
        targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_MODIFIED);

        Protocol::Item* item = targetSlot->mutable_item();
        int32 updatedCount = item->count() + count;
        item->set_count(updatedCount);

        if(replicatingSlot != nullptr)
            replicatingSlot->CopyFrom(*targetSlot);
    }
    else
    {
        // 빈 슬롯에 새로 넣는다.
        targetSlot->set_state(Protocol::UpdateState::UPDATE_STATE_ADDED);

        Protocol::Item* item = targetSlot->mutable_item();
        item->CopyFrom(itemInstance);
        item->set_count(count);

        if (bag->itemType == Protocol::ItemType::ITEM_TYPE_GEAR && itemInstance.has_item_uid() == false)
            item->set_item_uid(GNextItemUID.fetch_add(1));
        
        if (replicatingSlot != nullptr)
            replicatingSlot->CopyFrom(*targetSlot);
    }

    return true;
}

bool Inventory::AddItem(OUT RepeatedPtrField<Protocol::Slot>* replicatingSlots, int32 templateId, int32 count)
{
    if (count <= 0)
        return false;

    const Json* itemDataPtr = Gamedata::FindItemData(templateId);
    if (itemDataPtr == nullptr)
        return false;

    const Json& itemData = *itemDataPtr;
    optional<Protocol::ItemType> itemTypeOpt = ToItemType(itemData);
    if (itemTypeOpt.has_value() == false)
        return false;

    const Bag* bag = FindBag(itemTypeOpt.value());
    if (bag == nullptr)
        return false;

    const Protocol::ItemType itemType = bag->itemType;
    const int32 maxStack = (std::max)(1, itemData.value(JsonProperty::Item::MaxStack, 1));
    const RepeatedPtrField<Protocol::Slot>& lookupTable = *bag->slots;

    // 슬롯을 바꾸기 전에 넣을 자리를 전부 정한다. 도중에 모자라면 이미 바꾼 슬롯을 되돌릴 방법이 없다.
    vector<pair<int32, int32>> fills; // <slotId, 넣을 수량>
    int32 remaining = count;

    // 장비는 합치지 않는다. 나머지는 같은 아이템이 든 슬롯의 남은 칸부터 채운다.
    if (itemType != Protocol::ItemType::ITEM_TYPE_GEAR)
    {
        for (int32 slotId = 0; slotId < lookupTable.size() && remaining > 0; slotId++)
        {
            const Protocol::Slot& slot = lookupTable[slotId];
            if (slot.has_item() == false || slot.item().template_id() != templateId)
                continue;

            const int32 space = maxStack - slot.item().count();
            if (space <= 0)
                continue;

            const int32 amount = (std::min)(space, remaining);
            fills.emplace_back(slotId, amount);
            remaining -= amount;
        }
    }

    for (int32 slotId = 0; slotId < lookupTable.size() && remaining > 0; slotId++)
    {
        if (lookupTable[slotId].has_item())
            continue;

        const int32 amount = (std::min)(maxStack, remaining);
        fills.emplace_back(slotId, amount);
        remaining -= amount;
    }

    if (remaining > 0)
        return false;

    for (const auto& [slotId, amount] : fills)
    {
        Protocol::Item itemInstance;
        itemInstance.set_template_id(templateId);
        itemInstance.set_count(amount);

        // TODO: 초기 인스턴스 데이터 생성
        // itemInstance.set_gear_info(); 초기 랜덤 데이터 넣을 때 사용(지금은 안 씀)

        Protocol::Slot* replicatingSlot = replicatingSlots != nullptr ? replicatingSlots->Add() : nullptr;
        if (AddItem(replicatingSlot, itemInstance, amount, slotId) == false)
            return false;
    }

    return true;
}

bool Inventory::RemoveItem(const Protocol::Slot& requestSlot, OUT Protocol::Slot* replicatingSlot, int32 count)
{
    // requestSlot의 type과 slot_id는 클라이언트가 보낸 값이 그대로 들어온다.
    // 인덱싱에 닿기 전에 거르지 않으면 널 역참조와 범위 밖 접근으로 프로세스가 죽는다.
    Bag* bag = FindBag(requestSlot.type());
    if (bag == nullptr)
    {
        cout << "RemoveItem() Error: 저장소가 없는 SlotType(" << requestSlot.type() << ")" << endl;
        return false;
    }

    int32 slotId = requestSlot.slot_id();
    if (IsValidSlotId(slotId) == false)
    {
        cout << "RemoveItem() Error: 범위 밖 slot_id(" << slotId << ")" << endl;
        return false;
    }

    // 슬롯 해석은 GetSlot과 같이 FindBag 하나를 거친다. 두 함수가 같은 입력을 다르게
    // 해석하면 슬롯은 한 저장소에서 꺼내고 더티 플래그는 다른 저장소에 찍게 된다.
    Protocol::Slot* updatedSlot = bag->slots->Mutable(slotId);
    if (updatedSlot == nullptr)
        return false;

    // 검증이 먼저다. mutable_item()은 없던 item을 만들면서 has_item()을 켜므로,
    // 검증보다 먼저 부르면 빈 슬롯이 "아이템 있음"으로 오염돼 다시는 채워지지 않는다.
    // 더티 플래그도 마찬가지다. 실패한 제거까지 더티로 만들면 불필요한 DB 저장·복제가 따라온다.
    if (updatedSlot->has_item() == false || updatedSlot->item().count() < count)
        return false;

    bag->dirtyFlags[slotId] = true;

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

    Bag* bag = FindBag(type);
    if (bag == nullptr)
        return -1;

    RepeatedPtrField<Protocol::Slot>* lookupTable = bag->slots;
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

vector<bool>* Inventory::GetDirtyFlags(Protocol::ItemType itemType)
{
    Bag* bag = FindBag(itemType);
    return bag != nullptr ? &bag->dirtyFlags : nullptr;
}

Inventory::Bag* Inventory::FindBag(Protocol::ItemType itemType)
{
    for (Bag& bag : _bags)
    {
        if (bag.itemType == itemType)
            return &bag;
    }

    return nullptr;
}

Inventory::Bag* Inventory::FindBag(Protocol::SlotType slotType)
{
    for (Bag& bag : _bags)
    {
        if (bag.slotType == slotType)
            return &bag;
    }

    return nullptr;
}

optional<Protocol::ItemType> Inventory::ToItemType(const Json& itemData)
{
    if (itemData.is_object() == false)
        return nullopt;

    auto fieldIt = itemData.find(string(JsonProperty::Item::ItemType));
    if (fieldIt == itemData.end() || fieldIt->is_string() == false)
        return nullopt;

    // 기획 원본은 열거형 이름에서 접두사를 뺀 값(GEAR)을 적는다. 이름 표는 protobuf가 만든 것을 쓴다.
    Protocol::ItemType itemType;
    if (Protocol::ItemType_Parse(string(ITEM_TYPE_NAME_PREFIX) + fieldIt->get<string>(), &itemType) == false)
        return nullopt;

    if (itemType == Protocol::ItemType::ITEM_TYPE_NONE)
        return nullopt;

    return itemType;
}

Protocol::Slot* Inventory::GetSlot(Protocol::SlotType type, int32 slot_id)
{
    // 슬롯 해석의 단일 창구다. RemoveItem도 같은 FindBag을 거친다.
    // 인덱싱 전에 거르고, 거부는 널로 알린다. Mutable()의 범위 검사는 DCHECK라
    // Release에서 빠지므로, 실제로 안전을 보장하는 것은 아래 검증들이다.
    Bag* bag = FindBag(type);
    if (bag == nullptr)
        return nullptr;

    if (IsValidSlotId(slot_id) == false)
        return nullptr;

    return bag->slots->Mutable(slot_id);
}

void Inventory::ClearDirtyFlags()
{
    for (Bag& bag : _bags)
        std::fill(bag.dirtyFlags.begin(), bag.dirtyFlags.end(), false);
}

