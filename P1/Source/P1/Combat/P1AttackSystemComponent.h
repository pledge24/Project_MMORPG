#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "P1AttackSystemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class P1_API UP1AttackSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UP1AttackSystemComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintImplementableEvent, Category = "AttackSystem")
    void PerformNormalAttack();

    UFUNCTION(BlueprintImplementableEvent, Category = "AttackSystem")
    void S_PerformNormalAttack(int32 Combo);

    UFUNCTION(BlueprintCallable, Category = "AttackSystem")
    void InRestrictedArea();

    UFUNCTION(BlueprintCallable, Category = "AttackSystem")

    void OutRestrictedArea();

    bool IsAttacking() const;
    bool EnableInputAttack() const;
    int32 GetLastCombo() const { return NormalAttackCombo;}

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    TObjectPtr<class USkeletalMeshComponent> CharacterMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    bool bIsAttacking = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    bool bEnableInputAttack = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    int32 NormalAttackCombo = 0; // 쌓인 콤보 기준

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    int32 RestrictedArea = 0; 
};
