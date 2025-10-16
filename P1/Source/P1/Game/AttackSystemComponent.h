// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackSystemComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class P1_API UAttackSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAttackSystemComponent();

    UFUNCTION(BlueprintImplementableEvent, Category = "AttackSystem")
    void PerformNormalAttack();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable, Category = "AttackSystem")
    bool IsAttacking() const;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    bool bIsAttacking = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    int32 NormalAttackCombo = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    class USkeletalMeshComponent* Mesh;
};
