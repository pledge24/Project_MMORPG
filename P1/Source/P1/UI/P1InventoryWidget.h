#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Protocol.pb.h"
#include "P1InventoryWidget.generated.h"

class AP1MyPlayer;
class UUniformGridPanel;
class UTextBlock;

UCLASS()
class P1_API UP1InventoryWidget : public UUserWidget
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

    void UpdateSlotWidget(const Protocol::Slot& InSlot, bool OnUse = false);

    /** 맞는 슬롯이 없으면 nullptr을 돌려준다. */
    class UP1SlotWidget* GetSlotWidgetFromSlot(const Protocol::Slot& InSlot);

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UUniformGridPanel> Gear_Inven;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UUniformGridPanel> Consumables_Inven;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UUniformGridPanel> Misc_Inven;

    //~ Gold
protected:
    void UpdateGold(const int64 Gold);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Gold_txt;

    //~ Item Request
protected:
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendSellItemPacket(UP1SlotWidget* SlotWidget);

    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendUseItemPacket(UP1SlotWidget* SlotWidget);

    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendEquipItemPacket(UP1SlotWidget* SlotWidget);

    /** 응답을 기다리는 동안 참이다. 중복 요청을 막는다. */
    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    bool PendingPacket = false;
};
