#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Data/P1ItemData.h"
#include "UI/Common/P1ItemTooltipWidget.h"
#include "Engine/DataTable.h"
#include "Protocol.pb.h"
#include "P1SlotWidget.generated.h"

class UImage;
class UTextBlock;
class UP1ItemTooltipWidget;
class UButton;

UCLASS()
class P1_API UP1SlotWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
public:
    virtual void NativeConstruct() override;
    //~ End UUserWidget Interface

    //~ Slot Data
public:
    UFUNCTION(BlueprintCallable, Category = "Slot")
    void SetSlot(const FP1ItemData& Item, int32 Count = 1);
    void SetSlot(const Protocol::Slot& _Slot);

    void ClearSlot();
    void InsertData(const Protocol::Slot& _Slot);

    /** 아이템을 쓸 때 부른다. 구현은 블루프린트에 있다. */
    UFUNCTION(BlueprintImplementableEvent)
    void OnUse();

    /** 아이템 원본을 담은 데이터 테이블이다. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataTable")
    TObjectPtr<UDataTable> ItemTable;

    /** SlotData의 templateId가 바뀌면 함께 바뀐다. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item")
    FP1ItemData ItemData;

    Protocol::Slot SlotData;

    //~ Slot Visual
protected:
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UImage> ItemIcon;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemCountText;

    /** 슬롯이 비었을 때 쓰는 아이콘이다. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Slot")
    TObjectPtr<UTexture2D> SlotDefaultIcon;

    //~ Tooltip
protected:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    UWidget* GetToolTipWidget() const;

    /** 툴팁으로 띄울 위젯 클래스다. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Slot")
    TSubclassOf<UP1ItemTooltipWidget> TooltipClass;

    UPROPERTY()
    TObjectPtr<UP1ItemTooltipWidget> SlotTooltipWidget;
};
