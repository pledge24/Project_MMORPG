// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/StatusWindowWidget.h"
#include "Components/TextBlock.h"

void UStatusWindowWidget::UpdateSlot(const Protocol::Slot& _Slot)
{
    TObjectPtr<USlotWidget> SlotWidget = nullptr;
    switch (_Slot.slot_id())
    {
    case 1:
        SlotWidget = Equipped_Helmet;
        break;
    case 2:
        SlotWidget = Equipped_Chest;
        break;
    case 3:
        SlotWidget = Equipped_Gloves;
        break;
    case 4:
        SlotWidget = Equipped_Legs;
        break;
    case 5:
        SlotWidget = Equipped_Boots;
        break;
    case 6:
        SlotWidget = Equipped_Weapon;
        break;
    }

    if (SlotWidget)
        SlotWidget->SetSlot(_Slot);
}

void UStatusWindowWidget::UpdateAllStat(const Protocol::StatInfo& _StatInfo)
{
    UpdateMaxHp(_StatInfo.max_hp());
    UpdateMaxMp(_StatInfo.max_mp());
    UpdatePhysicalAttack(_StatInfo.physical_attack());
    UpdateMagicalAttack(_StatInfo.magical_attack());
}

void UStatusWindowWidget::UpdateMaxHp(int32 Value)
{
    Details_MaxHp->SetText(FText::AsNumber(Value));
}

void UStatusWindowWidget::UpdateMaxMp(int32 Value)
{
    Details_MaxMp->SetText(FText::AsNumber(Value));
}

void UStatusWindowWidget::UpdatePhysicalAttack(int32 Value)
{
    Details_Physical_Attack->SetText(FText::AsNumber(Value));
}

void UStatusWindowWidget::UpdateMagicalAttack(int32 Value)
{
    Details_Magical_Attack->SetText(FText::AsNumber(Value));
}
