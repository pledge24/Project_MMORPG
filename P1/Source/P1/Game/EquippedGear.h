// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Protocol.pb.h"
#include "EquippedGear.generated.h"

enum
{
    MAX_EQUIPPED_SLOTS = 6
};

/**
 * 
 */
UCLASS()
class P1_API UEquippedGear : public UObject
{
	GENERATED_BODY()

public:
    UEquippedGear();
    ~UEquippedGear();

    void Init(Protocol::PlayerInfo* PlayerInfo_);

    void SetSlot(const Protocol::Slot& Slot_);

private:
    /** 장착 아이템 LookUp */
    TArray<Protocol::Slot*> EquippedGearLookup;

    AActor* _Owner;
};
