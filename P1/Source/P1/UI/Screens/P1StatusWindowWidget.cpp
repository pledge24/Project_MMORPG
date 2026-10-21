#include "UI/Screens/P1StatusWindowWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Network/P1PacketSender.h"
#include "Core/P1GameInstance.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Game/Items/P1ItemContainer.h"
#include "Game/Items/P1ItemPackets.h"

void UP1StatusWindowWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 장비 슬롯 더블클릭과 팁 버튼 연결.
    for (UP1SlotWidget* SlotWidget : { Equipped_Helmet, Equipped_Chest, Equipped_Arms, Equipped_Legs, Equipped_Boots, Equipped_Weapon })
        SlotWidget->OnDoubleClicked.AddUObject(this, &UP1StatusWindowWidget::HandleSlotDoubleClicked);

    Button_Details->OnClicked.AddUniqueDynamic(this, &UP1StatusWindowWidget::ToggleTips);

    if (auto* GameInstance = GetP1GameInstance())
    {
        // 지금 능력치와 착용 장비로 채운다.
        if (UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>())
        {
            UpdateAllStat(MyPlayerData);

            if (UP1ItemContainer* Equipped = MyPlayerData->GetContainer(EP1ContainerType::Equipped))
            {
                RedrawContainer(*Equipped);
                Equipped->OnSlotChanged.AddUObject(this, &UP1StatusWindowWidget::UpdateSlotWidget);
                Equipped->OnReset.AddUObject(this, &UP1StatusWindowWidget::RedrawContainer);
            }

            // 이후 바뀌는 값과 해제 응답을 구독.
            // 창은 한 번 만들고 표시 여부만 바꾸므로, 수치도 열 때 다시 읽지 않고 바뀔 때마다 받는다.
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MAX_HP].AddUObject(this, &UP1StatusWindowWidget::UpdateMaxHp);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MAX_MP].AddUObject(this, &UP1StatusWindowWidget::UpdateMaxMp);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_PHYSICAL_ATTACK].AddUObject(this, &UP1StatusWindowWidget::UpdatePhysicalAttack);
            MyPlayerData->OnStatChangedMappings[Protocol::STAT_TYPE_MAGICAL_ATTACK].AddUObject(this, &UP1StatusWindowWidget::UpdateMagicalAttack);
            MyPlayerData->OnRecvUnequipGearPkt.AddWeakLambda(this, [this]() { bPendingPacket = false; });
        }
    }

}

void UP1StatusWindowWidget::UpdateSlotWidget(const FP1Slot& Slot_)
{
    if (UP1SlotWidget* SlotWidget = GetSlotWidget(Slot_.Address.SlotIndex))
        SlotWidget->SetSlot(Slot_);
}

void UP1StatusWindowWidget::RedrawContainer(const UP1ItemContainer& Container)
{
    // 컨테이너에 없는 부위는 빈 칸으로 그린다. 그러지 않으면 접속이 끊긴 뒤 이전 캐릭터의 장비가 남는다.
    for (int32 GearType = Protocol::GEAR_TYPE_HELMET; GearType <= Protocol::GEAR_TYPE_WEAPON; ++GearType)
    {
        UP1SlotWidget* SlotWidget = GetSlotWidget(GearType);
        if (SlotWidget == nullptr)
            continue;

        if (const FP1Slot* Found = Container.FindSlot(GearType))
        {
            SlotWidget->SetSlot(*Found);
        }
        else
        {
            FP1Slot Empty;
            Empty.Address.Container = Container.GetContainerType();
            Empty.Address.SlotIndex = GearType;
            SlotWidget->SetSlot(Empty);
        }
    }
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

UP1SlotWidget* UP1StatusWindowWidget::GetSlotWidget(int32 GearType) const
{
    switch (static_cast<Protocol::GearType>(GearType))
    {
    case Protocol::GEAR_TYPE_HELMET:
        return Equipped_Helmet;
    case Protocol::GEAR_TYPE_CHEST:
        return Equipped_Chest;
    case Protocol::GEAR_TYPE_ARMS:
        return Equipped_Arms;
    case Protocol::GEAR_TYPE_LEGS:
        return Equipped_Legs;
    case Protocol::GEAR_TYPE_BOOTS:
        return Equipped_Boots;
    case Protocol::GEAR_TYPE_WEAPON:
        return Equipped_Weapon;
    default:
        return nullptr;
    }
}

void UP1StatusWindowWidget::HandleSlotDoubleClicked(UP1SlotWidget* SlotWidget)
{
    if (bPendingPacket)
        return;

    Protocol::C_UNEQUIP_GEAR Pkt;
    *Pkt.mutable_address() = P1ItemPackets::ToProto(SlotWidget->SlotData.Address);
    Pkt.set_item_uid(SlotWidget->SlotData.Item.ItemUid);

    // 보내기 전에 돌아가는 분기를 모두 지난 뒤에 대기를 켠다. 켜고 보내지 않으면 응답이 오지 않아 해제가 막힌다.
    bPendingPacket = true;
    FP1PacketSender::Send(this, Pkt);
}

void UP1StatusWindowWidget::ToggleTips()
{
    CanvasPanel_Tips->SetVisibility(
        CanvasPanel_Tips->IsVisible() ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
}
