// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/StatusWindowWidget.h"
#include "Widgets/HUDWidget.h"
#include "Components/TextBlock.h"
#include "P1.h"
#include "P1MyPlayer.h"
#include "P1GameInstance.h"
#include "MyPlayerData.h"
#include "EquippedGear.h"

void UStatusWindowWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance()))
    {
        // MyPlayerData에서 인벤토리 정보를 가져와 갱신한다.
        if (UMyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UMyPlayerData>())
        {
            const Protocol::PlayerInfo& PlayerInfo_ = MyPlayerData->GetPlayerInfo();

            UpdateAllStat(MyPlayerData);

            for (const auto& Pair : MyPlayerData->GetEquippedGear()->GetAllSlot())
            {
                const Protocol::Slot& Slot_ = Pair.second;
                UpdateSlotWidget(Slot_);
            }

            // 바인딩 셋업
            MyPlayerData->OnEquipmentSlotChanged.AddUObject(this, &UStatusWindowWidget::UpdateSlotWidget);
            GameInstance->OnRecvUnequipGearPkt.AddLambda([this]() { if (IsValid(this)) this->PendingPacket = false; });
        }
    }

}

void UStatusWindowWidget::UpdateSlotWidget(const Protocol::Slot& Slot_)
{
    TObjectPtr<USlotWidget> SlotWidget = nullptr;
    switch ((Protocol::GearType)Slot_.slot_id())
    {
    case Protocol::GearType::GEAR_TYPE_HELMET:
        SlotWidget = Equipped_Helmet;
        break;
    case Protocol::GearType::GEAR_TYPE_CHEST:
        SlotWidget = Equipped_Chest;
        break;
    case Protocol::GearType::GEAR_TYPE_ARMS:
        SlotWidget = Equipped_Arms;
        break;
    case Protocol::GearType::GEAR_TYPE_LEGS:
        SlotWidget = Equipped_Legs;
        break;
    case Protocol::GearType::GEAR_TYPE_BOOTS:
        SlotWidget = Equipped_Boots;
        break;
    case Protocol::GearType::GEAR_TYPE_WEAPON:
        SlotWidget = Equipped_Weapon;
        break;
    default:
        return;
    }

    if (SlotWidget)
        SlotWidget->SetSlot(Slot_);
}

void UStatusWindowWidget::UpdateAllStat(UMyPlayerData* MyPlayerData)
{
    UpdateMaxHp(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MAX_HP));
    UpdateMaxMp(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MAX_MP));
    UpdatePhysicalAttack(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK));
    UpdateMagicalAttack(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK));
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

void UStatusWindowWidget::SendUnequipPacket(USlotWidget* Slot_)
{
    if (PendingPacket)
        return;
    else
        PendingPacket = true;

    if (Slot_)
    {
        const Protocol::Slot& SlotData = Slot_->SlotData;

        Protocol::C_UNEQUIP_GEAR pkt;
        pkt.mutable_slot()->CopyFrom(SlotData);
        SEND_PACKET(pkt);
    }
}
