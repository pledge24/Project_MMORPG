// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackSystemComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class P1_API UAttackSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAttackSystemComponent();

    UFUNCTION(BlueprintImplementableEvent, Category = "AttackSystem")
    bool PerformNormalAttack();

    UFUNCTION(BlueprintCallable, Category = "AttackSystem")
    int32 GetNextCombo() const;

    UFUNCTION(BlueprintCallable, Category = "AttackSystem")
    bool IsAttacking() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

protected:
    UPROPERTY()
    bool bIsAttacking = false;

    UPROPERTY()
    int32 NextCombo = 0;
};
