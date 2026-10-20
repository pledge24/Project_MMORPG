#include "Core/pch.h"
#include "DB/ItemRows.h"

namespace
{
    /** 인벤토리 구역 하나와 그 구역을 나타내는 패킷의 칸 종류와 칸 목록. */
    struct InventorySection
    {
        Protocol::ItemType itemType;
        Protocol::SlotType slotType;
        RepeatedPtrField<Protocol::Slot>* (Protocol::Inventory::*mutableSlots)();
        const RepeatedPtrField<Protocol::Slot>& (Protocol::Inventory::*slots)() const;
    };

    // 불러오기와 저장은 구역과 패킷 칸 종류의 대응을 이 표 하나로 정한다. 다른 표를 쓰면 아이템이 다른 구역으로 옮겨 간다.
    // InventoryComponent의 구역 목록도 같은 대응을 따로 적으므로, 대응을 바꾸면 그쪽도 함께 고친다.
    constexpr InventorySection INVENTORY_SECTIONS[] = {
        { Protocol::ITEM_TYPE_GEAR, Protocol::SLOT_TYPE_INVENTORY_GEAR, &Protocol::Inventory::mutable_gear, &Protocol::Inventory::gear },
        { Protocol::ITEM_TYPE_CONSUMABLE, Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE, &Protocol::Inventory::mutable_consumables, &Protocol::Inventory::consumables },
        { Protocol::ITEM_TYPE_MISCELLANEOUS, Protocol::SLOT_TYPE_INVENTORY_MISC, &Protocol::Inventory::mutable_miscellaneous, &Protocol::Inventory::miscellaneous },
    };

    const InventorySection* FindSection(Protocol::ItemType itemType)
    {
        for (const InventorySection& section : INVENTORY_SECTIONS)
        {
            if (section.itemType == itemType)
                return &section;
        }

        return nullptr;
    }

    Protocol::Slot MakeSlot(const ItemRow& row, Protocol::SlotType slotType)
    {
        Protocol::Slot slot;
        slot.set_type(slotType);
        slot.set_slot_id(row.slotIndex);

        Protocol::Item* item = slot.mutable_item();
        item->set_item_uid(row.itemUid);
        item->set_template_id(row.templateId);
        item->set_count(row.stackCount);

        // 추가 공격력은 DB에 없다. 패킷에는 0이 들어간다.
        if (row.itemType == Protocol::ITEM_TYPE_GEAR)
        {
            Protocol::GearInfo* gearInfo = item->mutable_gearinfo();
            gearInfo->set_enhance_level(row.enhanceLevel);
            gearInfo->set_durability(row.durability);
        }

        return slot;
    }

    ItemRow MakeRow(const Protocol::Slot& slot, ItemContainer container, Protocol::ItemType itemType, int32 slotIndex)
    {
        const Protocol::Item& item = slot.item();

        ItemRow row;
        row.itemUid = item.item_uid();
        row.templateId = item.template_id();
        row.itemType = itemType;
        row.container = container;
        row.slotIndex = slotIndex;
        row.stackCount = item.count();
        row.enhanceLevel = item.gearinfo().enhance_level();
        row.durability = item.gearinfo().durability();
        return row;
    }
}

bool ItemRows::AddToPossession(const ItemRow& row, OUT Protocol::Possession& possession)
{
    switch (row.container)
    {
    case ItemContainer::INVENTORY:
    {
        // 구역은 아이템 종류다. 아이템 표의 종류와 맞는지는 입장 검증이 본다.
        const InventorySection* section = FindSection(row.itemType);
        if (section == nullptr)
            return false;

        *(possession.mutable_inventory()->*section->mutableSlots)()->Add() = MakeSlot(row, section->slotType);
        return true;
    }
    case ItemContainer::EQUIPPED:
    {
        // 착용 장비에는 장비만 들고, 칸 번호가 장비 부위다.
        if (row.itemType != Protocol::ITEM_TYPE_GEAR)
            return false;

        auto* equippedGear = possession.mutable_equipped_gear();
        if (equippedGear->contains(row.slotIndex))
            return false;

        (*equippedGear)[row.slotIndex] = MakeSlot(row, Protocol::SLOT_TYPE_EQUIPPED);
        return true;
    }
    }

    return false;
}

vector<ItemRow> ItemRows::FromPossession(const Protocol::Possession& possession)
{
    vector<ItemRow> rows;

    for (const InventorySection& section : INVENTORY_SECTIONS)
    {
        for (const Protocol::Slot& slot : (possession.inventory().*section.slots)())
        {
            if (slot.has_item())
                rows.push_back(MakeRow(slot, ItemContainer::INVENTORY, section.itemType, slot.slot_id()));
        }
    }

    // 착용 장비는 칸 번호가 장비 부위다. 맵의 순회 순서는 정해져 있지 않으므로 부위 순서로 정렬한다.
    vector<ItemRow> equippedRows;
    for (const auto& [gearType, slot] : possession.equipped_gear())
    {
        if (slot.has_item())
            equippedRows.push_back(MakeRow(slot, ItemContainer::EQUIPPED, Protocol::ITEM_TYPE_GEAR, gearType));
    }
    ranges::sort(equippedRows, {}, &ItemRow::slotIndex);
    rows.insert(rows.end(), equippedRows.begin(), equippedRows.end());

    return rows;
}
