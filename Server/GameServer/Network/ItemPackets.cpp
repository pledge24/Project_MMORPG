#include "Core/pch.h"
#include "Network/ItemPackets.h"
#include "Game/Entities/Player.h"
#include "Game/Inventory/InventoryComponent.h"
#include "Game/Equipment/EquipmentComponent.h"
#include "Game/Items/InventorySections.h"

namespace
{
    /** 컨테이너 하나의 모든 칸. 빈 칸이면 아이템을 싣지 않는다. */
    Protocol::ContainerSlots MakeContainerSlots(ContainerType containerType, const ItemContainer& container)
    {
        Protocol::ContainerSlots containerSlots;
        containerSlots.set_container(ItemPackets::ToProto(containerType));
        for (int32 slotIndex : container.GetSlotIndices())
        {
            Protocol::Slot* slot = containerSlots.add_slots();
            slot->set_slot_index(slotIndex);
            if (const ItemInstance* item = container.Find(slotIndex))
                *slot->mutable_item() = ItemPackets::ToProto(*item);
        }

        return containerSlots;
    }
}

Protocol::Item ItemPackets::ToProto(const ItemInstance& item)
{
    Protocol::Item proto;
    proto.set_template_id(item.templateId);
    proto.set_count(item.count);
    if (item.itemUid != 0)
        proto.set_item_uid(item.itemUid);

    if (item.gear.has_value())
    {
        Protocol::GearInfo* gearInfo = proto.mutable_gearinfo();
        gearInfo->set_enhance_level(item.gear->enhanceLevel);
        gearInfo->set_durability(item.gear->durability);
    }

    return proto;
}

ItemInstance ItemPackets::FromProto(const Protocol::Item& proto)
{
    ItemInstance item;
    item.itemUid = proto.item_uid();
    item.templateId = proto.template_id();
    item.count = proto.count();
    if (proto.has_gearinfo())
        item.gear = GearDetail{ proto.gearinfo().enhance_level(), proto.gearinfo().durability() };

    return item;
}

Protocol::ContainerType ItemPackets::ToProto(ContainerType container)
{
    switch (container)
    {
    case ContainerType::INVENTORY_GEAR:
        return Protocol::CONTAINER_TYPE_INVENTORY_GEAR;
    case ContainerType::INVENTORY_CONSUMABLE:
        return Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE;
    case ContainerType::INVENTORY_MISC:
        return Protocol::CONTAINER_TYPE_INVENTORY_MISC;
    case ContainerType::EQUIPPED:
        return Protocol::CONTAINER_TYPE_EQUIPPED;
    default:
        return Protocol::CONTAINER_TYPE_NONE;
    }
}

ContainerType ItemPackets::FromProto(Protocol::ContainerType container)
{
    switch (container)
    {
    case Protocol::CONTAINER_TYPE_INVENTORY_GEAR:
        return ContainerType::INVENTORY_GEAR;
    case Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE:
        return ContainerType::INVENTORY_CONSUMABLE;
    case Protocol::CONTAINER_TYPE_INVENTORY_MISC:
        return ContainerType::INVENTORY_MISC;
    case Protocol::CONTAINER_TYPE_EQUIPPED:
        return ContainerType::EQUIPPED;
    default:
        return ContainerType::NONE;
    }
}

Protocol::SlotAddress ItemPackets::ToProto(const SlotAddress& address)
{
    Protocol::SlotAddress proto;
    proto.set_container(ToProto(address.container));
    proto.set_slot_index(address.slotIndex);
    return proto;
}

SlotAddress ItemPackets::FromProto(const Protocol::SlotAddress& proto)
{
    return SlotAddress{ FromProto(proto.container()), proto.slot_index() };
}

Protocol::SlotChange ItemPackets::ToProto(const SlotChange& change)
{
    Protocol::SlotChange proto;
    *proto.mutable_address() = ToProto(change.address);
    if (change.item.has_value())
        *proto.mutable_item() = ToProto(change.item.value());

    return proto;
}

void ItemPackets::AddChangedSlots(const vector<SlotChange>& changes, OUT RepeatedPtrField<Protocol::SlotChange>* protoChanges)
{
    for (const SlotChange& change : changes)
        *protoChanges->Add() = ToProto(change);
}

ItemSlotRequest ItemPackets::ToRequest(const Protocol::SlotAddress& address, int64 itemUid)
{
    ItemSlotRequest request;
    request.address = FromProto(address);
    request.itemUid = itemUid;
    return request;
}

Protocol::Possession ItemPackets::MakePossession(const Player& player)
{
    Protocol::Possession possession;
    possession.set_gold(player.GetGold());

    // 클라이언트는 받은 칸 목록으로 인벤토리 창의 칸을 만들므로 빈 칸도 싣는다.
    for (const InventorySection& section : InventorySections::ALL)
    {
        if (const ItemContainer* container = player.GetInventory().FindSection(section.container))
            *possession.add_containers() = MakeContainerSlots(section.container, *container);
    }

    *possession.add_containers() = MakeContainerSlots(ContainerType::EQUIPPED, player.GetEquipment().GetContainer());

    return possession;
}
