#include "UI/P1StatusWindowWidget.h"
#include "UI/P1HUDWidget.h"
#include "Components/TextBlock.h"
#include "P1.h"
#include "Characters/P1MyPlayer.h"
#include "Core/P1GameInstance.h"
#include "Core/P1MyPlayerData.h"
#include "Equipment/P1EquippedGear.h"

void UP1StatusWindowWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance()))
    {
        // MyPlayerData에서 인벤토리 정보를 가져와 갱신한다.
        if (UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>())
        {
            const Protocol::PlayerInfo& PlayerInfo_ = MyPlayerData->GetPlayerInfo();

            UpdateAllStat(MyPlayerData);

            for (const auto& Pair : MyPlayerData->GetEquippedGear()->GetAllSlot())
            {
                const Protocol::Slot& Slot_ = Pair.second;
                UpdateSlotWidget(Slot_);
            }

            // 바인딩 셋업
            MyPlayerData->OnEquipmentSlotChanged.AddUObject(this, &UP1StatusWindowWidget::UpdateSlotWidget);
            GameInstance->OnRecvUnequipGearPkt.AddLambda([this]() { if (IsValid(this)) this->PendingPacket = false; });
        }
    }

}

void UP1StatusWindowWidget::UpdateSlotWidget(const Protocol::Slot& Slot_)
{
    TObjectPtr<UP1SlotWidget> SlotWidget = nullptr;
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

void UP1StatusWindowWidget::UpdateAllStat(UP1MyPlayerData* MyPlayerData)
{
    UpdateMaxHp(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MAX_HP));
    UpdateMaxMp(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MAX_MP));
    UpdatePhysicalAttack(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_PHYSICAL_ATTACK));
    UpdateMagicalAttack(MyPlayerData->GetStatValue(Protocol::STAT_TYPE_MAGICAL_ATTACK));
}

void UP1StatusWindowWidget::UpdateMaxHp(int32 Value)
{
    Details_MaxHp->SetText(FText::AsNumber(Value));
}

void UP1StatusWindowWidget::UpdateMaxMp(int32 Value)
{
    Details_MaxMp->SetText(FText::AsNumber(Value));
}

void UP1StatusWindowWidget::UpdatePhysicalAttack(int32 Value)
{
    Details_Physical_Attack->SetText(FText::AsNumber(Value));
}

void UP1StatusWindowWidget::UpdateMagicalAttack(int32 Value)
{
    Details_Magical_Attack->SetText(FText::AsNumber(Value));
}

void UP1StatusWindowWidget::SendUnequipPacket(UP1SlotWidget* Slot_)
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
