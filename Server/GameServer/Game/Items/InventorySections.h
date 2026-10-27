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
 * 아이템은 자기 종류의 구역에만 든다. 구역과 아이템 종류의 짝을 정하는 유일한 표다.
 */
namespace InventorySections
{
    /** 구역마다의 칸 수. 클라이언트 UI가 구역마다 이 칸 수를 고정으로 그리므로 바꾸면 인벤토리 위젯도 함께 고친다. */
    inline constexpr int32 SLOT_COUNT = 32;

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
