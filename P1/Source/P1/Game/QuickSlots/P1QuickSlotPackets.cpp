#include "Game/QuickSlots/P1QuickSlotPackets.h"

FP1QuickSlot P1QuickSlotPackets::FromProto(const Protocol::QuickSlot& QuickSlot)
{
    FP1QuickSlot Result;
    Result.SlotIndex = QuickSlot.quick_slot_index();

    // 모르는 종류를 그대로 들면 등록된 것으로 보여 그릴 수 없는 퀵슬롯이 된다.
    if (QuickSlot.has_ref() && QuickSlot.ref().ref_type() == Protocol::QUICK_SLOT_REF_TYPE_ITEM)
        Result.Ref = FP1QuickSlotRef::ForItem(QuickSlot.ref().ref_id());

    return Result;
}

Protocol::QuickSlotRef P1QuickSlotPackets::ToProto(const FP1QuickSlotRef& Ref)
{
    Protocol::QuickSlotRef Proto;
    Proto.set_ref_type(Ref.Type == EP1QuickSlotRefType::Item ? Protocol::QUICK_SLOT_REF_TYPE_ITEM : Protocol::QUICK_SLOT_REF_TYPE_NONE);
    Proto.set_ref_id(Ref.Id);
    return Proto;
}
