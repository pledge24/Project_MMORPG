// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/InventoryWidget.h"
#include "Widgets/SlotWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/TextBlock.h"
#include "P1.h"
#include "P1GameInstance.h"
#include "Log/LogCategory.h"
#include "P1MyPlayer.h"
#include "MyPlayerData.h"

void UInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance()))
    {
        // MyPlayerData에서 인벤토리 정보를 가져와 갱신한다.
        if (UMyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UMyPlayerData>())
        {
            const Protocol::PlayerInfo& PlayerInfo_ = MyPlayerData->GetPlayerInfo();
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

            // MyPlayer 스폰 이벤트에 함수 등록
            MyPlayerData->OnMyPlayerSpawned.AddUObject(this, &UInventoryWidget::BindMyPlayerSpawned);
        }
        
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

void UInventoryWidget::BindMyPlayerSpawned(AP1MyPlayer* MyPlayer)
{
    // 바인딩 셋업
    MyPlayer->OnGoldChanged.AddUObject(this, &UInventoryWidget::UpdateGold);
    MyPlayer->OnInvenSlotChanged.AddUObject(this, &UInventoryWidget::UpdateSlotWidget);

    MyPlayer->OnRecvSellItemPkt.AddLambda([this]() { if (IsValid(this)) this->PendingPacket = false; });
    MyPlayer->OnRecvUseItemPkt.AddLambda([this]() { if (IsValid(this)) this->PendingPacket = false; });
    MyPlayer->OnRecvEquipGearPkt.AddLambda([this]() { if (IsValid(this)) this->PendingPacket = false; });
}

void UInventoryWidget::UpdateSlotWidget(const Protocol::Slot& InSlot, bool OnUse)
{
    USlotWidget* SlotWidget = GetSlotWidgetFromSlot(InSlot);

    if (SlotWidget)
    {
        SlotWidget->SetSlot(InSlot);
        if (OnUse)
            SlotWidget->OnUse();
    }
}

void UInventoryWidget::UpdateGold(const int64 Gold)
{
    Gold_txt->SetText(FText::AsNumber(Gold));
}

USlotWidget* UInventoryWidget::GetSlotWidgetFromSlot(const Protocol::Slot& InSlot)
{
    UUniformGridPanel* Inven = nullptr;
    int32 SlotId = InSlot.slot_id();

    switch (InSlot.type())
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

void UInventoryWidget::SendSellItemPacket(USlotWidget* SlotWidget)
{
    if (PendingPacket)
        return;
    else
        PendingPacket = true;

    if (SlotWidget == nullptr)
    {
        PendingPacket = false;
        return;
    }

    const Protocol::Slot& SlotData = SlotWidget->SlotData;

    Protocol::C_SELL_ITEM Pkt;
    Pkt.mutable_slot()->CopyFrom(SlotData);
    Pkt.set_count(1);
    SEND_PACKET(Pkt);
    
}

void UInventoryWidget::SendUseItemPacket(USlotWidget* SlotWidget)
{
    if (PendingPacket)
        return;
    else
        PendingPacket = true;

    if (SlotWidget)
    {
        const Protocol::Slot& SlotData = SlotWidget->SlotData;

        auto* PC = UGameplayStatics::GetPlayerController(this, 0);
        auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance());
        if (GameInstance == nullptr)
            return;

        UMyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UMyPlayerData>();
        int32 Level = MyPlayerData->GetPlayerLevel();
        if (Level < SlotWidget->ItemData.LevelRequirement)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Level Restricted!")));
            PendingPacket = false;
            return;
        }

        if (SlotData.type() == Protocol::SlotType::SLOT_TYPE_INVENTORY_CONSUMABLE)
        {
            Protocol::C_USE_ITEM Pkt;
            Pkt.mutable_slot()->CopyFrom(SlotData);
            SEND_PACKET(Pkt);
        }
        else
        {
            UE_LOG(LogNetwork, Warning, TEXT("Something Wrong in SendUseItemPacket.."));
        }
    }
}

void UInventoryWidget::SendEquipItemPacket(USlotWidget* SlotWidget)
{
    if (PendingPacket)
        return;
    else
        PendingPacket = true;

    if (SlotWidget)
    {
        const Protocol::Slot& SlotData = SlotWidget->SlotData;

        auto* PC = UGameplayStatics::GetPlayerController(this, 0);
        auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance());
        if (GameInstance == nullptr)
            return;

        UMyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UMyPlayerData>();
        int32 Level = MyPlayerData->GetPlayerLevel();
        if (Level < SlotWidget->ItemData.LevelRequirement)
        {
            PendingPacket = false;
            return;
        }

        if (SlotData.type() == Protocol::SlotType::SLOT_TYPE_INVENTORY_GEAR)
        {
            Protocol::C_EQUIP_GEAR Pkt;
            Pkt.mutable_slot()->CopyFrom(SlotData);
            SEND_PACKET(Pkt);
        }
        else
        {
            UE_LOG(LogNetwork, Warning, TEXT("Something Wrong in EquipGearPacket.."));
        }
        
    }
}
