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
    UInventory() = default;
    UInventory(AActor* Owner_);
    ~UInventory();

    void Init(const Protocol::Inventory& Inventory_);

    void AddItem(const Protocol::Slot& Slot_);
    void RemoveItem(const Protocol::Slot& Slot_);

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnInventoryUpdated, const Protocol::Slot&);
    FOnInventoryUpdated OnInventoryUpdated;

private:
    /** 카테고리별 인벤토리 Lookup 매핑 정보*/
    TMap<Protocol::SlotType, TArray<Protocol::Item*>&> InventoryLookupMappings;

    /** 카테고리별 인벤토리 LookUp */
    TArray<Protocol::Item*> GearLookup;
    TArray<Protocol::Item*> ConsumablesLookup;
    TArray<Protocol::Item*> MiscellaneousLookup;

    AActor* Owner;
};
