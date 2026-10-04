#pragma once

#include "CoreMinimal.h"
#include "UI/P1UserWidget.h"
#include "Protocol.pb.h"
#include "P1InventoryWidget.generated.h"

class UButton;
class UP1SlotWidget;
class UUniformGridPanel;
class UTextBlock;
class UWidgetSwitcher;

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

    void UpdateSlotWidget(const Protocol::Slot& InSlot);

    /** 맞는 슬롯이 없으면 nullptr을 돌려준다. */
    UP1SlotWidget* GetSlotWidgetFromSlot(const Protocol::Slot& InSlot);

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

    /** 0은 장비, 1은 소모품, 2는 기타 탭이다. */
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
    /** 응답을 기다리는 동안 참이다. 판매·사용·착용이 함께 쓴다. */
    UPROPERTY(VisibleAnywhere)
    bool bPendingPacket = false;

private:
    void BindSlotClicks(UUniformGridPanel* SlotGrid);

    /** 상점이 열려 있을 때만 판다. */
    void HandleSlotRightClicked(UP1SlotWidget* SlotWidget);

    /** 소모품은 사용하고 장비는 착용한다. */
    void HandleSlotDoubleClicked(UP1SlotWidget* SlotWidget);

    /** 응답을 기다리는 중이면 보내지 않는다. 보낼 때만 응답 대기를 켠다. */
    template <typename TPacket>
    void SendItemRequest(TPacket& Pkt);
};
