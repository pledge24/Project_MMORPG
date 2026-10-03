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
        // 이 공격의 몽타주 인스턴스에서 온 끝과 노티파이만 받는다. 다음 타격이 끊은 앞 몽타주의 이벤트는 섞이지 않는다.
        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(this, &UP1AttackSystemComponent::HandleMontageEnded);
        AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);

        const FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(Montage);
        AttackMontageInstanceId = MontageInstance ? MontageInstance->GetInstanceID() : INDEX_NONE;
        AnimInstance->OnPlayMontageNotifyBegin.AddUniqueDynamic(this, &UP1AttackSystemComponent::HandleMontageNotifyBegin);
    }

    // 재생에 실패해도 공격 상태로 들어간다. 끝 이벤트가 오지 않으므로 초기화 타이머가 풀어 준다.
    bIsAttacking = true;
    bEnableInputAttack = false;

    // 같은 핸들로 다시 걸면 남은 시간을 버리고 처음부터 센다. 공격을 이어 가는 동안에는 초기화되지 않는다.
    if (UWorld* World = GetWorld())
        World->GetTimerManager().SetTimer(ComboResetTimerHandle, this, &UP1AttackSystemComponent::ResetCombo, ComboResetSeconds, false);

    return NormalAttackCombo;
}

void UP1AttackSystemComponent::PlayRemoteNormalAttack(int32 Combo)
{
    // 내 플레이어는 입력에서 이미 재생했다. 로컬 플레이어 컨트롤러가 조종하는 폰은 내 플레이어뿐이다.
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
    // 다음 타격이 앞 몽타주를 끊으면 중단으로 끝난다. 이때는 새 공격이 상태를 쥐고 있으므로 건드리지 않는다.
    if (bInterrupted)
        return;

    bIsAttacking = false;
    bEnableInputAttack = true;
}

void UP1AttackSystemComponent::HandleMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
    // 노티파이 이름은 보지 않는다. 공격 몽타주의 어떤 노티파이든 다음 입력을 연다.
    if (AttackMontageInstanceId != INDEX_NONE && Payload.MontageInstanceID == AttackMontageInstanceId)
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
