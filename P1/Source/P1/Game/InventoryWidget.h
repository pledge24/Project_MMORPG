// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Protocol.pb.h"
#include "InventoryWidget.generated.h"

class UUniformGridPanel;
class UTextBlock;

/**
 * 
 */
UCLASS()
class P1_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    virtual void NativeConstruct() override;

    void SetupDelegateBinding();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void Clear();

    void UpdateSlot(const Protocol::Slot& _Slot);
    void UpdateGold(int32 Gold);
    class USlotWidget* GetSlotWidgetFromSlot(const Protocol::Slot& _Slot);

protected:
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendSellPacket(USlotWidget* _Slot);

    UFUNCTION(BlueprintCallable, Category = "Network")
    void SendEquipPacket(USlotWidget* _Slot);

    UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
    UUniformGridPanel* Gear_Inven;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UUniformGridPanel* Consumables_Inven;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UUniformGridPanel* Misc_Inven;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* Gold_txt;
};
