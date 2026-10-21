#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Game/Items/P1ContainerType.h"
#include "Game/Items/P1Slot.h"
#include "P1InventoryWidget.generated.h"

class UButton;
class UP1ItemContainer;
class UP1SlotWidget;
class UUniformGridPanel;
class UTextBlock;
class UWidgetSwitcher;

/**
 * 내 인벤토리 창.
 *
 * 가진 아이템을 종류별 탭으로 보여 주고, 슬롯 조작을 판매·사용·착용 요청으로 바꾼다.
 */
UCLASS()
class P1_API UP1InventoryWidget : public UP1UserWidget
{
    GENERATED_BODY()

    //~ Begin UUserWidget Interface
public:
    virtual void NativeConstruct() override;
    //~ End UUserWidget Interface

    //~ Slot Display
protected:
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void Clear();

    void UpdateSlotWidget(const FP1Slot& InSlot);

    /** 컨테이너의 격자를 그 컨테이너의 지금 칸으로 다시 그린다. */
    void RedrawContainer(const UP1ItemContainer& Container);

    /** 인벤토리 구역이 아니면 nullptr. */
    UUniformGridPanel* GetSlotGrid(EP1ContainerType Container) const;

    /** 맞는 슬롯 위젯이 없으면 nullptr. */
    UP1SlotWidget* GetSlotWidgetFromSlot(const FP1SlotAddress& Address) const;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UUniformGridPanel> Gear_Inven;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UUniformGridPanel> Consumables_Inven;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UUniformGridPanel> Misc_Inven;

    //~ Tabs
private:
    UFUNCTION()
    void ShowGearTab();

    UFUNCTION()
    void ShowConsumableTab();

    UFUNCTION()
    void ShowMiscTab();

    /** 0은 장비, 1은 소모품, 2는 기타 탭. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidgetSwitcher> InventoryTabSwitcher;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> GearTabButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ConsumableTabButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> MiscTabButton;

    //~ Gold
protected:
    void UpdateGold(const int64 Gold);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Gold_txt;

    //~ Item Request
protected:
    /** 판매·사용·착용 요청의 응답을 기다리는 동안 true. */
    UPROPERTY(VisibleAnywhere)
    bool bPendingPacket = false;

private:
    void BindSlotClicks(UUniformGridPanel* SlotGrid);

    /** 우클릭한 아이템을 하나 판다. 상점이 열려 있을 때만 판다. */
    void HandleSlotRightClicked(UP1SlotWidget* SlotWidget);

    /** 소모품은 사용하고 장비는 착용한다. */
    void HandleSlotDoubleClicked(UP1SlotWidget* SlotWidget);

    /** 응답을 기다리는 중이면 보내지 않는다. */
    template <typename TPacket>
    void SendItemRequest(TPacket& Pkt);
};
