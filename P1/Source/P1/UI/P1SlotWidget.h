#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/P1ItemData.h"
#include "UI/P1ItemTooltipWidget.h"
#include "Engine/DataTable.h"
#include "Protocol.pb.h"
#include "P1SlotWidget.generated.h"

class UImage;
class UTextBlock;
class UP1ItemTooltipWidget;
class UButton;

/**
 * 
 */
UCLASS()
class P1_API UP1SlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    /** 슬롯 UI를 초기화/갱신하는 함수 */
    UFUNCTION(BlueprintCallable, Category = "Slot")
    void SetSlot(const FP1ItemData& Item, int32 Count = 1);
    void SetSlot(const Protocol::Slot& _Slot);

    void ClearSlot();
    void InsertData(const Protocol::Slot& _Slot);

    UFUNCTION(BlueprintImplementableEvent)
    void OnUse();

    // 데이터 테이블 애셋 포인터
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataTable")
    TObjectPtr<UDataTable> ItemTable;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item")
    FP1ItemData ItemData; // templateId가 바뀌면 변경됨

    Protocol::Slot SlotData;

protected:
    /** 툴팁 반환 */ 
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    UWidget* GetToolTipWidget() const;

    /** 툴팁으로 사용할 위젯 클래스 */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Slot")
    TSubclassOf<UP1ItemTooltipWidget> TooltipClass;

    UPROPERTY()
    TObjectPtr<UP1ItemTooltipWidget> SlotTooltipWidget;

    /** 슬롯 비쥬얼 */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UImage> ItemIcon;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemCountText;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Slot")
    TObjectPtr<UTexture2D> SlotDefaultIcon;
};
