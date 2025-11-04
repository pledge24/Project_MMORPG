// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Protocol.pb.h"
#include "EquippedGearComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class P1_API UEquippedGearComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UEquippedGearComponent();

protected:
	virtual void BeginPlay() override;

public:
    void Init(Protocol::PlayerInfo* InPlayerInfo);
    void HandleSlotChanged(const Protocol::Slot& InSlot);

protected:
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnSlotChanged, const Protocol::Slot&);
    FOnSlotChanged OnSlotChanged;

private:
    /** 장착 아이템 LookUp */
    google::protobuf::Map<int32, Protocol::Slot>* EquippedGearLookup;
};
