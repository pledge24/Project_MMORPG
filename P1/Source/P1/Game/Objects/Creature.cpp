// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Objects/Creature.h"
#include "InGamePlayerController.h"

// Sets default values
ACreature::ACreature()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACreature::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (AInGamePlayerController* InGamePC = Cast<AInGamePlayerController>(PC))
    {
        InGamePC->AttachNameplate(this);
    }
	
}

void ACreature::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (AInGamePlayerController* InGamePC = Cast<AInGamePlayerController>(PC))
    {
        InGamePC->DetachNameplate(this);
    }
}

// Called every frame
void ACreature::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ACreature::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

