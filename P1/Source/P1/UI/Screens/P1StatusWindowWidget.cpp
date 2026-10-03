#include "UI/Screens/P1StatusWindowWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Network/P1PacketSender.h"
#include "Core/P1GameInstance.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Game/Equipment/P1EquippedGear.h"

void UP1StatusWindowWidget::NativeConstruct()
{
    Super::NativeConstruct();

    for (UP1SlotWidget* SlotWidget : { Equipped_Helmet, Equipped_Chest, Equipped_Arms, Equipped_Legs, Equipped_Boots, Equipped_Weapon })
        SlotWidget->OnDoubleClicked.AddUObject(this, &UP1StatusWindowWidget::HandleSlotDoubleClicked);

    Button_Details->OnClicked.AddUniqueDynamic(this, &UP1StatusWindowWidget::ToggleTips);

    if (auto* GameInstance = GetP1GameInstance())
    {
        // MyPlayerData에서 인벤토리 정보를 가져와 갱신한다.
        if (UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>())
        {
            UpdateAllStat(MyPlayerData);

            for (const auto& Pair : MyPlayerData->GetEquippedGear()->GetAllSlot())
            {
                const Protocol::Slot& Slot_ = Pair.second;
                UpdateSlotWidget(Slot_);
            }

            // 바인딩 셋업
            MyPlayerData->OnEquipmentSlotChanged.AddUObject(this, &UP1StatusWindowWidget::UpdateSlotWidget);

            // 창은 한 번 만들고 표시 여부만 바꾸므로, 수치도 열 때 다시 읽지 않고 바뀔 때마다 받는다.
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MAX_HP].AddUObject(this, &UP1StatusWindowWidget::UpdateMaxHp);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MAX_MP].AddUObject(this, &UP1StatusWindowWidget::UpdateMaxMp);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_PHYSICAL_ATTACK].AddUObject(this, &UP1StatusWindowWidget::UpdatePhysicalAttack);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MAGICAL_ATTACK].AddUObject(this, &UP1StatusWindowWidget::UpdateMagicalAttack);
            MyPlayerData->OnRecvUnequipGearPkt.AddWeakLambda(this, [this]() { PendingPacket = false; });
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

void UP1StatusWindowWidget::UpdateMaxHp(int64 Value)
{
    Details_MaxHp->SetText(FText::AsNumber(Value));
}

void UP1StatusWindowWidget::UpdateMaxMp(int64 Value)
{
    Details_MaxMp->SetText(FText::AsNumber(Value));
}

void UP1StatusWindowWidget::UpdatePhysicalAttack(int64 Value)
{
    Details_Physical_Attack->SetText(FText::AsNumber(Value));
}

void UP1StatusWindowWidget::UpdateMagicalAttack(int64 Value)
{
    Details_Magical_Attack->SetText(FText::AsNumber(Value));
}

void UP1StatusWindowWidget::HandleSlotDoubleClicked(UP1SlotWidget* SlotWidget)
{
    // 보내기 전에 돌아가는 분기를 모두 지난 뒤에 대기를 켠다. 켜고 보내지 않으면 응답이 오지 않아 해제가 막힌다.
    if (PendingPacket)
        return;

    Protocol::C_UNEQUIP_GEAR Pkt;
    Pkt.mutable_slot()->CopyFrom(SlotWidget->SlotData);

    PendingPacket = true;
    FP1PacketSender::Send(this, Pkt);
}

void UP1StatusWindowWidget::ToggleTips()
{
    CanvasPanel_Tips->SetVisibility(
        CanvasPanel_Tips->IsVisible() ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
}
