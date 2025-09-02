// Fill out your copyright notice in the Description page of Project Settings.

#include "SlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "P1.h"
#include "Engine/DataTable.h"
#include "ItemTooltipWidget.h"

void USlotWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!TooltipWidget)
        TooltipWidget = CreateWidget<UItemTooltipWidget>(this, TooltipClass);
}

void USlotWidget::InitSlot(const FItemData& Item, int32 Count)
{
    ItemData = Item;
    SlotData.set_count(Count);

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
    case Protocol::UpdateState::UPDATE_STATE_INSERT:
        InsertData(_Slot);
        break;
    case Protocol::UpdateState::UPDATE_STATE_UPDATE:
        SlotData.CopyFrom(_Slot);
        break;
    case Protocol::UpdateState::UPDATE_STATE_DELETE:
        ClearSlot();
        break;
    }
    
    if (SlotData.count() > 1)
        ItemCountText->SetText(FText::AsNumber(SlotData.count()));
    else
        ItemCountText->SetText(FText::GetEmpty());
}

void USlotWidget::ClearSlot()
{
    if (ItemIcon) ItemIcon->SetBrushFromTexture(nullptr);
    if (ItemCountText) ItemCountText->SetText(FText::GetEmpty());
    SlotData.Clear();
}

void USlotWidget::InsertData(const Protocol::Slot& _Slot)
{
    SlotData.CopyFrom(_Slot);
    static const FString Context(TEXT("LookupRow"));
    FString TemplateId_Str = FString::FromInt(_Slot.item().template_id());
    ItemData = *ItemTable->FindRow<FItemData>(FName(*TemplateId_Str), Context);

    if (!ItemData.Icon.IsNull())
    {
        if (UTexture2D* LoadedIcon = ItemData.Icon.LoadSynchronous())
        {
            ItemIcon->SetBrushFromTexture(LoadedIcon);
        }
    }
}

UWidget* USlotWidget::GetToolTipWidget_Implementation() const
{
    if (ItemData.TemplateId > 0 && TooltipClass)
    {
        if (TooltipWidget)
        {
            TooltipWidget->Init(ItemData); // 아이템 정보 전달
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("GetToolTipWidget_Implementation_Success")));
            return TooltipWidget;
        }
    }
    
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("GetToolTipWidget_Implementation")));

    return nullptr;
}
