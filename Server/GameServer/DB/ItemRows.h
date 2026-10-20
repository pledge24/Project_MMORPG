#pragma once

/** 아이템을 담는 곳. DB Item 테이블의 Container 열 값이다. */
enum class ItemContainer : uint8
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
    ItemContainer container = ItemContainer::INVENTORY;
    /** 인벤토리에서는 구역 안의 칸 번호, 착용 장비에서는 장비 부위다. */
    int32 slotIndex = 0;
    int32 stackCount = 1;

    //~ 장비만 쓴다(ItemGear).
    int32 enhanceLevel = 0;
    int32 durability = 0;
};

/**
 * DB 아이템 행과 패킷 칸 사이의 변환.
 *
 * DB는 아이템의 자리를 컨테이너, 구역, 칸 번호로 나타내고, 패킷은 칸 종류와 slot_id로 나타낸다.
 * 불러오기와 저장이 이 변환 하나를 함께 쓴다. DB와 기획 데이터는 읽지 않는다.
 */
namespace ItemRows
{
    /**
     * 행을 패킷 칸으로 바꿔 possession의 인벤토리나 착용 장비에 붙인다.
     * 모르는 컨테이너나 구역, 장비가 아닌 착용 장비, 이미 찬 장비 부위면 아무것도 바꾸지 않고 false.
     * 칸 수와 아이템 표는 보지 않으므로 입장 검증이 따로 본다.
     */
    bool AddToPossession(const ItemRow& row, OUT Protocol::Possession& possession);

    /** possession에서 아이템이 든 칸을 모두 행으로 만든다. 컨테이너, 구역, 칸 번호 순서다. */
    vector<ItemRow> FromPossession(const Protocol::Possession& possession);
}
