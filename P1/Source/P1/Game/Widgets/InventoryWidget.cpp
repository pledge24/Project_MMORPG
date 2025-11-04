// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/InventoryWidget.h"
#include "Widgets/SlotWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/TextBlock.h"
#include "P1.h"
#include "P1GameInstance.h"
#include "Log/LogCategory.h"
#include "P1MyPlayer.h"
#include "InventoryComponent.h"

void UInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance()))
    {
        // Init
        const Protocol::PlayerInfo& PlayerInfo_ = GameInstance->MyPlayer->GetPlayerInfo();
        const Protocol::Inventory& Inven_ = PlayerInfo_.inventory();

        UpdateGold(PlayerInfo_.gold());

        for (const Protocol::Slot& Slot_ : Inven_.gear())
        {
            UpdateSlotWidget(Slot_);
        }

        for (const Protocol::Slot& Slot_ : Inven_.consumables())
        {
            UpdateSlotWidget(Slot_);
        }

        for (const Protocol::Slot& Slot_ : Inven_.miscellaneous())
        {
            UpdateSlotWidget(Slot_);
        }
        
        // 바인딩 셋업
        GameInstance->MyPlayer->OnGoldChanged.AddUObject(this, &UInventoryWidget::UpdateGold);
        //GameInstance->MyPlayer->GetInventory()->OnSlotChanged.AddUObject(this, &UInventoryWidget::UpdateSlotWidget);

        GameInstance->MyPlayer->OnRecvSellItemPkt.AddLambda([this]() { if(IsValid(this)) PendingPacket = false; });
        GameInstance->MyPlayer->OnRecvUseItemPkt.AddLambda([this]() { if (IsValid(this)) PendingPacket = false; });
        GameInstance->MyPlayer->OnRecvEquipGearPkt.AddLambda([this]() { if (IsValid(this)) PendingPacket = false; });
    }
}

void UInventoryWidget::Clear()
{
    for (UWidget* GridSlot : Gear_Inven->GetAllChildren())
    {
        USlotWidget* Slot_ = Cast<USlotWidget>(GridSlot);
        if (Slot_)
            Slot_->ClearSlot();
    }

    for (UWidget* GridSlot : Consumables_Inven->GetAllChildren())
    {
        USlotWidget* Slot_ = Cast<USlotWidget>(GridSlot);
        if (Slot_)
            Slot_->ClearSlot();
    }

    for (UWidget* GridSlot : Misc_Inven->GetAllChildren())
    {
        USlotWidget* Slot_ = Cast<USlotWidget>(GridSlot);
        if (Slot_)
            Slot_->ClearSlot();
    }
}

void UInventoryWidget::UpdateSlotWidget(const Protocol::Slot& _Slot, bool OnUse)
{
    USlotWidget* SlotWidget = GetSlotWidgetFromSlot(_Slot);

    if (SlotWidget)
    {
        SlotWidget->SetSlot(_Slot);
        if (OnUse)
            SlotWidget->OnUse();
    }
}

void UInventoryWidget::UpdateGold(int32 Gold)
{
    Gold_txt->SetText(FText::AsNumber(Gold));
}

USlotWidget* UInventoryWidget::GetSlotWidgetFromSlot(const Protocol::Slot& _Slot)
{
    UUniformGridPanel* Inven = nullptr;
    int32 SlotId = _Slot.slot_id();

    switch (_Slot.type())
    {
    case Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR:
        Inven = Gear_Inven;
        break;
    case Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE:
        Inven = Consumables_Inven;
        break;
    case Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC:
        Inven = Misc_Inven;
        break;
    }

    if (!Inven || SlotId < 0 || SlotId >= Inven->GetChildrenCount())
    {
        return nullptr;
    }

    UWidget* ChildWidget = Inven->GetChildAt(SlotId);

    if (ChildWidget)
    {
        return Cast<USlotWidget>(ChildWidget);
    }

    return nullptr;
}

void UInventoryWidget::SendSellItemPacket(USlotWidget* _Slot)
{
    if (PendingPacket)
        return;
    else
        PendingPacket = true;

    if (_Slot == nullptr)
    {
        PendingPacket = false;
        return;
    }

    const Protocol::Slot& SlotData = _Slot->SlotData;

    Protocol::C_SELL_ITEM pkt;
    pkt.mutable_slot()->CopyFrom(SlotData);
    pkt.set_count(1);
    SEND_PACKET(pkt);
    
}

void UInventoryWidget::SendUseItemPacket(USlotWidget* _Slot)
{
    if (PendingPacket)
        return;
    else
        PendingPacket = true;

    if (_Slot)
    {
        const Protocol::Slot& SlotData = _Slot->SlotData;

        auto* PC = UGameplayStatics::GetPlayerController(this, 0);
        auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance());
        if (GameInstance == nullptr)
            return;

        int32 Level = GameInstance->MyPlayer->GetPlayerLevel();
        if (Level < _Slot->ItemData.LevelRequirement)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Level Restricted!")));
            PendingPacket = false;
            return;
        }

        if (SlotData.type() == Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE)
        {
            Protocol::C_USE_ITEM pkt;
            pkt.mutable_slot()->CopyFrom(SlotData);
            SEND_PACKET(pkt);
        }
        else
        {
            UE_LOG(LogNetwork, Warning, TEXT("Something Wrong in SendUseItemPacket.."));
        }
    }
}

void UInventoryWidget::SendEquipItemPacket(USlotWidget* _Slot)
{
    if (PendingPacket)
        return;
    else
        PendingPacket = true;

    if (_Slot)
    {
        const Protocol::Slot& SlotData = _Slot->SlotData;

        auto* PC = UGameplayStatics::GetPlayerController(this, 0);
        auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance());
        if (GameInstance == nullptr)
            return;

        int32 Level = GameInstance->MyPlayer->GetPlayerLevel();
        if (Level < _Slot->ItemData.LevelRequirement)
        {
            PendingPacket = false;
            return;
        }

        if (SlotData.type() == Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR)
        {
            Protocol::C_EQUIP_GEAR pkt;
            pkt.mutable_slot()->CopyFrom(SlotData);
            SEND_PACKET(pkt);
        }
        else
        {
            UE_LOG(LogNetwork, Warning, TEXT("Something Wrong in EquipGearPacket.."));
        }
        
    }
}
