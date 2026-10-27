#include "UI/Common/P1ItemTooltipWidget.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/Data/Generated/P1ItemConsumeEffectRow.h"
#include "Game/Data/Generated/P1ItemEquipStatRow.h"
#include "Game/Data/Generated/P1ItemRow.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"

// 툴팁에 보일 아이템 종류의 이름이다. 종류를 모르면 빈 문자열이고, 그 줄은 적지 않는다.
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

void UP1ItemTooltipWidget::Init(const FP1ItemRow& Item, UTexture2D* Icon)
{
    if (Item.Id <= 0)
        return;

    if (Icon && ItemIcon)
    {
        ItemIcon->SetBrushFromTexture(Icon);
    }

    if (ItemNameText)
    {
        ItemNameText->SetText(FText::FromString(Item.Name));
    }

    if (ItemDescriptionText)
    {
        ItemDescriptionText->SetText(FText::FromString(Item.Description));
    }

    // 툴팁 위젯은 다시 쓰이므로 앞서 띄운 아이템의 줄을 먼저 지운다. 지우지 않으면 이번 아이템에 없는 정보가 남는다.
    for (UWidget* Child : VB_ItemInfo->GetAllChildren())
    {
        if (UTextBlock* TextBlock = Cast<UTextBlock>(Child))
        {
            TextBlock->SetText(FText::GetEmpty());
        }
    }

    // 종류별 값은 그 종류의 표에만 행이 있다. 행이 없는 줄은 0으로 보고 적지 않는다.
    const FP1ItemEquipStatRow* EquipStat = UP1GameDataSettings::FindItemEquipStatRow(Item.Id);
    const FP1ItemConsumeEffectRow* ConsumeEffect = UP1GameDataSettings::FindItemConsumeEffectRow(Item.Id);

    int32 ChildIdx = 0;
    SetItemDetailsToVerticalBox(ChildIdx, TEXT("아이템 종류: "), GetItemTypeDisplayName(Item.GetType()));

    SetItemStatToVerticalBox(ChildIdx, TEXT("레벨 제한: "), Item.LevelRequirement);
    SetItemStatToVerticalBox(ChildIdx, TEXT("쿨타임: "), ConsumeEffect ? ConsumeEffect->CooldownMs / 1000.f : 0.f);
    SetItemStatToVerticalBox(ChildIdx, TEXT("물리 피해 +"), EquipStat ? EquipStat->PhysicalAttack : 0);
    SetItemStatToVerticalBox(ChildIdx, TEXT("마법 피해 +"), EquipStat ? EquipStat->MagicalAttack : 0);
    SetItemStatToVerticalBox(ChildIdx, TEXT("추가 Hp +"), EquipStat ? EquipStat->Hp : 0);
    SetItemStatToVerticalBox(ChildIdx, TEXT("추가 Mp +"), EquipStat ? EquipStat->Mp : 0);
    SetItemStatToVerticalBox(ChildIdx, TEXT("Hp 회복 +"), ConsumeEffect ? ConsumeEffect->HpRestoreRatio : 0.f, true);
    SetItemStatToVerticalBox(ChildIdx, TEXT("Mp 회복 +"), ConsumeEffect ? ConsumeEffect->MpRestoreRatio : 0.f, true);
    SetItemStatToVerticalBox(ChildIdx, TEXT("상점 구매 가격: "), static_cast<int32>(Item.BuyPrice));
    SetItemStatToVerticalBox(ChildIdx, TEXT("상점 판매 가격: "), static_cast<int32>(Item.SellPrice));
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
