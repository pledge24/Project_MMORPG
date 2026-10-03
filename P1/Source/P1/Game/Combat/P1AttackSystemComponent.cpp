#include "Game/Combat/P1AttackSystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Game/Combat/P1NormalAttackCombo.h"

void UP1AttackSystemComponent::BeginPlay()
{
	Super::BeginPlay();

    // 1. Owner가 유효한지 확인
    AActor* Owner = GetOwner();
    if (Owner)
    {
        USkeletalMeshComponent* SkeletalMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
        if (SkeletalMesh)
        {
            CharacterMesh = SkeletalMesh;
        }

    }

}

int32 UP1AttackSystemComponent::StartNormalAttack()
{
    if (CanStartNormalAttack() == false || NormalAttackMontages.Num() == 0)
        return 0;

    NormalAttackCombo = FP1NormalAttackCombo::Next(NormalAttackCombo, NormalAttackMontages.Num());
    UAnimMontage* Montage = NormalAttackMontages[FP1NormalAttackCombo::MontageIndexFor(NormalAttackCombo, NormalAttackMontages.Num())];

    if (UAnimInstance* AnimInstance = PlayMontage(Montage))
    {
        // 블루프린트의 몽타주 재생 노드처럼 이 공격의 몽타주에서 온 끝과 노티파이만 받는다.
        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(this, &UP1AttackSystemComponent::HandleMontageEnded);
        AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);

        const FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(Montage);
        AttackMontageInstanceId = MontageInstance ? MontageInstance->GetInstanceID() : INDEX_NONE;
        AnimInstance->OnPlayMontageNotifyBegin.AddUniqueDynamic(this, &UP1AttackSystemComponent::HandleMontageNotifyBegin);
    }

    // 재생에 실패해도 블루프린트처럼 공격 상태로 들어가고, 초기화 타이머가 풀어 준다.
    bIsAttacking = true;
    bEnableInputAttack = false;

    // 같은 핸들로 다시 걸면 남은 시간을 버리고 처음부터 센다. 블루프린트의 RetriggerableDelay와 같다.
    if (UWorld* World = GetWorld())
        World->GetTimerManager().SetTimer(ComboResetTimerHandle, this, &UP1AttackSystemComponent::ResetCombo, ComboResetSeconds, false);

    return NormalAttackCombo;
}

void UP1AttackSystemComponent::PlayNotifiedNormalAttack(int32 Combo)
{
    // 블루프린트는 소유자를 내 플레이어로 캐스트해 걸렀다. 로컬 플레이어 컨트롤러가 조종하는 폰은 내 플레이어뿐이다.
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn && OwnerPawn->IsPlayerControlled() && OwnerPawn->IsLocallyControlled())
        return;

    const int32 Index = FP1NormalAttackCombo::MontageIndexFor(Combo, NormalAttackMontages.Num());
    if (Index == INDEX_NONE)
        return;

    PlayMontage(NormalAttackMontages[Index]);
}

UAnimInstance* UP1AttackSystemComponent::PlayMontage(UAnimMontage* Montage)
{
    UAnimInstance* AnimInstance = CharacterMesh ? CharacterMesh->GetAnimInstance() : nullptr;
    if (AnimInstance == nullptr || Montage == nullptr)
        return nullptr;

    const float Length = AnimInstance->Montage_Play(Montage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, true);
    return Length > 0.f ? AnimInstance : nullptr;
}

void UP1AttackSystemComponent::ResetCombo()
{
    NormalAttackCombo = 0;
    bIsAttacking = false;
    bEnableInputAttack = true;
}

void UP1AttackSystemComponent::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 다음 타격이 앞 몽타주를 끊으면 중단으로 끝난다. 블루프린트도 중단에는 연결이 없었다.
    if (bInterrupted)
        return;

    bIsAttacking = false;
    bEnableInputAttack = true;
}

void UP1AttackSystemComponent::HandleMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
    // 노티파이 이름은 보지 않는다. 블루프린트도 이름과 무관하게 다음 입력을 열었다.
    if (Payload.MontageInstanceID == AttackMontageInstanceId)
        bEnableInputAttack = true;
}

void UP1AttackSystemComponent::InRestrictedArea()
{
    RestrictedArea++;
}

void UP1AttackSystemComponent::OutRestrictedArea()
{
    RestrictedArea = FMath::Max(RestrictedArea - 1, 0);
}

bool UP1AttackSystemComponent::IsAttacking() const
{
    return bIsAttacking;
}

bool UP1AttackSystemComponent::CanStartNormalAttack() const
{
    return bEnableInputAttack && RestrictedArea == 0;
}
