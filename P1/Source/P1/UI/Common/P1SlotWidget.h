#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Game/Data/P1ItemData.h"
#include "UI/Common/P1ItemTooltipWidget.h"
#include "Engine/DataTable.h"
#include "Protocol.pb.h"
#include "P1SlotWidget.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;
class UP1ItemTooltipWidget;
class UButton;
class UP1SlotWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSlotClicked, UP1SlotWidget*);

UCLASS()
class P1_API UP1SlotWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
public:
    virtual void NativeConstruct() override;

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativePreConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    //~ End UUserWidget Interface

    //~ Click
public:
    /** 아이콘 위에서 우클릭하면 알린다. 빈 칸에서도 알린다. */
    FOnSlotClicked OnRightClicked;

    /** 아이콘 위에서 좌클릭으로 더블클릭하면 알린다. 빈 칸에서도 알린다. */
    FOnSlotClicked OnDoubleClicked;

private:
    bool IsOverIcon(const FPointerEvent& InMouseEvent) const;

    //~ Slot Data
public:
    void SetSlot(const FP1ItemData& Item, int32 Count = 1);
    void SetSlot(const Protocol::Slot& _Slot);

    void ClearSlot();
    void InsertData(const Protocol::Slot& _Slot);

    /** 아이템 아이콘을 담은 에셋 테이블이다. 행 구조체는 FP1ItemAssetData다. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataTable")
    TObjectPtr<UDataTable> ItemAssetTable;

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

private:
    /** 에셋 테이블에서 아이콘을 찾아 표시한다. */
    void ApplyIcon(int32 TemplateId);

    /** 에셋 테이블에서 아이콘을 찾아 로드한다. 행이 없으면 nullptr */
    UTexture2D* LoadIcon(int32 TemplateId) const;

    //~ Display
protected:
    /** 0보다 크면 PreConstruct에서 이 템플릿의 아이템 하나로 칸을 채운다. 상점의 진열 칸이 쓴다. */
    UPROPERTY(EditAnywhere, Category = "Slot")
    int32 DisplayTemplateId = 0;

    /** DisplayTemplateId가 0일 때 PreConstruct에서 아이콘으로 입힌다. 비어 있으면 SlotDefaultIcon을 쓴다. HUD의 물약 그림이 쓴다. */
    UPROPERTY(EditAnywhere, Category = "Slot")
    TObjectPtr<UTexture2D> DisplayIcon;

    //~ Cooldown
private:
    /** 칸이 지금 든 아이템의 템플릿 id다. 빈 칸이나 진열 칸이면 0이다. */
    int32 GetHeldTemplateId() const;

    void HandleItemCooldownStarted(int32 TemplateId);

    /** 든 아이템이 재사용 대기 중이면 막대를 갱신하고 타이머를 켠다. 아니면 막대를 0으로 두고 타이머를 끈다. */
    void RefreshCooldown();

    /** 남은 비율을 1에서 0으로 줄여 보인다. 블루프린트와 같은 간격이다. */
    static constexpr float CooldownBarIntervalSeconds = 0.05f;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> CooldownBar;

    FTimerHandle CooldownTimerHandle;

    //~ Tooltip
protected:
    /** 아이콘에 마우스를 올리면 아이콘의 툴팁 델리게이트가 부른다. 빈 칸이면 nullptr을 돌려 툴팁을 띄우지 않는다. */
    UFUNCTION()
    UWidget* GetToolTipWidget() const;

    /** 툴팁으로 띄울 위젯 클래스다. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Slot")
    TSubclassOf<UP1ItemTooltipWidget> TooltipClass;

    UPROPERTY()
    TObjectPtr<UP1ItemTooltipWidget> SlotTooltipWidget;
};
