// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Protocol.pb.h"
#include "InventoryWidget.generated.h"

class UUniformGridPanel;

/**
 * 
 */
UCLASS()
class P1_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void Clear();

    void UpdateSlot(const Protocol::Slot& _Slot);
    class USlotWidget* GetSlotWidgetFromSlot(const Protocol::Slot& _Slot);

protected:
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
    UUniformGridPanel* Gear_Inven;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UUniformGridPanel* Consumables_Inven;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UUniformGridPanel* Misc_Inven;
};
