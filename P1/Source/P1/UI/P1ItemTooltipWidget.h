#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/BP_Structs.h"
#include "P1ItemTooltipWidget.generated.h"

class UImage;
class UTextBlock;
class UVerticalBox;

/**
 * 
 */
UCLASS()
class P1_API UP1ItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    void Init(const FP1ItemData& Item);

public:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> ItemIcon;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemNameText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemDescriptionText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> VB_ItemInfo;

private:
    void SetItemDetailsToVerticalBox(int32& ChildIdx, FString InfoText, FString DetailsText);
    void SetItemStatToVerticalBox(int32& ChildIdx, FString InfoText, int32 Value, bool IsPercent = false);
    void SetItemStatToVerticalBox(int32& ChildIdx, FString InfoText, float Value, bool IsPercent = false);
};
