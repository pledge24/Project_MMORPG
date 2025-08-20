// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Structs/ItemData.h"
#include "ItemTooltipWidget.h"
#include "SlotWidget.generated.h"

class UImage;
class UTextBlock;
class UItemTooltipWidget;

/**
 * 
 */
UCLASS()
class P1_API USlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    /** 슬롯 UI를 초기화/갱신하는 함수 */
    UFUNCTION(BlueprintCallable, Category = "Item")
    void InitSlot(const FItemData& Item, int32 Quantity = 1);

    /** 슬롯 UI를 초기화/갱신하는 함수 */
    UFUNCTION(BlueprintCallable, Category = "Item")
    void ClearSlot();

    /** 슬롯에 표시할 아이템 정보 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Item")
    FItemData SlotItem;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item")
    int32 SlotItemQuantity;

    /** 툴팁으로 사용할 위젯 클래스 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item")
    TSubclassOf<UItemTooltipWidget> TooltipClass;

protected:
    /** 툴팁 반환 */ 
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    UWidget* GetToolTipWidget() const;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* ItemIcon;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* ItemQuantityText;
};
