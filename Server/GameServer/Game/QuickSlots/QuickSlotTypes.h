#pragma once

/** 퀵슬롯이 가리키는 대상의 종류. DB의 QuickSlot.RefType에 이 값이 들어가므로 값을 바꾸지 않는다. */
enum class QuickSlotRefType : uint8
{
    NONE = 0,
    /** 아이템 정의. id가 아이템 번호다. */
    ITEM = 1,
};

/** 퀵슬롯이 가리키는 대상. 아이템 하나가 아니라 아이템 정의를 가리킨다. */
struct QuickSlotRef
{
    QuickSlotRefType type = QuickSlotRefType::NONE;
    int32 id = 0;

    bool operator==(const QuickSlotRef&) const = default;
};

/** 퀵슬롯 하나의 지금 내용. ref가 비어 있으면 빈 퀵슬롯이다. */
struct QuickSlot
{
    /** 0부터 퀵슬롯 개수 미만이다. */
    int32 slotIndex = 0;
    optional<QuickSlotRef> ref;

    bool operator==(const QuickSlot&) const = default;
};
