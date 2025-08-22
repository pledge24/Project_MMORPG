// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Structs/ItemData.h"
#include "Protocol.pb.h"
#include "EquipmentComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class P1_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UEquipmentComponent();

    void Init(const Protocol::ObjectInfo& ObjectInfo);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void Equip(int32 SlotId);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void UnEquip(int32 SlotId);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    //UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
    //TArray<FItemData> Equipments; /*ItemId*/
};
