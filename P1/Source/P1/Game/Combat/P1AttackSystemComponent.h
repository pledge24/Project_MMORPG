#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "P1AttackSystemComponent.generated.h"

class UAnimInstance;
class UAnimMontage;
class USkeletalMeshComponent;
struct FBranchingPointNotifyPayload;

/**
 * 크리처의 일반 공격 동작.
 *
 * 내 플레이어의 공격 입력으로 콤보를 이어 가고, 서버가 알린 원격 크리처의 공격 동작을 재생한다.
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

    /** 서버가 알린 일반 공격 동작만 재생하고 공격 상태는 바꾸지 않는다. 내 플레이어면 무시한다. */
    void PlayRemoteNormalAttack(int32 Combo);

    bool IsAttacking() const;
    bool CanStartNormalAttack() const;

    /** 콤보 순번 1부터 차례로 재생할 몽타주를 바꾼다. 몬스터처럼 스폰할 때 외형을 입는 크리처가 쓴다. */
    void SetNormalAttackMontages(const TArray<UAnimMontage*>& Montages);

private:
    /** 몽타주를 재생하고 다른 몽타주는 모두 멈춘다. 재생하지 못하면 nullptr. */
    UAnimInstance* PlayMontage(UAnimMontage* Montage);

    void ResetCombo();

    void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    UFUNCTION()
    void HandleMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload);

    /** 마지막 공격 뒤 이 시간(초) 동안 다음 공격이 없으면 콤보를 처음으로 되돌린다. */
    static constexpr float ComboResetSeconds = 2.f;

    /** 콤보 순번 1부터 차례로 재생하는 몽타주. */
    UPROPERTY(EditDefaultsOnly, Category = "AttackSystem")
    TArray<TObjectPtr<UAnimMontage>> NormalAttackMontages;

    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> CharacterMesh;

    bool bIsAttacking = false;
    bool bEnableInputAttack = true;

    /** 지금 콤보 순번. 0이면 콤보가 없는 상태. */
    int32 NormalAttackCombo = 0;

    /** 내 입력으로 재생한 몽타주의 인스턴스 번호. 다른 몽타주에서 온 노티파이를 거른다. */
    int32 AttackMontageInstanceId = INDEX_NONE;

    FTimerHandle ComboResetTimerHandle;

    //~ Restricted Area
public:
    UFUNCTION(BlueprintCallable, Category = "AttackSystem")
    void InRestrictedArea();

    UFUNCTION(BlueprintCallable, Category = "AttackSystem")
    void OutRestrictedArea();

protected:
    /** 겹쳐 있는 제한 구역의 수. 0보다 크면 공격 입력을 막는다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttackSystem")
    int32 RestrictedArea = 0;
};
