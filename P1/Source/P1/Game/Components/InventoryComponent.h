// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Protocol.pb.h"
#include "InventoryComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class P1_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInventoryComponent();

protected:
	virtual void BeginPlay() override;

public:	
    void Init(Protocol::Inventory* InInventory);
    void HandleSlotChanged(const Protocol::Slot& InSlot, bool OnUse = false);

protected:
    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSlotChanged, const Protocol::Slot&, bool);
    FOnSlotChanged OnSlotChanged;

private:
    /** 카테고리별 인벤토리 Lookup 매핑 정보*/
    TMap<Protocol::SlotType, TArray<Protocol::Slot*>> InventoryLookupMappings;
};
