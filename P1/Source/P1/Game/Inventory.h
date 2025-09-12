// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Protocol.pb.h"
#include "Structs/ItemData.h"
#include "Inventory.generated.h"

enum
{
    MAX_SLOTS = 50
};

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

    void UpdateSlots(const google::protobuf::RepeatedPtrField<Protocol::Slot>& Slots);
    void UpdateSlot(const Protocol::Slot& Slot_);

private:
    /* 카테고리별 인벤토리 LookUp */
    TArray<Protocol::Item*> GearLookup;
    TArray<Protocol::Item*> ConsumablesLookup;
    TArray<Protocol::Item*> MiscellaneousLookup;

    AActor* Owner;
};
