#include "Game/World/P1FieldBoundaryWall.h"
#include "Components/BoxComponent.h"

AP1FieldBoundaryWall::AP1FieldBoundaryWall()
{
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

void AP1FieldBoundaryWall::UpdateWalls()
{
    // 언리얼 좌표를 따른다. depth가 x 방향, width가 y 방향이다.
    float DepthHalfExtent = BoundaryDepth * 0.5f;
    float WidthHalfExtent = BoundaryWidth * 0.5f;

    // 상단과 하단은 y 방향 끝에 서서 x 방향으로 필드의 depth만큼 막는다.
    TopWall->SetBoxExtent(FVector(DepthHalfExtent, WallThickness, WallHeight));
    TopWall->SetRelativeLocation(FVector(0, WidthHalfExtent + WallThickness, WallHeight));

    BottomWall->SetBoxExtent(FVector(DepthHalfExtent, WallThickness, WallHeight));
    BottomWall->SetRelativeLocation(FVector(0, -WidthHalfExtent - WallThickness, WallHeight));

    // 좌측과 우측은 x 방향 끝에 서서 y 방향으로 필드의 width만큼 막는다.
    LeftWall->SetBoxExtent(FVector(WallThickness, WidthHalfExtent, WallHeight));
    LeftWall->SetRelativeLocation(FVector(-DepthHalfExtent - WallThickness, 0, WallHeight));

    RightWall->SetBoxExtent(FVector(WallThickness, WidthHalfExtent, WallHeight));
    RightWall->SetRelativeLocation(FVector(DepthHalfExtent + WallThickness, 0, WallHeight));
}

void AP1FieldBoundaryWall::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    UpdateWalls();
}
