#include "Game/Items/P1ItemPackets.h"

Protocol::ContainerType P1ItemPackets::ToProto(EP1ContainerType Container)
{
    switch (Container)
    {
    case EP1ContainerType::InventoryGear:       return Protocol::CONTAINER_TYPE_INVENTORY_GEAR;
    case EP1ContainerType::InventoryConsumable: return Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE;
    case EP1ContainerType::InventoryMisc:       return Protocol::CONTAINER_TYPE_INVENTORY_MISC;
    case EP1ContainerType::Equipped:            return Protocol::CONTAINER_TYPE_EQUIPPED;
    default:                                    return Protocol::CONTAINER_TYPE_NONE;
    }
}

EP1ContainerType P1ItemPackets::FromProto(Protocol::ContainerType Container)
{
    switch (Container)
    {
    case Protocol::CONTAINER_TYPE_INVENTORY_GEAR:       return EP1ContainerType::InventoryGear;
    case Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE: return EP1ContainerType::InventoryConsumable;
    case Protocol::CONTAINER_TYPE_INVENTORY_MISC:       return EP1ContainerType::InventoryMisc;
    case Protocol::CONTAINER_TYPE_EQUIPPED:             return EP1ContainerType::Equipped;
    default:                                            return EP1ContainerType::None;
    }
}

Protocol::SlotAddress P1ItemPackets::ToProto(const FP1SlotAddress& Address)
{
    Protocol::SlotAddress Proto;
    Proto.set_container(ToProto(Address.Container));
    Proto.set_slot_index(Address.SlotIndex);
    return Proto;
}

FP1SlotAddress P1ItemPackets::FromProto(const Protocol::SlotAddress& Address)
{
    FP1SlotAddress Result;
    Result.Container = FromProto(Address.container());
    Result.SlotIndex = Address.slot_index();
    return Result;
}

FP1ItemInstance P1ItemPackets::FromProto(const Protocol::Item& Item)
{
    FP1ItemInstance Result;
    Result.ItemUid = Item.item_uid();
    Result.TemplateId = Item.template_id();
    Result.Count = Item.count();
    return Result;
}

FP1Slot P1ItemPackets::FromProto(const Protocol::SlotChange& Change)
{
    FP1Slot Result;
    Result.Address = FromProto(Change.address());
    if (Change.has_item())
        Result.Item = FromProto(Change.item());
    return Result;
}

TArray<FP1Slot> P1ItemPackets::FromProto(const Protocol::ContainerSlots& ContainerSlots)
{
    const EP1ContainerType Container = FromProto(ContainerSlots.container());

    TArray<FP1Slot> Result;
    Result.Reserve(ContainerSlots.slots_size());
    for (const Protocol::Slot& ProtoSlot : ContainerSlots.slots())
    {
        FP1Slot& Slot_ = Result.AddDefaulted_GetRef();
        Slot_.Address.Container = Container;
        Slot_.Address.SlotIndex = ProtoSlot.slot_index();
        if (ProtoSlot.has_item())
            Slot_.Item = FromProto(ProtoSlot.item());
    }
    return Result;
}
