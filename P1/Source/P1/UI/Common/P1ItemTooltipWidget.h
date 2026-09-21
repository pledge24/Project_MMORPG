#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Game/Data/BP_Structs.h"
#include "P1ItemTooltipWidget.generated.h"

class UImage;
class UTextBlock;
class UVerticalBox;

UCLASS()
class P1_API UP1ItemTooltipWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Item Display
public:
    void Init(const FP1ItemData& Item);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> ItemIcon;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemNameText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemDescriptionText;

    //~ Item Details
public:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> VB_ItemInfo;

private:
    /** ChildIdx는 채운 칸의 다음 자리로 옮겨진다. */
    void SetItemDetailsToVerticalBox(int32& ChildIdx, FString InfoText, FString DetailsText);
    void SetItemStatToVerticalBox(int32& ChildIdx, FString InfoText, int32 Value, bool IsPercent = false);
    void SetItemStatToVerticalBox(int32& ChildIdx, FString InfoText, float Value, bool IsPercent = false);
};
