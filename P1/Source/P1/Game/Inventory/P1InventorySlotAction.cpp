#include "Game/Inventory/P1InventorySlotAction.h"

FP1InventorySlotAction::EKind FP1InventorySlotAction::Decide(
    Protocol::SlotType SlotType, int32 TemplateId, int32 LevelRequirement, int32 PlayerLevel)
{
    if (TemplateId <= 0)
        return EKind::Empty;

    if (SlotType != Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE && SlotType != Protocol::SLOT_TYPE_INVENTORY_GEAR)
        return EKind::Unusable;

    if (LevelRequirement > PlayerLevel)
        return EKind::LevelTooLow;

    return SlotType == Protocol::SLOT_TYPE_INVENTORY_CONSUMABLE ? EKind::Use : EKind::Equip;
}
