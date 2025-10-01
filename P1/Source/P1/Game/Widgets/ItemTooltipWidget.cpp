// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/ItemTooltipWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"

void UItemTooltipWidget::Init(const FItemData& Item)
{
    if (Item.TemplateId <= 0)
        return;

    if (Item.Icon.IsValid() || Item.Icon.ToSoftObjectPath().IsValid())
    {
        UTexture2D* LoadedIcon = Item.Icon.LoadSynchronous();
        if (LoadedIcon && ItemIcon)
        {
            ItemIcon->SetBrushFromTexture(LoadedIcon);
        }
    }

    if (ItemNameText)
    {
        ItemNameText->SetText(FText::FromString(Item.ItemName));
    }

    if (ItemDescriptionText)
    {
        ItemDescriptionText->SetText(FText::FromString(Item.Description));
    }

    // Clear Text
    for (UWidget* Child : VB_ItemInfo->GetAllChildren())
    {
        if (UTextBlock* TextBlock = Cast<UTextBlock>(Child))
        {
            TextBlock->SetText(FText::GetEmpty());
        }
    }

    int32 ChildIdx = 0;
    SetItemDetailsToVerticalBox(ChildIdx, TEXT("아이템 타입: "), Item.ItemType);

    SetItemStatToVerticalBox(ChildIdx, TEXT("레벨 제한: "), Item.LevelRequirement);
    SetItemStatToVerticalBox(ChildIdx, TEXT("쿨타임: "), Item.Cooldown);
    SetItemStatToVerticalBox(ChildIdx, TEXT("물리 피해 +"), Item.PhysicalAttack);
    SetItemStatToVerticalBox(ChildIdx, TEXT("마법 피해 +"), Item.MagicalAttack);
    SetItemStatToVerticalBox(ChildIdx, TEXT("추가 Hp +"), Item.Hp);
    SetItemStatToVerticalBox(ChildIdx, TEXT("추가 Mp +"), Item.Mp);
    SetItemStatToVerticalBox(ChildIdx, TEXT("Hp 회복 +"), Item.HpRestore, true);
    SetItemStatToVerticalBox(ChildIdx, TEXT("Mp 회복 +"), Item.MpRestore, true);
    SetItemStatToVerticalBox(ChildIdx, TEXT("상점 구매 가격: "), Item.BuyPrice);
    SetItemStatToVerticalBox(ChildIdx, TEXT("상점 판매 가격: "), Item.SellPrice);
}

void UItemTooltipWidget::SetItemDetailsToVerticalBox(int32& ChildIdx, FString InfoText, FString DetailsText)
{
    if (ChildIdx >= VB_ItemInfo->GetChildrenCount())
        return;

    if (DetailsText.IsEmpty() == false)
    {
        if (UTextBlock* TextBlock = Cast<UTextBlock>(VB_ItemInfo->GetChildAt(ChildIdx)))
        {
            FString StatText = FString::Printf(TEXT("%s %s"), *InfoText, *DetailsText);
            TextBlock->SetText(FText::FromString(StatText));
            ChildIdx++;
        }
    }
    
}

void UItemTooltipWidget::SetItemStatToVerticalBox(int32& ChildIdx, FString InfoText, int32 Value, bool IsPercent)
{
    if (ChildIdx >= VB_ItemInfo->GetChildrenCount())
        return;

    if (Value > 0)
    {
        if (UTextBlock* TextBlock = Cast<UTextBlock>(VB_ItemInfo->GetChildAt(ChildIdx)))
        {
            FString StatText = IsPercent ? FString::Printf(TEXT("%s %d%%"), *InfoText, Value)
                : FString::Printf(TEXT("%s %d"), *InfoText, Value);
            TextBlock->SetText(FText::FromString(StatText));
            ChildIdx++;
        }
    }
}

void UItemTooltipWidget::SetItemStatToVerticalBox(int32& ChildIdx, FString InfoText, float Value, bool IsPercent)
{
    if (ChildIdx >= VB_ItemInfo->GetChildrenCount())
        return;

    if (Value > 0)
    {
        if (UTextBlock* TextBlock = Cast<UTextBlock>(VB_ItemInfo->GetChildAt(ChildIdx)))
        {
            FString StatText = IsPercent ? FString::Printf(TEXT("%s %.1f%%"), *InfoText, Value*100)
                : FString::Printf(TEXT("%s %f"), *InfoText, Value);
            TextBlock->SetText(FText::FromString(StatText));
            ChildIdx++;
        }
    }
}
