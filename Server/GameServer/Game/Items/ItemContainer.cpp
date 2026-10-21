#include "Core/pch.h"
#include "Game/Items/ItemContainer.h"

ItemContainer::ItemContainer(ContainerType type, int32 slotCount) : _type(type)
{
    for (int32 slotIndex = 0; slotIndex < slotCount; slotIndex++)
        _slots.emplace(slotIndex, nullopt);
}

ItemContainer::ItemContainer(ContainerType type, const vector<int32>& slotIndices) : _type(type)
{
    for (int32 slotIndex : slotIndices)
        _slots.emplace(slotIndex, nullopt);
}

vector<int32> ItemContainer::GetSlotIndices() const
{
    vector<int32> slotIndices;
    slotIndices.reserve(_slots.size());
    for (const auto& [slotIndex, item] : _slots)
        slotIndices.push_back(slotIndex);

    return slotIndices;
}

const ItemInstance* ItemContainer::Find(int32 slotIndex) const
{
    auto slotIt = _slots.find(slotIndex);
    if (slotIt == _slots.end() || slotIt->second.has_value() == false)
        return nullptr;

    return &slotIt->second.value();
}

optional<int32> ItemContainer::FindEmptySlot(int32 fromSlotIndex) const
{
    for (auto slotIt = _slots.lower_bound(fromSlotIndex); slotIt != _slots.end(); ++slotIt)
    {
        if (slotIt->second.has_value() == false)
            return slotIt->first;
    }

    return nullopt;
}

optional<int32> ItemContainer::FindFirstSlotOf(int32 templateId) const
{
    for (const auto& [slotIndex, item] : _slots)
    {
        if (item.has_value() && item->templateId == templateId)
            return slotIndex;
    }

    return nullopt;
}

bool ItemContainer::Put(int32 slotIndex, const ItemInstance& item)
{
    auto slotIt = _slots.find(slotIndex);
    if (slotIt == _slots.end() || slotIt->second.has_value() || item.count < 1)
        return false;

    slotIt->second = item;
    MarkChanged(slotIndex);
    return true;
}

optional<ItemInstance> ItemContainer::Take(int32 slotIndex)
{
    auto slotIt = _slots.find(slotIndex);
    if (slotIt == _slots.end() || slotIt->second.has_value() == false)
        return nullopt;

    optional<ItemInstance> taken = std::move(slotIt->second);
    slotIt->second.reset();
    MarkChanged(slotIndex);
    return taken;
}

bool ItemContainer::AddCount(int32 slotIndex, int32 amount)
{
    auto slotIt = _slots.find(slotIndex);
    if (slotIt == _slots.end() || slotIt->second.has_value() == false || amount < 1)
        return false;

    slotIt->second->count += amount;
    MarkChanged(slotIndex);
    return true;
}

bool ItemContainer::RemoveCount(int32 slotIndex, int32 amount)
{
    auto slotIt = _slots.find(slotIndex);
    if (slotIt == _slots.end() || slotIt->second.has_value() == false || amount < 1)
        return false;

    if (slotIt->second->count < amount)
        return false;

    slotIt->second->count -= amount;
    if (slotIt->second->count == 0)
        slotIt->second.reset();

    MarkChanged(slotIndex);
    return true;
}

void ItemContainer::TakeChanges(OUT vector<SlotChange>& changes)
{
    // 바뀐 순간이 아니라 꺼내는 지금의 내용을 싣는다. 한 요청 안에서 한 칸이 여러 번 바뀌어도 응답에는 마지막 내용 하나만 간다.
    for (int32 slotIndex : _changedSlots)
        changes.push_back(SlotChange{ SlotAddress{ _type, slotIndex }, _slots.at(slotIndex) });

    _changedSlots.clear();
}

void ItemContainer::MarkChanged(int32 slotIndex)
{
    if (ranges::find(_changedSlots, slotIndex) == _changedSlots.end())
        _changedSlots.push_back(slotIndex);
}
