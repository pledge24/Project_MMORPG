#include "Core/pch.h"
#include "Game/Inventory/InventoryComponent.h"
#include "Game/Entities/Player.h"

InventoryComponent::InventoryComponent(PlayerRef owner, Protocol::Inventory* inventory) : EntityComponent(owner)
{
    
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

InventoryComponent::~InventoryComponent()
{
}

bool InventoryComponent::AddItem(OUT Protocol::Slot* replicatingSlot, const Protocol::Item& itemInstance, int32 count, optional<int32> setSlotId)
{
    const ItemTemplate* itemTemplate = Gamedata::FindItem(itemInstance.template_id());
    if (itemTemplate == nullptr)
        return false;

    Bag* bag = FindBag(itemTemplate->itemType);
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

bool InventoryComponent::AddItem(OUT RepeatedPtrField<Protocol::Slot>* replicatingSlots, int32 templateId, int32 count)
{
    if (count <= 0)
        return false;

    const ItemTemplate* itemTemplate = Gamedata::FindItem(templateId);
    if (itemTemplate == nullptr)
        return false;

    const Bag* bag = FindBag(itemTemplate->itemType);
    if (bag == nullptr)
        return false;

    const Protocol::ItemType itemType = bag->itemType;
    const int32 maxStack = itemTemplate->maxStack;
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

    for (int32 slotId = FindEmptySlotId(*bag); slotId != -1 && remaining > 0; slotId = FindEmptySlotId(*bag, slotId + 1))
    {
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

bool InventoryComponent::LoadItem(const Protocol::Slot& loadedSlot)
{
    Bag* bag = FindBag(loadedSlot.type());
    if (bag == nullptr || IsValidSlotId(loadedSlot.slot_id()) == false)
        return false;

    // 같은 칸의 두 행을 합치면 다음 저장이 한 행만 남긴다.
    if (bag->slots->Get(loadedSlot.slot_id()).has_item())
        return false;

    const Protocol::Item& item = loadedSlot.item();
    const ItemTemplate* itemTemplate = Gamedata::FindItem(item.template_id());
    if (itemTemplate == nullptr || itemTemplate->itemType != bag->itemType)
        return false;

    if (item.count() < 1 || item.count() > itemTemplate->maxStack)
        return false;

    return AddItem(nullptr, item, item.count(), loadedSlot.slot_id());
}

bool InventoryComponent::RemoveItem(const Protocol::Slot& requestSlot, OUT Protocol::Slot* replicatingSlot, int32 count)
{
    // requestSlot의 type과 slot_id는 클라이언트가 보낸 값이 그대로 들어온다.
    // 인덱싱에 닿기 전에 거르지 않으면 널 역참조와 범위 밖 접근으로 프로세스가 죽는다.
    Bag* bag = FindBag(requestSlot.type());
    if (bag == nullptr)
    {
        GLogger->Warning("RemoveItem: 저장소가 없는 SlotType({})", static_cast<int32>(requestSlot.type()));
        return false;
    }

    int32 slotId = requestSlot.slot_id();
    if (IsValidSlotId(slotId) == false)
    {
        GLogger->Warning("RemoveItem: 범위 밖 slot_id({})", slotId);
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

int32 InventoryComponent::FindFirstAvailableSlotId(Protocol::ItemType type, int32 templateId)
{
    if (type == Protocol::ItemType::ITEM_TYPE_NONE)
    {
        GLogger->Warning("FindFirstAvailableSlotId: 아이템 종류가 없다");
        return -1;
    }

    const Bag* bag = FindBag(type);
    if (bag == nullptr)
        return -1;

    // 장비는 합치지 않는다. 나머지는 같은 아이템이 든 슬롯을 먼저 고른다.
    if (type != Protocol::ItemType::ITEM_TYPE_GEAR)
    {
        const RepeatedPtrField<Protocol::Slot>& lookupTable = *bag->slots;
        for (int32 slotId = 0; slotId < lookupTable.size(); slotId++)
        {
            if (lookupTable[slotId].has_item() && lookupTable[slotId].item().template_id() == templateId)
                return slotId;
        }
    }

    return FindEmptySlotId(*bag);
}

bool InventoryComponent::IsCoolingDown(int32 templateId, uint64 nowMs) const
{
    const ItemTemplate* itemTemplate = Gamedata::FindItem(templateId);
    if (itemTemplate == nullptr)
        return false;

    auto lastUseIt = _lastUseTimeMs.find(templateId);
    return lastUseIt != _lastUseTimeMs.end() && nowMs < lastUseIt->second + itemTemplate->cooldownMs;
}

void InventoryComponent::StartCooldown(int32 templateId, uint64 nowMs)
{
    _lastUseTimeMs[templateId] = nowMs;
}

vector<bool>* InventoryComponent::GetDirtyFlags(Protocol::ItemType itemType)
{
    Bag* bag = FindBag(itemType);
    return bag != nullptr ? &bag->dirtyFlags : nullptr;
}

Protocol::Slot* InventoryComponent::GetSlot(Protocol::SlotType type, int32 slot_id)
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

const Protocol::Slot* InventoryComponent::GetSlot(Protocol::SlotType type, int32 slot_id) const
{
    return const_cast<InventoryComponent*>(this)->GetSlot(type, slot_id);
}

const vector<bool>* InventoryComponent::GetDirtyFlags(Protocol::ItemType itemType) const
{
    return const_cast<InventoryComponent*>(this)->GetDirtyFlags(itemType);
}

void InventoryComponent::ClearDirtyFlags()
{
    for (Bag& bag : _bags)
        std::fill(bag.dirtyFlags.begin(), bag.dirtyFlags.end(), false);
}

InventoryComponent::Bag* InventoryComponent::FindBag(Protocol::ItemType itemType)
{
    for (Bag& bag : _bags)
    {
        if (bag.itemType == itemType)
            return &bag;
    }

    return nullptr;
}

InventoryComponent::Bag* InventoryComponent::FindBag(Protocol::SlotType slotType)
{
    for (Bag& bag : _bags)
    {
        if (bag.slotType == slotType)
            return &bag;
    }

    return nullptr;
}

int32 InventoryComponent::FindEmptySlotId(const Bag& bag, int32 fromSlotId)
{
    const RepeatedPtrField<Protocol::Slot>& lookupTable = *bag.slots;
    for (int32 slotId = fromSlotId; slotId < lookupTable.size(); slotId++)
    {
        if (lookupTable[slotId].has_item() == false)
            return slotId;
    }

    return -1;
}
