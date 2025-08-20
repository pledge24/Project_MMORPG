// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BP_Structs.h"
#include "Inventory.generated.h"

/**
 * 
 */
UCLASS()
class P1_API UInventory : public UObject
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable)
    void Init();

    UFUNCTION(BlueprintCallable)
    void Clear();

    UFUNCTION(BlueprintCallable)
    void AddItem(int32 ItemID);

    UFUNCTION(BlueprintCallable)
    void RemoveItem(int32 SlotID, int32 Quantity);

    
private:
    FItemData* FindItem(int32 ItemID);

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FItemData> Items;
};
