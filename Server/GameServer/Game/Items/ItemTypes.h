#pragma once

/** 아이템을 담는 컨테이너의 종류. 인벤토리의 구역 셋과 착용 장비다. */
enum class ContainerType : uint8
{
    NONE = 0,
    INVENTORY_GEAR = 1,
    INVENTORY_CONSUMABLE = 2,
    INVENTORY_MISC = 3,
    EQUIPPED = 4,
};

/** 칸 하나를 가리키는 주소. 착용 장비에서는 칸 번호가 장비 부위 값이다. */
struct SlotAddress
{
    ContainerType container = ContainerType::NONE;
    int32 slotIndex = 0;

    bool operator==(const SlotAddress&) const = default;
};

/** 장비 아이템만 갖는 세부 값. */
struct GearDetail
{
    int32 enhanceLevel = 0;
    int32 durability = 0;

    bool operator==(const GearDetail&) const = default;
};

/** 칸에 든 아이템 하나. 묶음으로 쌓이는 아이템이면 묶음 하나다. */
struct ItemInstance
{
    /** 0이면 아직 발급하지 않았다. 빈 칸에 처음 놓일 때 발급한다. */
    int64 itemUid = 0;
    int32 templateId = 0;
    /** 묶음의 개수. 칸에 든 아이템은 1 이상이다. */
    int32 count = 1;
    /** 장비만 값이 있다. */
    optional<GearDetail> gear;

    bool operator==(const ItemInstance&) const = default;
};

/** 칸 하나의 지금 내용. 응답에 실을 바뀐 칸이다. 빈 칸이면 item이 비어 있다. */
struct SlotChange
{
    SlotAddress address;
    optional<ItemInstance> item;

    bool operator==(const SlotChange&) const = default;
};

/** 어느 칸에 든 아이템 하나. 진행의 불러오기와 저장이 아이템 목록으로 쓴다. */
struct PlacedItem
{
    SlotAddress address;
    ItemInstance item;

    bool operator==(const PlacedItem&) const = default;
};
