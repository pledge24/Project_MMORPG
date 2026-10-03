#include "UI/Screens/P1InventoryWidget.h"
#include "UI/Common/P1SlotWidget.h"
#include "UI/P1ScreenSubsystem.h"
#include "Components/Button.h"
#include "Components/UniformGridPanel.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Network/P1PacketSender.h"
#include "Core/P1GameInstance.h"
#include "Utils/LogCategory.h"
#include "Game/Inventory/P1InventorySlotAction.h"
#include "Game/Progress/P1MyPlayerData.h"

void UP1InventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    InventoryTabSwitcher->SetActiveWidgetIndex(0);
    GearTabButton->OnClicked.AddUniqueDynamic(this, &UP1InventoryWidget::ShowGearTab);
    ConsumableTabButton->OnClicked.AddUniqueDynamic(this, &UP1InventoryWidget::ShowConsumableTab);
    MiscTabButton->OnClicked.AddUniqueDynamic(this, &UP1InventoryWidget::ShowMiscTab);

    BindSlotClicks(Gear_Inven);
    BindSlotClicks(Consumables_Inven);
    BindSlotClicks(Misc_Inven);

    if (auto* GameInstance = GetP1GameInstance())
    {
        // MyPlayerData에서 인벤토리 정보를 가져와 갱신한다.
        if (UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>())
        {
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

            MyPlayerData->OnRecvSellItemPkt.AddWeakLambda(this, [this]() { PendingPacket = false; });
            MyPlayerData->OnRecvUseItemPkt.AddWeakLambda(this, [this]() { PendingPacket = false; });
            MyPlayerData->OnRecvEquipGearPkt.AddWeakLambda(this, [this]() { PendingPacket = false; });
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

void UP1InventoryWidget::ShowGearTab()
{
    InventoryTabSwitcher->SetActiveWidgetIndex(0);
}

void UP1InventoryWidget::ShowConsumableTab()
{
    InventoryTabSwitcher->SetActiveWidgetIndex(1);
}

void UP1InventoryWidget::ShowMiscTab()
{
    InventoryTabSwitcher->SetActiveWidgetIndex(2);
}

template <typename TPacket>
void UP1InventoryWidget::SendItemRequest(TPacket& Pkt)
{
    // 보내기 전에 돌아가는 분기를 모두 지난 뒤에 켠다. 켜고 보내지 않으면 응답이 오지 않아 대기가 풀리지 않는다.
    if (PendingPacket)
        return;

    PendingPacket = true;
    FP1PacketSender::Send(this, Pkt);
}

void UP1InventoryWidget::BindSlotClicks(UUniformGridPanel* Inven)
{
    for (UWidget* Child : Inven->GetAllChildren())
    {
        if (UP1SlotWidget* SlotWidget = Cast<UP1SlotWidget>(Child))
        {
            SlotWidget->OnRightClicked.AddUObject(this, &UP1InventoryWidget::HandleSlotRightClicked);
            SlotWidget->OnDoubleClicked.AddUObject(this, &UP1InventoryWidget::HandleSlotDoubleClicked);
        }
    }
}

void UP1InventoryWidget::HandleSlotRightClicked(UP1SlotWidget* SlotWidget)
{
    const UP1ScreenSubsystem* Screens = ULocalPlayer::GetSubsystem<UP1ScreenSubsystem>(GetOwningLocalPlayer());
    if (Screens == nullptr || !Screens->IsWindowOpen(EP1WidgetType::WIDGET_SHOP))
        return;

    // 빈 칸은 ItemData가 아니라 SlotData로 가린다. 칸을 비울 때 ItemData는 지난 아이템을 그대로 들고 있다.
    const Protocol::Slot& SlotData = SlotWidget->SlotData;
    if (!SlotData.has_item() || SlotData.item().template_id() <= 0)
    {
        UE_LOG(LogP1UI, Log, TEXT("빈 칸은 팔 수 없다."));
        return;
    }

    Protocol::C_SELL_ITEM Pkt;
    Pkt.mutable_slot()->CopyFrom(SlotData);
    Pkt.set_count(1);
    SendItemRequest(Pkt);
}

void UP1InventoryWidget::HandleSlotDoubleClicked(UP1SlotWidget* SlotWidget)
{
    UP1GameInstance* GameInstance = GetP1GameInstance();
    UP1MyPlayerData* MyPlayerData = GameInstance ? GameInstance->GetSubsystem<UP1MyPlayerData>() : nullptr;
    if (MyPlayerData == nullptr)
        return;

    const Protocol::Slot& SlotData = SlotWidget->SlotData;
    const int32 TemplateId = SlotData.has_item() ? SlotData.item().template_id() : 0;

    switch (FP1InventorySlotAction::Decide(
        SlotData.type(), TemplateId, SlotWidget->ItemData.LevelRequirement, MyPlayerData->GetPlayerLevel()))
    {
    case FP1InventorySlotAction::EKind::Use:
    {
        Protocol::C_USE_ITEM Pkt;
        Pkt.mutable_slot()->CopyFrom(SlotData);
        SendItemRequest(Pkt);
        break;
    }
    case FP1InventorySlotAction::EKind::Equip:
    {
        Protocol::C_EQUIP_GEAR Pkt;
        Pkt.mutable_slot()->CopyFrom(SlotData);
        SendItemRequest(Pkt);
        break;
    }
    case FP1InventorySlotAction::EKind::LevelTooLow:
        UE_LOG(LogP1UI, Log, TEXT("요구 레벨이 모자라 쓸 수 없다. 템플릿 %d"), TemplateId);
        break;
    default:
        break;
    }
}
