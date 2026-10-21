#pragma once
#include "Game/Items/ItemTypes.h"

/** 아이템을 담는 곳. DB Item 테이블의 Container 열 값이다. 인벤토리의 구역은 ItemType 열이 가른다. */
enum class ItemRowContainer : uint8
{
    INVENTORY = 1,
    EQUIPPED = 2,
};

/** Item 테이블의 행 하나. 장비면 그 아이템의 ItemGear 행 값도 함께 든다. */
struct ItemRow
{
    int64 itemUid = 0;
    int32 templateId = 0;
    /** 인벤토리에서는 아이템이 든 구역이다. */
    Protocol::ItemType itemType = Protocol::ITEM_TYPE_NONE;
    ItemRowContainer container = ItemRowContainer::INVENTORY;
    /** 인벤토리에서는 구역 안의 칸 번호, 착용 장비에서는 장비 부위다. */
    int32 slotIndex = 0;
    int32 stackCount = 1;

    // 아래 둘은 장비만 쓴다(ItemGear).
    int32 enhanceLevel = 0;
    int32 durability = 0;
};

/**
 * DB 아이템 행과 칸에 든 아이템 사이의 변환.
 *
 * DB는 아이템의 자리를 컨테이너, 아이템 종류, 칸 번호로 나타내고, 서버는 칸 주소로 나타낸다.
 * 불러오기와 저장이 이 변환 하나를 함께 쓴다. DB와 기획 데이터는 읽지 않으므로 칸 수와 아이템 표는 입장 검증이 본다.
 */
namespace ItemRows
{
    /** 행을 칸에 든 아이템으로 바꾼다. 모르는 컨테이너나 구역, 장비가 아닌 착용 장비면 nullopt. */
    optional<PlacedItem> ToPlacedItem(const ItemRow& row);
    /** 칸에 든 아이템을 행으로 바꾼다. 인벤토리 구역도 착용 장비도 아닌 주소면 nullopt. */
    optional<ItemRow> ToRow(const PlacedItem& placed);
}
