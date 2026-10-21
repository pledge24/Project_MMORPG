#pragma once
#include "Game/Items/ItemTypes.h"

/** 인벤토리 구역 하나와 그 구역에 드는 아이템 종류. */
struct InventorySection
{
    ContainerType container;
    Protocol::ItemType itemType;
};

/**
 * 인벤토리 구역과 아이템 종류의 대응.
 *
 * 구역 컨테이너를 만들고, 아이템을 넣을 구역을 고르고, DB 행의 (컨테이너, 아이템 종류)를 칸 주소로 바꾸는 일이
 * 모두 이 표 하나를 쓴다. 다른 표를 두면 불러온 아이템이 다른 구역으로 옮겨 간다.
 */
namespace InventorySections
{
    /** 구역 셋. 저장과 응답의 구역 순서도 이 순서다. */
    inline constexpr InventorySection ALL[] = {
        { ContainerType::INVENTORY_GEAR, Protocol::ITEM_TYPE_GEAR },
        { ContainerType::INVENTORY_CONSUMABLE, Protocol::ITEM_TYPE_CONSUMABLE },
        { ContainerType::INVENTORY_MISC, Protocol::ITEM_TYPE_MISCELLANEOUS },
    };

    /** 그 종류의 아이템이 드는 구역. 구역이 없는 종류면 nullopt. */
    inline optional<ContainerType> FindContainer(Protocol::ItemType itemType)
    {
        for (const InventorySection& section : ALL)
        {
            if (section.itemType == itemType)
                return section.container;
        }

        return nullopt;
    }

    /** 그 구역에 드는 아이템 종류. 인벤토리 구역이 아니면 nullopt. */
    inline optional<Protocol::ItemType> FindItemType(ContainerType container)
    {
        for (const InventorySection& section : ALL)
        {
            if (section.container == container)
                return section.itemType;
        }

        return nullopt;
    }
}
