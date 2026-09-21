#include "UI/P1InventoryWidget.h"
#include "UI/P1SlotWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/TextBlock.h"
#include "P1.h"
#include "Core/P1GameInstance.h"
#include "Utils/LogCategory.h"
#include "Characters/P1MyPlayer.h"
#include "Core/P1MyPlayerData.h"

void UP1InventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance()))
    {
        // MyPlayerData에서 인벤토리 정보를 가져와 갱신한다.
        if (UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>())
        {
            const Protocol::PlayerInfo& PlayerInfo_ = MyPlayerData->GetPlayerInfo();
            const Protocol::Inventory& Inven_ = MyPlayerData->GetPossession()->inventory();

            UpdateGold(MyPlayerData->GetGold());

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
            MyPlayerData->OnGoldChanged.AddUObject(this, &UP1InventoryWidget::UpdateGold);
            MyPlayerData->OnInvenSlotChanged.AddUObject(this, &UP1InventoryWidget::UpdateSlotWidget);

            GameInstance->OnRecvSellItemPkt.AddLambda([this]() { if (IsValid(this)) this->PendingPacket = false; });
            GameInstance->OnRecvUseItemPkt.AddLambda([this]() { if (IsValid(this)) this->PendingPacket = false; });
            GameInstance->OnRecvEquipGearPkt.AddLambda([this]() { if (IsValid(this)) this->PendingPacket = false; });
        }
        
    }
}

void UP1InventoryWidget::Clear()
{
    for (UWidget* GridSlot : Gear_Inven->GetAllChildren())
    {
        UP1SlotWidget* Slot_ = Cast<UP1SlotWidget>(GridSlot);
        if (Slot_)
            Slot_->ClearSlot();
    }

    for (UWidget* GridSlot : Consumables_Inven->GetAllChildren())
    {
        UP1SlotWidget* Slot_ = Cast<UP1SlotWidget>(GridSlot);
        if (Slot_)
            Slot_->ClearSlot();
    }

    for (UWidget* GridSlot : Misc_Inven->GetAllChildren())
    {
        UP1SlotWidget* Slot_ = Cast<UP1SlotWidget>(GridSlot);
        if (Slot_)
            Slot_->ClearSlot();
    }
}

void UP1InventoryWidget::UpdateSlotWidget(const Protocol::Slot& InSlot, bool OnUse)
{
    UP1SlotWidget* SlotWidget = GetSlotWidgetFromSlot(InSlot);

    if (SlotWidget)
    {
        SlotWidget->SetSlot(InSlot);
        if (OnUse)
            SlotWidget->OnUse();
    }
}

void UP1InventoryWidget::UpdateGold(const int64 Gold)
{
    Gold_txt->SetText(FText::AsNumber(Gold));
}

UP1SlotWidget* UP1InventoryWidget::GetSlotWidgetFromSlot(const Protocol::Slot& InSlot)
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
        return Cast<UP1SlotWidget>(ChildWidget);
    }

    return nullptr;
}

void UP1InventoryWidget::SendSellItemPacket(UP1SlotWidget* SlotWidget)
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

void UP1InventoryWidget::SendUseItemPacket(UP1SlotWidget* SlotWidget)
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

        UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>();
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
            UE_LOG(LogP1Network, Warning, TEXT("Something Wrong in SendUseItemPacket.."));
        }
    }
}

void UP1InventoryWidget::SendEquipItemPacket(UP1SlotWidget* SlotWidget)
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

        UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>();
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
            UE_LOG(LogP1Network, Warning, TEXT("Something Wrong in EquipGearPacket.."));
        }
        
    }
}
