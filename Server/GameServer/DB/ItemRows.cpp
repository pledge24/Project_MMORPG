#include "Core/pch.h"
#include "DB/ItemRows.h"
#include "Game/Items/InventorySections.h"

optional<PlacedItem> ItemRows::ToPlacedItem(const ItemRow& row)
{
    PlacedItem placed;
    placed.address.slotIndex = row.slotIndex;

    switch (row.container)
    {
    case ItemRowContainer::INVENTORY:
    {
        // 구역은 아이템 종류다. 아이템 표의 종류와 맞는지는 입장 검증이 본다.
        optional<ContainerType> section = InventorySections::FindContainer(row.itemType);
        if (section.has_value() == false)
            return nullopt;

        placed.address.container = section.value();
        break;
    }
    case ItemRowContainer::EQUIPPED:
    {
        // 착용 장비에는 장비만 들고, 칸 번호가 장비 부위다.
        if (row.itemType != Protocol::ITEM_TYPE_GEAR)
            return nullopt;

        placed.address.container = ContainerType::EQUIPPED;
        break;
    }
    default:
        return nullopt;
    }

    placed.item.itemUid = row.itemUid;
    placed.item.templateId = row.templateId;
    placed.item.count = row.stackCount;
    if (row.itemType == Protocol::ITEM_TYPE_GEAR)
        placed.item.gear = GearDetail{ row.enhanceLevel, row.durability };

    return placed;
}

optional<ItemRow> ItemRows::ToRow(const PlacedItem& placed)
{
    ItemRow row;
    if (placed.address.container == ContainerType::EQUIPPED)
    {
        row.container = ItemRowContainer::EQUIPPED;
        row.itemType = Protocol::ITEM_TYPE_GEAR;
    }
    else
    {
        optional<Protocol::ItemType> itemType = InventorySections::FindItemType(placed.address.container);
        if (itemType.has_value() == false)
            return nullopt;

        row.container = ItemRowContainer::INVENTORY;
        row.itemType = itemType.value();
    }

    const GearDetail gear = placed.item.gear.value_or(GearDetail{});

    row.itemUid = placed.item.itemUid;
    row.templateId = placed.item.templateId;
    row.slotIndex = placed.address.slotIndex;
    row.stackCount = placed.item.count;
    row.enhanceLevel = gear.enhanceLevel;
    row.durability = gear.durability;
    return row;
}
