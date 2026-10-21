#include "Game/Inventory/P1InventorySlotAction.h"

FP1InventorySlotAction::EKind FP1InventorySlotAction::Decide(
    Protocol::ContainerType Container, Protocol::ItemType ItemType, int32 TemplateId, int32 LevelRequirement, int32 PlayerLevel, bool bCoolingDown)
{
    if (TemplateId <= 0)
        return EKind::Empty;

    const bool bInInventory = Container == Protocol::CONTAINER_TYPE_INVENTORY_GEAR
        || Container == Protocol::CONTAINER_TYPE_INVENTORY_CONSUMABLE
        || Container == Protocol::CONTAINER_TYPE_INVENTORY_MISC;
    if (bInInventory == false)
        return EKind::Unusable;

    if (ItemType != Protocol::ITEM_TYPE_GEAR && ItemType != Protocol::ITEM_TYPE_CONSUMABLE)
        return EKind::Unusable;

    if (LevelRequirement > PlayerLevel)
        return EKind::LevelTooLow;

    if (ItemType == Protocol::ITEM_TYPE_GEAR)
        return EKind::Equip;

    return bCoolingDown ? EKind::CoolingDown : EKind::Use;
}
