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

// Called when the game starts
void UAttackSystemComponent::BeginPlay()
{
	Super::BeginPlay();

    // 1. Owner가 유효한지 확인
    AActor* Owner = GetOwner();
    if (Owner)
    {
        // 2. Owner 액터에서 USkeletalMeshComponent 타입을 찾습니다.
        // TSubclassOf<USkeletalMeshComponent>()는 USkeletalMeshComponent 클래스 자체를 의미합니다.
        USkeletalMeshComponent* SkeletalMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();

        if (SkeletalMesh)
        {
            // 스켈레탈 메쉬 컴포넌트를 성공적으로 가져왔을 때의 로직
            UE_LOG(LogTemp, Warning, TEXT("Skeletal Mesh Found: %s"), *SkeletalMesh->GetName());
            Mesh = SkeletalMesh;
        }

    }
	
}

bool UAttackSystemComponent::IsAttacking() const
{
    return bIsAttacking;
}

