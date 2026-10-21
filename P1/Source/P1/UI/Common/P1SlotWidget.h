#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "UI/Common/P1SlotDisplay.h"
#include "UI/Common/P1SlotDragPayload.h"
#include "P1SlotWidget.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;
class UTexture2D;
class UP1ItemTooltipWidget;
class UP1SlotWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSlotClicked, UP1SlotWidget*);
DECLARE_DELEGATE_RetVal_OneParam(TOptional<FP1SlotDragPayload>, FOnSlotDragRequested, const UP1SlotWidget*);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnSlotDropped, UP1SlotWidget*, const FP1SlotDragPayload&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSlotDragCancelled, UP1SlotWidget*, const FP1SlotDragPayload&);

/**
 * 아이템 한 칸을 보여 주는 슬롯 위젯.
 *
 * 받은 표시 내용대로 아이템의 아이콘, 수량, 재사용 대기와 툴팁을 보여 주고 칸의 클릭과 끌어 놓기를 알린다.
 * 어느 칸을 그리는지, 클릭과 끌어 놓기가 무슨 뜻인지는 품는 위젯이 정한다.
 */
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
    virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    //~ End UUserWidget Interface

    //~ Click
public:
    /** 아이콘 위에서 우클릭하면 알린다. 빈 칸에서도 알린다. */
    FOnSlotClicked OnRightClicked;

    /** 아이콘 위에서 좌클릭으로 더블클릭하면 알린다. 빈 칸에서도 알린다. */
    FOnSlotClicked OnDoubleClicked;

private:
    bool IsOverIcon(const FPointerEvent& InMouseEvent) const;

    //~ Drag and Drop
public:
    /** 묶여 있으면 아이템이 든 아이콘을 좌클릭으로 끌 수 있다. 끌 내용을 돌려주고, 비워 돌려주면 끌지 않는다. */
    FOnSlotDragRequested OnDragRequested;

    /** 이 슬롯에 놓으면 부른다. 받아들였으면 true. 묶이지 않았거나 false면 바깥에 놓은 것과 같다. */
    FOnSlotDropped OnDropped;

    /** 이 슬롯에서 시작한 끌기를 받아들인 곳 없이 놓으면 알린다. */
    FOnSlotDragCancelled OnDragCancelled;

    //~ Display
public:
    /** 그릴 내용을 바꾸고 다시 그린다. 빈 내용이면 빈 칸 그림을 보인다. */
    void SetDisplay(const FP1SlotDisplay& InDisplay);

    const FP1SlotDisplay& GetDisplay() const { return Display; }

protected:
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UImage> ItemIcon;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemCountText;

    /** 칸이 비었을 때 보이는 아이콘. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Slot")
    TObjectPtr<UTexture2D> SlotDefaultIcon;

    /** 0보다 크면 이 템플릿의 아이템 하나를 진열한다. 상점의 진열 칸이 쓴다. */
    UPROPERTY(EditAnywhere, Category = "Slot")
    int32 DisplayTemplateId = 0;

    /** 칸이 비었을 때 SlotDefaultIcon 대신 보일 아이콘. HUD의 물약 그림이 쓴다. */
    UPROPERTY(EditAnywhere, Category = "Slot")
    TObjectPtr<UTexture2D> DisplayIcon;

    /** 어둡게 그리는 아이템 아이콘의 색. */
    UPROPERTY(EditAnywhere, Category = "Slot")
    FLinearColor DimmedIconColor = FLinearColor(0.25f, 0.25f, 0.25f, 1.f);

private:
    FP1SlotDisplay Display;

    /** 어둡게 그리기 전 아이콘의 색. WBP가 정한 색으로 되돌리려고 처음 그릴 때 한 번 읽는다. */
    TOptional<FLinearColor> NormalIconColor;

    //~ Cooldown
private:
    void HandleItemCooldownStarted(int32 TemplateId);

    /** 재사용 대기를 막대에 그린다. 대기 중이면 끝날 때까지 타이머로 다시 불린다. */
    void RefreshCooldown();

    static constexpr float CooldownBarIntervalSeconds = 0.05f;

    /** 재사용 대기의 남은 비율을 1에서 0으로 줄여 보인다. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> CooldownBar;

    FTimerHandle CooldownTimerHandle;

    //~ Tooltip
protected:
    /** 아이콘의 툴팁 델리게이트가 부른다. 빈 칸이면 nullptr을 돌려 툴팁을 띄우지 않는다. */
    UFUNCTION()
    UWidget* GetToolTipWidget() const;

    /** 툴팁으로 띄울 위젯 클래스. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Slot")
    TSubclassOf<UP1ItemTooltipWidget> TooltipClass;

    UPROPERTY()
    TObjectPtr<UP1ItemTooltipWidget> SlotTooltipWidget;
};
