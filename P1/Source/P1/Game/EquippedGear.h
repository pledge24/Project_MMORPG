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

using namespace google::protobuf;

/**
 *
 */
UCLASS()
class P1_API UEquippedGear : public UObject
{
    GENERATED_BODY()

public:
    UEquippedGear() = default;
    void Init(Map<int32, Protocol::Slot>* EquippedGear_);

public:
    void Rep_SlotChanged(const Protocol::Slot& Slot_);
    const Map<int32, Protocol::Slot>& GetAllSlot() { return *EquippedGearLookup; }

private:
    /** 장착 아이템 LookUp */
    Map<int32, Protocol::Slot>* EquippedGearLookup;
};