#include "pch.h"
#include "Inventory.h"

Inventory::Inventory()
{
}

Inventory::~Inventory()
{
}

void Inventory::Init(vector<Protocol::Item>& gear, vector<Protocol::Item>& consumables, vector<Protocol::Item>& miscellaneous)
{
    _gear = std::move(gear);
    _consumables = std::move(consumables);
    _miscellaneous = std::move(miscellaneous);
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

