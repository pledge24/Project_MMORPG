#include "UI/Common/P1ItemTooltipWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"

// 툴팁에 보일 아이템 종류의 이름이다. 종류를 모르면 빈 문자열이고, 그 줄은 비워 둔다.
static FString GetItemTypeDisplayName(Protocol::ItemType ItemType)
{
    switch (ItemType)
    {
    case Protocol::ITEM_TYPE_GEAR:          return TEXT("장비");
    case Protocol::ITEM_TYPE_CONSUMABLE:    return TEXT("소모품");
    case Protocol::ITEM_TYPE_MISCELLANEOUS: return TEXT("기타");
    default:                                return FString();
    }
}

void UP1ItemTooltipWidget::Init(const FP1ItemData& Item, UTexture2D* Icon)
{
    if (Item.TemplateId <= 0)
        return;

    if (Icon && ItemIcon)
    {
        ItemIcon->SetBrushFromTexture(Icon);
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
    SetItemDetailsToVerticalBox(ChildIdx, TEXT("아이템 종류: "), GetItemTypeDisplayName(Item.GetItemType()));

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

void UP1ItemTooltipWidget::SetItemDetailsToVerticalBox(int32& ChildIdx, FString InfoText, FString DetailsText)
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

void UP1ItemTooltipWidget::SetItemStatToVerticalBox(int32& ChildIdx, FString InfoText, int32 Value, bool IsPercent)
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

void UP1ItemTooltipWidget::SetItemStatToVerticalBox(int32& ChildIdx, FString InfoText, float Value, bool IsPercent)
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
