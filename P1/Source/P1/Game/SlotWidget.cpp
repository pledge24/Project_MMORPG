// Fill out your copyright notice in the Description page of Project Settings.

#include "SlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "ItemTooltipWidget.h"

void USlotWidget::InitSlot(const FItemData& Item, int32 Quantity)
{
    SlotItem = Item;
    SlotItemQuantity = Quantity;

    if (!Item.Icon.IsNull())
    {
        if (UTexture2D* LoadedIcon = Item.Icon.LoadSynchronous())
        {
            ItemIcon->SetBrushFromTexture(LoadedIcon);
        }
    }


    if (Quantity > 1)
        ItemQuantityText->SetText(FText::AsNumber(Quantity));
    else
        ItemQuantityText->SetText(FText::GetEmpty());
}

void USlotWidget::ClearSlot()
{
    if (ItemIcon) ItemIcon->SetBrushFromTexture(nullptr);
    if (ItemQuantityText) ItemQuantityText->SetText(FText::GetEmpty());
    SlotItem = FItemData{};
    SlotItemQuantity = 0;
}


UWidget* USlotWidget::GetToolTipWidget_Implementation() const
{
    if (SlotItem.TemplateId > 0 && TooltipClass)
    {
        UItemTooltipWidget* Tooltip = CreateWidget<UItemTooltipWidget>(GetWorld(), TooltipClass);
        if (Tooltip)
        {
            Tooltip->Init(SlotItem); // 아이템 정보 전달
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("GetToolTipWidget_Implementation_Success")));
            return Tooltip;
        }
    }
    
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("GetToolTipWidget_Implementation")));

    return nullptr;
}
