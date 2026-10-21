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

/**
 * 아이템 한 칸을 보여 주는 슬롯 위젯.
 *
 * 인벤토리, 장비, 상점, HUD에서 아이템의 아이콘, 수량, 재사용 대기와 툴팁을 보여 주고 칸의 클릭을 알린다.
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
    /** 아이템을 진열만 한다. 칸이 아이템을 든 것으로 보지 않으므로 재사용 대기를 보이지 않는다. */
    void SetSlot(const FP1ItemData& Item, int32 Count = 1);

    /** 칸의 지금 내용을 입힌다. 아이템이 없으면 칸을 비운다. */
    void SetSlot(const Protocol::SlotChange& _Slot);

    void ClearSlot();

    /** 슬롯 데이터와 아이콘만 입히고 수량 문구는 갱신하지 않는다. */
    void InsertData(const Protocol::SlotChange& _Slot);

    /** 아이템 아이콘을 찾는 에셋 테이블. 행 구조체는 FP1ItemAssetData. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "DataTable")
    TObjectPtr<UDataTable> ItemAssetTable;

    /** 칸에 보이는 아이템의 정의. SlotData의 템플릿이 바뀌면 함께 바뀐다. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Item")
    FP1ItemData ItemData;

    /** 이 위젯이 보이는 칸의 주소와 내용. 한 번도 채운 적 없는 위젯이면 비어 있다. */
    Protocol::SlotChange SlotData;

    //~ Slot Visual
protected:
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UImage> ItemIcon;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemCountText;

    /** 칸이 비었을 때 보이는 아이콘. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Slot")
    TObjectPtr<UTexture2D> SlotDefaultIcon;

private:
    /** 아이콘을 찾지 못하면 지금 아이콘을 그대로 둔다. */
    void ApplyIcon(int32 TemplateId);

    /** 에셋 테이블에서 아이콘을 동기 로드한다. 테이블이나 행이 없으면 nullptr. */
    UTexture2D* LoadIcon(int32 TemplateId) const;

    //~ Display
protected:
    /** 0보다 크면 이 템플릿의 아이템 하나를 진열한다. 상점의 진열 칸이 쓴다. */
    UPROPERTY(EditAnywhere, Category = "Slot")
    int32 DisplayTemplateId = 0;

    /** 진열할 아이템이 없을 때 입힐 아이콘. 비어 있으면 SlotDefaultIcon. HUD의 물약 그림이 쓴다. */
    UPROPERTY(EditAnywhere, Category = "Slot")
    TObjectPtr<UTexture2D> DisplayIcon;

    //~ Cooldown
private:
    /** 칸이 든 아이템의 템플릿 id. 빈 칸이나 진열 칸이면 0. */
    int32 GetHeldTemplateId() const;

    void HandleItemCooldownStarted(int32 TemplateId);

    /** 든 아이템의 재사용 대기를 막대에 그린다. 대기 중이면 끝날 때까지 타이머로 다시 불린다. */
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
