// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Structs/ItemData.h"
#include "Structs/SlotData.h"
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
    void Init(TArray<FSlotData>& Items);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void Clear();

    UFUNCTION(BlueprintCallable, Category= "Inventory")
    void UpdateSingleSlot(FSlotData slotItem);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void UpdateMultipleSlots(TArray<FSlotData>& Items);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    USlotWidget* GetSlotFromSlotId(int32 SlotId);

protected:
    UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
    UUniformGridPanel* GridPanel;
};
