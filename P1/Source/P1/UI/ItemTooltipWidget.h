// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BP_Structs.h"
#include "ItemTooltipWidget.generated.h"

class UImage;
class UTextBlock;
class UVerticalBox;

/**
 * 
 */
UCLASS()
class P1_API UItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    void Init(const FItemData& Item);

public:
    UPROPERTY(meta = (BindWidget))
    UImage* ItemIcon;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* ItemNameText;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* ItemDescriptionText;

    UPROPERTY(meta = (BindWidget))
    UVerticalBox* VB_ItemInfo;

private:
    void SetItemDetailsToVerticalBox(int32& ChildIdx, FString InfoText, FString DetailsText);
    void SetItemStatToVerticalBox(int32& ChildIdx, FString InfoText, int32 Value, bool IsPercent = false);
    void SetItemStatToVerticalBox(int32& ChildIdx, FString InfoText, float Value, bool IsPercent = false);
};
