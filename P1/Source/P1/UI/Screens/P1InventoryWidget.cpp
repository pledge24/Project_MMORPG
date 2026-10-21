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
#include "Game/Items/P1ItemContainer.h"
#include "Game/Items/P1ItemPackets.h"
#include "Game/Inventory/P1InventorySlotAction.h"
#include "Game/Progress/P1MyPlayerData.h"

void UP1InventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 탭 버튼과 슬롯 클릭 연결.
    InventoryTabSwitcher->SetActiveWidgetIndex(0);
    GearTabButton->OnClicked.AddUniqueDynamic(this, &UP1InventoryWidget::ShowGearTab);
    ConsumableTabButton->OnClicked.AddUniqueDynamic(this, &UP1InventoryWidget::ShowConsumableTab);
    MiscTabButton->OnClicked.AddUniqueDynamic(this, &UP1InventoryWidget::ShowMiscTab);

    BindSlotClicks(Gear_Inven);
    BindSlotClicks(Consumables_Inven);
    BindSlotClicks(Misc_Inven);

    if (auto* GameInstance = GetP1GameInstance())
    {
        // 지금 골드와 슬롯으로 채운다.
        if (UP1MyPlayerData* MyPlayerData = GameInstance->GetSubsystem<UP1MyPlayerData>())
        {
            UpdateGold(MyPlayerData->GetGold());

            // 구역마다 지금 칸으로 그리고, 이후 바뀌는 칸을 구독.
            for (EP1ContainerType Section : { EP1ContainerType::InventoryGear, EP1ContainerType::InventoryConsumable, EP1ContainerType::InventoryMisc })
            {
                if (UP1ItemContainer* Container = MyPlayerData->GetContainer(Section))
                {
                    RedrawContainer(*Container);
                    Container->OnSlotChanged.AddUObject(this, &UP1InventoryWidget::UpdateSlotWidget);
                    Container->OnReset.AddUObject(this, &UP1InventoryWidget::RedrawContainer);
                }
            }

            // 이후 바뀌는 값과 요청 응답을 구독.
            MyPlayerData->OnGoldChanged.AddUObject(this, &UP1InventoryWidget::UpdateGold);

            MyPlayerData->OnRecvSellItemPkt.AddWeakLambda(this, [this]() { bPendingPacket = false; });
            MyPlayerData->OnRecvUseItemPkt.AddWeakLambda(this, [this]() { bPendingPacket = false; });
            MyPlayerData->OnRecvEquipGearPkt.AddWeakLambda(this, [this]() { bPendingPacket = false; });
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

void UP1InventoryWidget::UpdateSlotWidget(const FP1Slot& InSlot)
{
    if (UP1SlotWidget* SlotWidget = GetSlotWidgetFromSlot(InSlot.Address))
        SlotWidget->SetSlot(InSlot);
}

void UP1InventoryWidget::RedrawContainer(const UP1ItemContainer& Container)
{
    UUniformGridPanel* Grid = GetSlotGrid(Container.GetContainerType());
    if (Grid == nullptr)
        return;

    // 컨테이너에 없는 칸 번호의 위젯은 빈 칸으로 그린다. 그러지 않으면 접속이 끊긴 뒤 이전 캐릭터의 칸이 남는다.
    for (int32 SlotIndex = 0; SlotIndex < Grid->GetChildrenCount(); ++SlotIndex)
    {
        UP1SlotWidget* SlotWidget = Cast<UP1SlotWidget>(Grid->GetChildAt(SlotIndex));
        if (SlotWidget == nullptr)
            continue;

        if (const FP1Slot* Found = Container.FindSlot(SlotIndex))
        {
            SlotWidget->SetSlot(*Found);
        }
        else
        {
            FP1Slot Empty;
            Empty.Address.Container = Container.GetContainerType();
            Empty.Address.SlotIndex = SlotIndex;
            SlotWidget->SetSlot(Empty);
        }
    }
}

void UP1InventoryWidget::UpdateGold(const int64 Gold)
{
    Gold_txt->SetText(FText::AsNumber(Gold));
}

UUniformGridPanel* UP1InventoryWidget::GetSlotGrid(EP1ContainerType Container) const
{
    switch (Container)
    {
    case EP1ContainerType::InventoryGear:
        return Gear_Inven;
    case EP1ContainerType::InventoryConsumable:
        return Consumables_Inven;
    case EP1ContainerType::InventoryMisc:
        return Misc_Inven;
    default:
        return nullptr;
    }
}

UP1SlotWidget* UP1InventoryWidget::GetSlotWidgetFromSlot(const FP1SlotAddress& Address) const
{
    UUniformGridPanel* Inven = GetSlotGrid(Address.Container);
    const int32 SlotIndex = Address.SlotIndex;

    if (!Inven || SlotIndex < 0 || SlotIndex >= Inven->GetChildrenCount())
    {
        return nullptr;
    }

    UWidget* ChildWidget = Inven->GetChildAt(SlotIndex);

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
    if (bPendingPacket)
        return;

    bPendingPacket = true;
    FP1PacketSender::Send(this, Pkt);
}

void UP1InventoryWidget::BindSlotClicks(UUniformGridPanel* SlotGrid)
{
    for (UWidget* Child : SlotGrid->GetAllChildren())
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

    // ItemData로 빈 칸을 가린다. 칸을 비우면 ItemData도 비워진다.
    const FP1Slot& SlotData = SlotWidget->SlotData;
    if (SlotWidget->ItemData.TemplateId <= 0)
    {
        UE_LOG(LogP1UI, Log, TEXT("빈 칸은 팔 수 없다."));
        return;
    }

    Protocol::C_SELL_ITEM Pkt;
    *Pkt.mutable_address() = P1ItemPackets::ToProto(SlotData.Address);
    Pkt.set_item_uid(SlotData.Item.ItemUid);
    Pkt.set_count(1);
    SendItemRequest(Pkt);
}

void UP1InventoryWidget::HandleSlotDoubleClicked(UP1SlotWidget* SlotWidget)
{
    UP1GameInstance* GameInstance = GetP1GameInstance();
    UP1MyPlayerData* MyPlayerData = GameInstance ? GameInstance->GetSubsystem<UP1MyPlayerData>() : nullptr;
    if (MyPlayerData == nullptr)
        return;

    const FP1Slot& SlotData = SlotWidget->SlotData;
    const int32 TemplateId = SlotWidget->ItemData.TemplateId;

    switch (FP1InventorySlotAction::Decide(
        SlotData.Address.Container, SlotWidget->ItemData.GetItemType(), TemplateId, SlotWidget->ItemData.LevelRequirement, MyPlayerData->GetPlayerLevel(),
        MyPlayerData->IsItemCoolingDown(TemplateId)))
    {
    case FP1InventorySlotAction::EKind::Use:
    {
        Protocol::C_USE_ITEM Pkt;
        *Pkt.mutable_address() = P1ItemPackets::ToProto(SlotData.Address);
        Pkt.set_item_uid(SlotData.Item.ItemUid);
        SendItemRequest(Pkt);
        break;
    }
    case FP1InventorySlotAction::EKind::Equip:
    {
        Protocol::C_EQUIP_GEAR Pkt;
        *Pkt.mutable_address() = P1ItemPackets::ToProto(SlotData.Address);
        Pkt.set_item_uid(SlotData.Item.ItemUid);
        SendItemRequest(Pkt);
        break;
    }
    case FP1InventorySlotAction::EKind::LevelTooLow:
        UE_LOG(LogP1UI, Log, TEXT("요구 레벨이 모자라 쓸 수 없다. 템플릿 %d"), TemplateId);
        break;
    case FP1InventorySlotAction::EKind::CoolingDown:
        UE_LOG(LogP1UI, Log, TEXT("재사용 대기 중이라 쓸 수 없다. 템플릿 %d"), TemplateId);
        break;
    default:
        break;
    }
}
