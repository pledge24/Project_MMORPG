// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/FieldBoundaryWall.h"
#include "Components/BoxComponent.h"

AFieldBoundaryWall::AFieldBoundaryWall()
{
	PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    TopWall = CreateDefaultSubobject<UBoxComponent>(TEXT("TopWall"));
    TopWall->SetupAttachment(Root);

    BottomWall = CreateDefaultSubobject<UBoxComponent>(TEXT("BottomWall"));
    BottomWall->SetupAttachment(Root);

    LeftWall = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftWall"));
    LeftWall->SetupAttachment(Root);

    RightWall = CreateDefaultSubobject<UBoxComponent>(TEXT("RightWall"));
    RightWall->SetupAttachment(Root);

    auto SetupWall = [](UBoxComponent* Wall)
        {
            Wall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Wall->SetCollisionResponseToAllChannels(ECR_Block);
            Wall->SetHiddenInGame(true);
        };

    SetupWall(TopWall);
    SetupWall(BottomWall);
    SetupWall(LeftWall);
    SetupWall(RightWall);
}

void AFieldBoundaryWall::UpdateWalls()
{
    float WidthHalfExtent = BoundaryWidth * 0.5f;
    float HeightHalfExtent = BoundaryHeight * 0.5f;

    // 상단
    TopWall->SetBoxExtent(FVector(HeightHalfExtent, WallThickness, WallHeight));
    TopWall->SetRelativeLocation(FVector(0, HeightHalfExtent + WallThickness, WallHeight));

    // 하단
    BottomWall->SetBoxExtent(FVector(HeightHalfExtent, WallThickness, WallHeight));
    BottomWall->SetRelativeLocation(FVector(0, -HeightHalfExtent - WallThickness, WallHeight));

    // 좌측
    LeftWall->SetBoxExtent(FVector(WallThickness, WidthHalfExtent, WallHeight));
    LeftWall->SetRelativeLocation(FVector(-WidthHalfExtent - WallThickness, 0, WallHeight));

    // 우측
    RightWall->SetBoxExtent(FVector(WallThickness, WidthHalfExtent, WallHeight));
    RightWall->SetRelativeLocation(FVector(WidthHalfExtent + WallThickness, 0, WallHeight));
}

void AFieldBoundaryWall::BeginPlay()
{
	Super::BeginPlay();
}

void AFieldBoundaryWall::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    UpdateWalls();
}
