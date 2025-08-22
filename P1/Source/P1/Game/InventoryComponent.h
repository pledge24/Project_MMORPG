// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class P1_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInventoryComponent();

    void Init();

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void AddItem(const FItemData& NewItem);

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    void RemoveItem(const FItemData& ItemToRemove);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    //UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    //TArray<FItemData> Items; // 아이템 목록
};
