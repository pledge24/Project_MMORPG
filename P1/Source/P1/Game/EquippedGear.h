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
    UEquippedGear() = default;

    void Init(Protocol::PlayerInfo* PlayerInfo_);

    void Rep_SlotChanged(const Protocol::Slot& Slot_);

private:
    /** 장착 아이템 LookUp */
    google::protobuf::Map<int32, Protocol::Slot> EquippedGearLookup;
};