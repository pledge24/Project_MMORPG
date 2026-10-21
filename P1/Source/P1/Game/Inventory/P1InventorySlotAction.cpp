#include "Game/Inventory/P1InventorySlotAction.h"

FP1InventorySlotAction::EKind FP1InventorySlotAction::Decide(
    EP1ContainerType Container, Protocol::ItemType ItemType, int32 TemplateId, int32 LevelRequirement, int32 PlayerLevel, bool bCoolingDown)
{
    if (TemplateId <= 0)
        return EKind::Empty;

    const bool bInInventory = Container == EP1ContainerType::InventoryGear
        || Container == EP1ContainerType::InventoryConsumable
        || Container == EP1ContainerType::InventoryMisc;
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
