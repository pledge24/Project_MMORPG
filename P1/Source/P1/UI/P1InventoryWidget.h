#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Protocol.pb.h"
#include "P1InventoryWidget.generated.h"

class AP1MyPlayer;
class UUniformGridPanel;
class UTextBlock;

/**
 * 
 */
UCLASS()
class P1_API UP1InventoryWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    virtual void NativeConstruct() override;

protected:
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void Clear();

    void UpdateSlotWidget(const Protocol::Slot& InSlot, bool OnUse = false);
    void UpdateGold(const int64 Gold);
    class UP1SlotWidget* GetSlotWidgetFromSlot(const Protocol::Slot& InSlot);

protected:
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendSellItemPacket(UP1SlotWidget* SlotWidget);

    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendUseItemPacket(UP1SlotWidget* SlotWidget);

    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendEquipItemPacket(UP1SlotWidget* SlotWidget);

    UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
    TObjectPtr<UUniformGridPanel> Gear_Inven;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UUniformGridPanel> Consumables_Inven;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    TObjectPtr<UUniformGridPanel> Misc_Inven;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> Gold_txt;

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
    bool PendingPacket = false;
};
