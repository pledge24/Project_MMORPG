#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Game/Combat/P1ActionCombo.h"
#include "P1ActionComponent.generated.h"

class UAnimInstance;
class UAnimMontage;
class USkeletalMeshComponent;
struct FBranchingPointNotifyPayload;

/**
 * 크리처의 액션 재생.
 *
 * 액션 에셋 표로 액션 번호의 몽타주 섹션을 찾아 재생한다. 내 플레이어의 입력으로 콤보를 이어 가고 서버에 액션 시작을 알리며,
 * 서버가 알린 원격 크리처의 액션을 재생한다. 재생한 액션의 타격 시점마다 공격 범위 디버그 표시를 부른다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class P1_API UP1ActionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UP1ActionComponent();

    //~ My Player Combo
public:
    /**
     * 내 입력으로 일반 공격을 누른다. 진행 중인 액션이 없으면 FirstActionId(직업의 콤보 첫 단계)를 재생하고 서버에 알린다.
     * 진행 중이면 콤보 입력 창 안의 입력만 다음 단계로 예약한다. 다음 단계는 입력 창이 닫힐 때 재생하고 알린다.
     */
    void InputNormalAttack(int32 FirstActionId);

    /** 내 콤보의 액션이 재생 중이면 true. 원격 크리처의 액션은 세지 않는다. */
    bool IsActing() const { return Combo.IsActing(); }
    bool CanInputAction() const { return RestrictedArea == 0; }

private:
    /** 내 액션을 재생하고 서버에 알린다. 재생하지 못하면 알리지 않고 false. */
    bool StartLocalAction(int32 ActionId);

    UFUNCTION()
    void HandleMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload);
    UFUNCTION()
    void HandleMontageNotifyEnd(FName NotifyName, const FBranchingPointNotifyPayload& Payload);
    void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    FP1ActionCombo Combo;

    //~ Remote Action
public:
    /** 서버가 알린 액션을 재생만 하고 콤보는 바꾸지 않는다. 내 플레이어면 무시한다. */
    void PlayRemoteAction(int32 ActionId);

    //~ Attack Hit
public:
    /** 재생 중인 액션의 타격 노티파이가 부른다. 디버그 표시가 켜져 있으면 그 타격의 공격 범위를 그린다. */
    void HandleAttackHitNotify();

    //~ Restricted Area
public:
    UFUNCTION(BlueprintCallable, Category = "Action")
    void InRestrictedArea();

    UFUNCTION(BlueprintCallable, Category = "Action")
    void OutRestrictedArea();

protected:
    /** 겹쳐 있는 제한 구역의 수. 0보다 크면 공격 입력을 막는다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
    int32 RestrictedArea = 0;

    //~ 공통
private:
    /**
     * 액션 에셋 표에서 액션의 몽타주 섹션을 찾아 재생한다. 같은 몽타주가 재생 중이면 블렌드 없이 그 섹션으로 넘어간다.
     * 행이나 몽타주가 없거나 재생하지 못하면 false.
     */
    bool PlayActionSection(int32 ActionId);
    UAnimInstance* GetAnimInstance() const;

    /** 지금 재생하는 액션. 타격 순번을 세는 기준이다. 없으면 0. */
    int32 PlayingActionId = 0;
    /** 지금 섹션에서 다음에 올 타격의 순번. 섹션을 시작할 때 0으로 돌린다. */
    int32 NextHitIndex = 0;
    /** 액션을 재생한 몽타주 인스턴스 번호. 다른 몽타주에서 온 노티파이를 거른다. */
    int32 ActionMontageInstanceId = INDEX_NONE;
};
