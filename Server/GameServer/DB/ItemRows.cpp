#include "Core/pch.h"
#include "DB/ItemRows.h"

namespace
{
    /** 그 종류의 아이템이 드는 인벤토리 구역. 구역이 없는 종류면 nullopt. */
    optional<ContainerType> FindSection(ItemRowType itemType)
    {
        switch (itemType)
        {
        case ItemRowType::GEAR: return ContainerType::INVENTORY_GEAR;
        case ItemRowType::CONSUMABLE: return ContainerType::INVENTORY_CONSUMABLE;
        case ItemRowType::MISC: return ContainerType::INVENTORY_MISC;
        default: return nullopt;
        }
    }

    /** 그 인벤토리 구역에 드는 아이템 종류. 인벤토리 구역이 아니면 nullopt. */
    optional<ItemRowType> FindItemType(ContainerType section)
    {
        switch (section)
        {
        case ContainerType::INVENTORY_GEAR: return ItemRowType::GEAR;
        case ContainerType::INVENTORY_CONSUMABLE: return ItemRowType::CONSUMABLE;
        case ContainerType::INVENTORY_MISC: return ItemRowType::MISC;
        default: return nullopt;
        }
    }
}

optional<PlacedItem> ItemRows::ToPlacedItem(const ItemRow& row)
{
    PlacedItem placed;
    placed.address.slotIndex = row.slotIndex;

    switch (row.container)
    {
    case ItemRowContainer::INVENTORY:
    {
        // 구역은 아이템 종류다. 아이템 표의 종류와 맞는지는 입장 검증이 본다.
        optional<ContainerType> section = FindSection(row.itemType);
        if (section.has_value() == false)
            return nullopt;

        placed.address.container = section.value();
        break;
    }
    case ItemRowContainer::EQUIPPED:
    {
        // 착용 장비에는 장비만 들고, 칸 번호가 장비 부위다.
        if (row.itemType != ItemRowType::GEAR)
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
    if (row.itemType == ItemRowType::GEAR)
        placed.item.gear = GearDetail{ row.enhanceLevel, row.durability };

    return placed;
}

optional<ItemRow> ItemRows::ToRow(const PlacedItem& placed)
{
    ItemRow row;
    if (placed.address.container == ContainerType::EQUIPPED)
    {
        row.container = ItemRowContainer::EQUIPPED;
        row.itemType = ItemRowType::GEAR;
    }
    else
    {
        optional<ItemRowType> itemType = FindItemType(placed.address.container);
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
