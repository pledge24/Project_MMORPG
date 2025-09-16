// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/StatusWindowWidget.h"
#include "Components/TextBlock.h"
#include "P1.h"
#include "P1MyPlayer.h"
#include "P1GameInstance.h"

void UStatusWindowWidget::NativeConstruct()
{
    Super::NativeConstruct();

    auto* PC = UGameplayStatics::GetPlayerController(this, 0);
    AP1MyPlayer* MyPlayer = Cast<AP1MyPlayer>(PC->GetPawn());

    if (MyPlayer)
    {
        // Init
        const Protocol::PlayerInfo& PlayerInfo_ = MyPlayer->GetPlayerInfo();

        UpdateAllStat(PlayerInfo_.stat_info());

        for (const Protocol::Slot& Slot_ : PlayerInfo_.equipped_gear())
        {
            UpdateSlotWidget(Slot_);
        }

        // 바인딩 셋업
        MyPlayer->OnStatInfoChanged.AddUObject(this, &UStatusWindowWidget::UpdateAllStat);
        MyPlayer->OnEquippedGearSlotChanged.AddUObject(this, &UStatusWindowWidget::UpdateSlotWidget);

        MyPlayer->OnRep_UnequipGear.AddLambda([this]() { if (IsValid(this)) PendingPacket = false; });
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
    }

    if (SlotWidget)
        SlotWidget->SetSlot(Slot_);
}

void UStatusWindowWidget::UpdateAllStat(const Protocol::StatInfo& StatInfo_)
{
    if(StatInfo_.has_max_hp())
        UpdateMaxHp(StatInfo_.max_hp());
    if(StatInfo_.has_max_mp())
        UpdateMaxMp(StatInfo_.max_mp());
    if(StatInfo_.has_physical_attack())
        UpdatePhysicalAttack(StatInfo_.physical_attack());
    if(StatInfo_.has_magical_attack())
        UpdateMagicalAttack(StatInfo_.magical_attack());
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

    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("OnUnequip! template_id: %d")));
    
    if (Slot_)
    {
        const Protocol::Slot& SlotData = Slot_->SlotData;

        Protocol::C_UNEQUIP_GEAR pkt;
        pkt.mutable_slot()->CopyFrom(SlotData);
        SEND_PACKET(pkt);
    }
}
