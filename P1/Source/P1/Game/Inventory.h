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
    UInventory();
    ~UInventory();

    void Init(const Protocol::ObjectInfo& Info);

    void UpdateSlots(const google::protobuf::RepeatedPtrField<Protocol::Slot>& Slots);

private:
    /* 카테고리별 인벤토리(캐시용) */
    TArray<Protocol::Slot> _Gear;
    TArray<Protocol::Slot> _Consumables;
    TArray<Protocol::Slot> _Miscellaneous;
};
