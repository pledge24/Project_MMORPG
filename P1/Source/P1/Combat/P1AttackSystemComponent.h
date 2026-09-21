#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "P1AttackSystemComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class P1_API UP1AttackSystemComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UP1AttackSystemComponent();

    //~ Begin UActorComponent Interface
protected:
    virtual void BeginPlay() override;
    //~ End UActorComponent Interface

    //~ Normal Attack
public:
    /** 내 입력으로 시작하는 공격이다. 구현은 블루프린트에 있다. */
    UFUNCTION(BlueprintImplementableEvent, Category = "AttackSystem")
    void PerformNormalAttack();

    /** 서버 통지로 재생하는 공격이다. 구현은 블루프린트에 있다. */
    UFUNCTION(BlueprintImplementableEvent, Category = "AttackSystem")
    void S_PerformNormalAttack(int32 Combo);

    bool IsAttacking() const;
    bool EnableInputAttack() const;
    int32 GetLastCombo() const { return NormalAttackCombo; }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    TObjectPtr<class USkeletalMeshComponent> CharacterMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    bool bIsAttacking = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    bool bEnableInputAttack = true;

    /** 지금까지 쌓인 콤보 수다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    int32 NormalAttackCombo = 0;

    //~ Restricted Area
public:
    UFUNCTION(BlueprintCallable, Category = "AttackSystem")
    void InRestrictedArea();

    UFUNCTION(BlueprintCallable, Category = "AttackSystem")
    void OutRestrictedArea();

protected:
    /** 겹쳐 있는 제한 구역의 수다. 0보다 크면 공격 입력을 막는다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    int32 RestrictedArea = 0;
};
