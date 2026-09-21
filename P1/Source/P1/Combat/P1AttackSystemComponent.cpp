#include "Combat/P1AttackSystemComponent.h"

// Sets default values for this component's properties
UP1AttackSystemComponent::UP1AttackSystemComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

// Called when the game starts
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
    RestrictedArea = FMath::Min(RestrictedArea - 1, 0);
}

bool UP1AttackSystemComponent::IsAttacking() const
{
    return bIsAttacking;
}

bool UP1AttackSystemComponent::EnableInputAttack() const
{
    return bEnableInputAttack && RestrictedArea == 0;
}