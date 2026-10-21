#include "Game/Equipment/P1Equipment.h"

void UP1Equipment::Init(const Protocol::Possession& Possession)
{
    EquippedSlots.Reset();

    for (const Protocol::ContainerSlots& ContainerSlots : Possession.containers())
    {
        if (ContainerSlots.container() != Protocol::CONTAINER_TYPE_EQUIPPED)
            continue;

        for (const Protocol::Slot& Slot_ : ContainerSlots.slots())
        {
            Protocol::SlotChange& Cached = EquippedSlots.Add(Slot_.slot_index());
            Cached.mutable_address()->set_container(Protocol::CONTAINER_TYPE_EQUIPPED);
            Cached.mutable_address()->set_slot_index(Slot_.slot_index());
            if (Slot_.has_item())
                *Cached.mutable_item() = Slot_.item();
        }
    }
}

void UP1Equipment::Rep_SlotChanged(const Protocol::SlotChange& Change)
{
    if (Change.address().container() != Protocol::CONTAINER_TYPE_EQUIPPED)
        return;

    EquippedSlots.FindOrAdd(Change.address().slot_index()).CopyFrom(Change);
}
