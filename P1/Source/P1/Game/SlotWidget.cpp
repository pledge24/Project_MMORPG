// Fill out your copyright notice in the Description page of Project Settings.

#include "SlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "P1.h"
#include "Engine/DataTable.h"
#include "ItemTooltipWidget.h"
#include "Components/Button.h"

void USlotWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (TooltipClass && !SlotTooltipWidget)
        SlotTooltipWidget = CreateWidget<UItemTooltipWidget>(this, TooltipClass);
}

void USlotWidget::SetSlot(const FItemData& Item, int32 Count)
{
    ItemData = Item;
    SlotData.mutable_item()->set_count(Count);

    if (!Item.Icon.IsNull())
    {
        if (UTexture2D* LoadedIcon = Item.Icon.LoadSynchronous())
        {
            ItemIcon->SetBrushFromTexture(LoadedIcon);
        }
    }

    if (Count > 1)
        ItemCountText->SetText(FText::AsNumber(Count));
    else
        ItemCountText->SetText(FText::GetEmpty());
}

void USlotWidget::SetSlot(const Protocol::Slot& _Slot)
{
    // 슬롯 정보 저장(언리얼 방식으로)
    switch (_Slot.state())
    {
    case Protocol::UpdateState::UPDATE_STATE_ADDED:
        InsertData(_Slot);
        break;
    case Protocol::UpdateState::UPDATE_STATE_MODIFIED:
        SlotData.CopyFrom(_Slot);
        break;
    case Protocol::UpdateState::UPDATE_STATE_REMOVED:
        ClearSlot();
        break;
    }
    
    if (SlotData.item().count() > 1)
        ItemCountText->SetText(FText::AsNumber(SlotData.item().count()));
    else
        ItemCountText->SetText(FText::GetEmpty());
}

void USlotWidget::ClearSlot()
{
    if (ItemIcon) ItemIcon->SetBrushFromTexture(SlotDefaultIcon);
    if (ItemCountText) ItemCountText->SetText(FText::GetEmpty());
    SlotData.Clear();
}

void USlotWidget::InsertData(const Protocol::Slot& _Slot)
{
    SlotData.CopyFrom(_Slot);
    FString TemplateId_Str = FString::FromInt(_Slot.item().template_id());

    if (ItemTable)
    {
        ItemData = *ItemTable->FindRow<FItemData>(FName(*TemplateId_Str), FString("USlotWidget::InsertData"));

        if (!ItemData.Icon.IsNull())
        {
            if (UTexture2D* LoadedIcon = ItemData.Icon.LoadSynchronous())
            {
                ItemIcon->SetBrushFromTexture(LoadedIcon);
            }
        }
    }
}

UWidget* USlotWidget::GetToolTipWidget_Implementation() const
{
    if (ItemData.TemplateId > 0 && TooltipClass)
    {
        if (SlotTooltipWidget)
        {
            SlotTooltipWidget->Init(ItemData); // 아이템 정보 전달
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("GetToolTipWidget_Implementation_Success")));
            return SlotTooltipWidget;
        }
    }
    
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("GetToolTipWidget_Implementation")));

    return nullptr;
}
