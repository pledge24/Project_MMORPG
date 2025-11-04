// Fill out your copyright notice in the Description page of Project Settings.


#include "AttackSystemComponent.h"

// Sets default values for this component's properties
UAttackSystemComponent::UAttackSystemComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

// Called when the game starts
void UAttackSystemComponent::BeginPlay()
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

bool UAttackSystemComponent::IsAttacking() const
{
    return bIsAttacking;
}

bool UAttackSystemComponent::EnableInputAttack() const
{
    return bEnableInputAttack;
}