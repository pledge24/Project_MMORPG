// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/AttackSystemComponent.h"

// Sets default values for this component's properties
UAttackSystemComponent::UAttackSystemComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

int32 UAttackSystemComponent::GetNextCombo() const
{
    return NextCombo;
}

bool UAttackSystemComponent::IsAttacking() const
{
    return bIsAttacking;
}


// Called when the game starts
void UAttackSystemComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

