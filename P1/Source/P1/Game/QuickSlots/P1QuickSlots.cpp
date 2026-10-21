#include "Game/QuickSlots/P1QuickSlots.h"
#include "Utils/LogCategory.h"

void UP1QuickSlots::ResetQuickSlots(const TArray<FP1QuickSlot>& InQuickSlots)
{
    QuickSlots.Reset(InQuickSlots.Num());
    for (const FP1QuickSlot& InQuickSlot : InQuickSlots)
    {
        // 같은 번호가 두 번 오면 뒤의 것을 쓴다. 둘 두면 변경이 한쪽만 고친다.
        if (FP1QuickSlot* Existing = QuickSlots.FindByPredicate([&](const FP1QuickSlot& Q) { return Q.SlotIndex == InQuickSlot.SlotIndex; }))
        {
            UE_LOG(LogP1QuickSlots, Warning, TEXT("퀵슬롯 번호 %d가 두 번 왔다"), InQuickSlot.SlotIndex);
            Existing->Ref = InQuickSlot.Ref;
            continue;
        }

        QuickSlots.Add(InQuickSlot);
    }

    QuickSlots.Sort([](const FP1QuickSlot& A, const FP1QuickSlot& B) { return A.SlotIndex < B.SlotIndex; });

    OnReset.Broadcast(*this);
}

void UP1QuickSlots::Clear()
{
    ResetQuickSlots({});
}

bool UP1QuickSlots::ApplyChange(const FP1QuickSlot& Change)
{
    FP1QuickSlot* Target = const_cast<FP1QuickSlot*>(Find(Change.SlotIndex));
    if (Target == nullptr)
    {
        UE_LOG(LogP1QuickSlots, Warning, TEXT("없는 퀵슬롯의 변경을 버린다. 퀵슬롯 번호 %d"), Change.SlotIndex);
        return false;
    }

    // 먼저 고친다. 구독자가 알림 안에서 다른 퀵슬롯을 읽어도 새 값을 읽어야 한다.
    Target->Ref = Change.Ref;
    OnChanged.Broadcast(*Target);
    return true;
}

const FP1QuickSlot* UP1QuickSlots::Find(int32 SlotIndex) const
{
    return QuickSlots.FindByPredicate([SlotIndex](const FP1QuickSlot& QuickSlot) { return QuickSlot.SlotIndex == SlotIndex; });
}
