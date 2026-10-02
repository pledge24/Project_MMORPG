#include "Game/Combat/P1AttackSystemComponent.h"

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

bool UP1AttackSystemComponent::EnableInputAttack() const
{
    return bEnableInputAttack && RestrictedArea == 0;
}