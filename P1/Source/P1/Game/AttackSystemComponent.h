// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttackSystemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class P1_API UAttackSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAttackSystemComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintImplementableEvent, Category = "AttackSystem")
    void M_PerformNormalAttack();

    UFUNCTION(BlueprintImplementableEvent, Category = "AttackSystem")
    void O_PerformNormalAttack(int32 Combo);

    bool IsAttacking() const;
    bool EnableInputAttack() const;
    int32 GetLastCombo() const { return NormalAttackCombo;}

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    class USkeletalMeshComponent* CharacterMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    bool bIsAttacking = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    bool bEnableInputAttack = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    int32 NormalAttackCombo = 0; // 쌓인 콤보 기준
};
