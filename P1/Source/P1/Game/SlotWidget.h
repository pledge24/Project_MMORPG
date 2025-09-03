// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Structs/ItemData.h"
#include "ItemTooltipWidget.h"
#include "Engine/DataTable.h"
#include "Protocol.pb.h"
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
    virtual void NativeConstruct() override;

    /** 슬롯 UI를 초기화/갱신하는 함수 */
    UFUNCTION(BlueprintCallable, Category = "Item")
    void InitSlot(const FItemData& Item, int32 Count = 1);

    void SetSlot(const Protocol::Slot& _Slot);
    void ClearSlot();

    /** 툴팁으로 사용할 위젯 클래스 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item")
    TSubclassOf<UItemTooltipWidget> TooltipClass;

    UItemTooltipWidget* TooltipWidget;

    // 데이터 테이블 애셋 포인터
    UPROPERTY(EditAnywhere, Category = "DataTable")
    UDataTable* ItemTable;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item")
    FItemData ItemData;

    Protocol::Slot SlotData;

protected:
    /** 툴팁 반환 */ 
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    UWidget* GetToolTipWidget() const;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* ItemIcon;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* ItemCountText;

private:
    void InsertData(const Protocol::Slot& _Slot);
};
