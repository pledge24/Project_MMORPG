#include "Core/pch.h"
#include "Network/ItemPackets.h"
#include "Game/Entities/Player.h"
#include "Game/Inventory/InventoryComponent.h"
#include "Game/Equipment/EquipmentComponent.h"
#include "Game/Items/InventorySections.h"

namespace
{
    /** 칸 하나를 패킷 칸으로 쓴다. 빈 칸이면 아이템을 싣지 않는다. */
    Protocol::Slot MakeSlot(const SlotAddress& address, const ItemInstance* item, Protocol::UpdateState state)
    {
        Protocol::Slot slot;
        slot.set_slot_id(address.slotIndex);
        slot.set_type(ItemPackets::ToSlotType(address.container));
        slot.set_state(state);
        if (item != nullptr)
            *slot.mutable_item() = ItemPackets::ToProto(*item);

        return slot;
    }

    /** 인벤토리 패킷에서 그 구역의 칸 목록. 인벤토리 구역이 아니면 nullptr. */
    RepeatedPtrField<Protocol::Slot>* FindSectionSlots(Protocol::Inventory& inventory, ContainerType container)
    {
        switch (container)
        {
        case ContainerType::INVENTORY_GEAR:
            return inventory.mutable_gear();
        case ContainerType::INVENTORY_CONSUMABLE:
            return inventory.mutable_consumables();
        case ContainerType::INVENTORY_MISC:
            return inventory.mutable_miscellaneous();
        default:
            return nullptr;
        }
    }
}

Protocol::Item ItemPackets::ToProto(const ItemInstance& item)
{
    Protocol::Item proto;
    proto.set_template_id(item.templateId);
    proto.set_count(item.count);
    if (item.itemUid != 0)
        proto.set_item_uid(item.itemUid);

    // 추가 공격력은 서버가 들지 않는다. 패킷에는 0이 들어간다.
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

Protocol::SlotType ItemPackets::ToSlotType(ContainerType container)
{
    switch (container)
    {
    case ContainerType::INVENTORY_GEAR:
        return Protocol::SLOT_TYPE_INVENTORY_GEAR;
    case ContainerType::INVENTORY_CONSUMABLE:
        return Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE;
    case ContainerType::INVENTORY_MISC:
        return Protocol::SLOT_TYPE_INVENTORY_MISC;
    case ContainerType::EQUIPPED:
        return Protocol::SLOT_TYPE_EQUIPPED;
    default:
        return Protocol::SLOT_TYPE_NONE;
    }
}

ContainerType ItemPackets::ToContainerType(Protocol::SlotType slotType)
{
    // 퀵슬롯은 아이템을 담지 않으므로 컨테이너가 아니다.
    switch (slotType)
    {
    case Protocol::SLOT_TYPE_INVENTORY_GEAR:
        return ContainerType::INVENTORY_GEAR;
    case Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE:
        return ContainerType::INVENTORY_CONSUMABLE;
    case Protocol::SLOT_TYPE_INVENTORY_MISC:
        return ContainerType::INVENTORY_MISC;
    case Protocol::SLOT_TYPE_EQUIPPED:
        return ContainerType::EQUIPPED;
    default:
        return ContainerType::NONE;
    }
}

Protocol::Slot ItemPackets::ToChangedSlot(const SlotChange& change)
{
    // 클라이언트는 아이템이 있으면 상태와 무관하게 칸을 채우므로, 아이템이 남은 칸은 추가와 수정을 가리지 않는다.
    const Protocol::UpdateState state = change.item.has_value() ? Protocol::UPDATE_STATE_MODIFIED : Protocol::UPDATE_STATE_REMOVED;
    return MakeSlot(change.address, change.item.has_value() ? &change.item.value() : nullptr, state);
}

void ItemPackets::AddChangedSlots(const vector<SlotChange>& changes, OUT RepeatedPtrField<Protocol::Slot>* slots)
{
    for (const SlotChange& change : changes)
        *slots->Add() = ToChangedSlot(change);
}

ItemSlotRequest ItemPackets::ToRequest(const Protocol::Slot& slot)
{
    ItemSlotRequest request;
    request.address = SlotAddress{ ToContainerType(slot.type()), slot.slot_id() };
    if (slot.has_item())
    {
        request.templateId = slot.item().template_id();
        request.itemUid = slot.item().item_uid();
    }

    return request;
}

Protocol::Possession ItemPackets::MakePossession(const Player& player)
{
    Protocol::Possession possession;
    possession.set_gold(player.GetGold());

    // 클라이언트는 받은 칸 목록으로 인벤토리 창의 칸을 만들므로 빈 칸도 싣는다.
    for (const InventorySection& section : InventorySections::ALL)
    {
        const ItemContainer* container = player.GetInventory().FindSection(section.container);
        RepeatedPtrField<Protocol::Slot>* slots = FindSectionSlots(*possession.mutable_inventory(), section.container);
        if (container == nullptr || slots == nullptr)
            continue;

        for (int32 slotIndex : container->GetSlotIndices())
            *slots->Add() = MakeSlot(SlotAddress{ section.container, slotIndex }, container->Find(slotIndex), Protocol::UPDATE_STATE_NONE);
    }

    const ItemContainer& equipped = player.GetEquipment().GetContainer();
    auto* equippedGear = possession.mutable_equipped_gear();
    for (int32 gearType : equipped.GetSlotIndices())
        (*equippedGear)[gearType] = MakeSlot(SlotAddress{ ContainerType::EQUIPPED, gearType }, equipped.Find(gearType), Protocol::UPDATE_STATE_NONE);

    return possession;
}
