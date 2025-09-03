#include "pch.h"
#include "Inventory.h"

Inventory::Inventory()
{
    _gear.resize(MAX_SLOTS);
    _consumables.resize(MAX_SLOTS);
    _miscellaneous.resize(MAX_SLOTS);

    _gearDirtyFlags.resize(MAX_SLOTS);
    _consumablesDirtyFlags.resize(MAX_SLOTS);
    _miscellaneousDirtyFlags.resize(MAX_SLOTS);
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

void Inventory::addItem(Protocol::Slot* updatedSlots, int32 templateId, int32 count)
{
    Protocol::Item item;

    // 1. templateId를 통해 해당 아이템이 무슨 아이템 타입인지 알아낸다.
    // 2-1. 장비라면 GearInfo를 채우고(짜피 고유 id밖에 없음), 새로운 슬롯에 넣는다.
    // 2-2  stackable 이라면, 가장 쌓을수 있는 가장 왼쪽에 쌓는다. 없으면 새슬롯
    // 3. 만든 item을 저장한다.
}

void Inventory::removeItem(Protocol::Slot* targetSlot, Protocol::Slot* updatedSlots, int32 count)
{
}

