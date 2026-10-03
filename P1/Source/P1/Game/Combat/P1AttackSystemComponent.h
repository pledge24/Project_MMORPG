#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "P1AttackSystemComponent.generated.h"

class UAnimInstance;
class UAnimMontage;
class USkeletalMeshComponent;
struct FBranchingPointNotifyPayload;

/**
 * 크리처의 일반 공격 동작을 맡는다. 내 입력으로 시작하는 공격은 콤보 순번을 돌리고 입력 가능 여부를 관리한다.
 * 서버가 알린 다른 크리처의 공격은 순번에 맞는 몽타주만 재생한다. 몽타주 배열은 블루프린트 기본값에 있다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class P1_API UP1AttackSystemComponent : public UActorComponent
{
    GENERATED_BODY()

    //~ Begin UActorComponent Interface
protected:
    virtual void BeginPlay() override;
    //~ End UActorComponent Interface

    //~ Normal Attack
public:
    /**
     * 내 입력으로 일반 공격을 시작하고 새 콤보 순번(1부터)을 돌려준다.
     * 입력을 받을 수 없거나 몽타주가 없으면 시작하지 않고 0을 돌려준다.
     */
    int32 StartNormalAttack();

    /** 서버가 알린 일반 공격의 동작을 재생한다. 내 플레이어는 입력에서 이미 재생했으므로 무시한다. 상태는 바꾸지 않는다. */
    void PlayNotifiedNormalAttack(int32 Combo);

    bool IsAttacking() const;
    bool CanStartNormalAttack() const;

private:
    /** 몽타주를 재생한다. 다른 몽타주는 모두 멈춘다. 재생하지 못하면 nullptr. */
    UAnimInstance* PlayMontage(UAnimMontage* Montage);

    /** 마지막 공격 뒤 ComboResetSeconds 동안 다음 공격이 없으면 콤보를 처음으로 되돌린다. */
    void ResetCombo();

    void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    UFUNCTION()
    void HandleMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload);

    static constexpr float ComboResetSeconds = 2.f;

    /** 콤보 순번 1부터 차례로 재생하는 몽타주다. 서버가 알린 순번 0은 첫 몽타주로 재생한다. */
    UPROPERTY(EditDefaultsOnly, Category = "AttackSystem")
    TArray<TObjectPtr<UAnimMontage>> NormalAttackMontages;

    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> CharacterMesh;

    bool bIsAttacking = false;
    bool bEnableInputAttack = true;

    /** 지금 콤보 순번이다. 0은 콤보가 없는 상태다. */
    int32 NormalAttackCombo = 0;

    /** 내 입력으로 재생한 몽타주의 인스턴스다. 다른 몽타주에서 온 노티파이를 거른다. */
    int32 AttackMontageInstanceId = INDEX_NONE;

    FTimerHandle ComboResetTimerHandle;

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
