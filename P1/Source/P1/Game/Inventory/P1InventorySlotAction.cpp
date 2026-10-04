#include "Game/Inventory/P1InventorySlotAction.h"

FP1InventorySlotAction::EKind FP1InventorySlotAction::Decide(
    Protocol::SlotType SlotType, Protocol::ItemType ItemType, int32 TemplateId, int32 LevelRequirement, int32 PlayerLevel, bool bCoolingDown)
{
    if (TemplateId <= 0)
        return EKind::Empty;

    const bool bInInventory = SlotType == Protocol::SLOT_TYPE_INVENTORY_GEAR
        || SlotType == Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE
        || SlotType == Protocol::SLOT_TYPE_INVENTORY_MISC;
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
