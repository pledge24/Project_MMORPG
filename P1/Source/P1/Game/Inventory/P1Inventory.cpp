#include "Game/Inventory/P1Inventory.h"

namespace
{
    bool IsInventorySection(Protocol::ContainerType Container)
    {
        return Container == Protocol::CONTAINER_TYPE_INVENTORY_GEAR
            || Container == Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE
            || Container == Protocol::CONTAINER_TYPE_INVENTORY_MISC;
    }
}

void UP1Inventory::Init(const Protocol::Possession& Possession)
{
    Sections.Reset();

    // 칸 번호가 구역마다 0부터 칸 수 미만이라고 보고 그대로 인덱스로 쓴다. 범위를 벗어난 번호의 칸은 버린다.
    for (const Protocol::ContainerSlots& ContainerSlots : Possession.containers())
    {
        if (IsInventorySection(ContainerSlots.container()) == false)
            continue;

        TArray<Protocol::SlotChange>& SectionSlots = Sections.Add(ContainerSlots.container());
        SectionSlots.SetNum(ContainerSlots.slots_size());
        for (const Protocol::Slot& Slot_ : ContainerSlots.slots())
        {
            if (SectionSlots.IsValidIndex(Slot_.slot_index()) == false)
                continue;

            Protocol::SlotChange& Cached = SectionSlots[Slot_.slot_index()];
            Cached.mutable_address()->set_container(ContainerSlots.container());
            Cached.mutable_address()->set_slot_index(Slot_.slot_index());
            if (Slot_.has_item())
                *Cached.mutable_item() = Slot_.item();
        }
    }
}

void UP1Inventory::Rep_SlotChanged(const Protocol::SlotChange& Change)
{
    TArray<Protocol::SlotChange>* SectionSlots = Sections.Find(Change.address().container());
    if (SectionSlots == nullptr || SectionSlots->IsValidIndex(Change.address().slot_index()) == false)
        return;

    (*SectionSlots)[Change.address().slot_index()].CopyFrom(Change);
}

const Protocol::SlotChange* UP1Inventory::FindSlot(Protocol::ContainerType Container, int32 SlotIndex) const
{
    const TArray<Protocol::SlotChange>* SectionSlots = Sections.Find(Container);
    if (SectionSlots == nullptr || SectionSlots->IsValidIndex(SlotIndex) == false)
        return nullptr;

    return &(*SectionSlots)[SlotIndex];
}

const TArray<Protocol::SlotChange>* UP1Inventory::GetSectionSlots(Protocol::ContainerType Container) const
{
    return Sections.Find(Container);
}
