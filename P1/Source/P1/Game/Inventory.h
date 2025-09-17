// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Protocol.pb.h"
#include "Structs/ItemData.h"
#include "Inventory.generated.h"

/**
 * 
 */
UCLASS()
class P1_API UInventory : public UObject
{
	GENERATED_BODY()
	
public:
    UInventory();
    ~UInventory();

    void Init(Protocol::Inventory* Inventory_);

    void SetSlot(const Protocol::Slot& Slot_);

private:
    /** 카테고리별 인벤토리 Lookup 매핑 정보*/
    TMap<Protocol::SlotType, TArray<Protocol::Slot*>> InventoryLookupMappings;
    AActor* _Owner;
};
