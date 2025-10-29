// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Monster.h"
#include "NameplateManager.h"
#include "InGamePlayerController.h"

AMonster::AMonster()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AMonster::BeginPlay()
{
	Super::BeginPlay();

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (AInGamePlayerController* InGamePC = Cast<AInGamePlayerController>(PC))
    {
        InGamePC->AttachNameplate(this);
    }
}

void AMonster::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (AInGamePlayerController* InGamePC = Cast<AInGamePlayerController>(PC))
    {
        InGamePC->DetachNameplate(this);
    }
}

void AMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMonster::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

