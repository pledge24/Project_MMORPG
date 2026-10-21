#include "Game/Items/P1ItemContainer.h"
#include "Utils/LogCategory.h"

void UP1ItemContainer::Init(EP1ContainerType InContainerType)
{
    ContainerType = InContainerType;
}

void UP1ItemContainer::ResetSlots(const TArray<FP1Slot>& InSlots)
{
    Slots.Reset(InSlots.Num());
    for (const FP1Slot& InSlot : InSlots)
    {
        // 같은 칸 번호가 두 번 오면 뒤의 것을 쓴다. 칸을 둘 두면 칸 변경이 한쪽만 고친다.
        if (FP1Slot* Existing = FindMutableSlot(InSlot.Address.SlotIndex))
        {
            UE_LOG(LogP1CharacterInventory, Warning, TEXT("컨테이너 %d에 칸 번호 %d가 두 번 왔다"), static_cast<int32>(ContainerType), InSlot.Address.SlotIndex);
            Existing->Item = InSlot.Item;
            continue;
        }

        FP1Slot& Added = Slots.Add_GetRef(InSlot);
        Added.Address.Container = ContainerType;
    }

    Slots.Sort([](const FP1Slot& A, const FP1Slot& B) { return A.Address.SlotIndex < B.Address.SlotIndex; });

    OnReset.Broadcast(*this);
}

void UP1ItemContainer::Clear()
{
    ResetSlots({});
}

bool UP1ItemContainer::ApplySlotChange(const FP1Slot& Change)
{
    FP1Slot* Target = Change.Address.Container == ContainerType ? FindMutableSlot(Change.Address.SlotIndex) : nullptr;
    if (Target == nullptr)
    {
        UE_LOG(LogP1CharacterInventory, Warning, TEXT("컨테이너 %d에 없는 칸의 변경을 버린다. 컨테이너 %d, 칸 번호 %d"),
            static_cast<int32>(ContainerType), static_cast<int32>(Change.Address.Container), Change.Address.SlotIndex);
        return false;
    }

    // 칸을 먼저 고친다. 구독자가 알림 안에서 다른 칸이나 구역 합계를 읽어도 새 값을 읽어야 한다.
    Target->Item = Change.Item;
    OnSlotChanged.Broadcast(*Target);
    return true;
}

const FP1Slot* UP1ItemContainer::FindSlot(int32 SlotIndex) const
{
    return Slots.FindByPredicate([SlotIndex](const FP1Slot& Slot_) { return Slot_.Address.SlotIndex == SlotIndex; });
}

int32 UP1ItemContainer::CountItem(int32 TemplateId) const
{
    int32 Total = 0;
    for (const FP1Slot& Slot_ : Slots)
    {
        if (Slot_.HasItem() && Slot_.Item.TemplateId == TemplateId)
            Total += Slot_.Item.Count;
    }
    return Total;
}

FP1Slot* UP1ItemContainer::FindMutableSlot(int32 SlotIndex)
{
    return const_cast<FP1Slot*>(FindSlot(SlotIndex));
}
