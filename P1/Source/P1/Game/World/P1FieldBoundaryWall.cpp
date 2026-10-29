#include "Game/World/P1FieldBoundaryWall.h"
#include "Components/BoxComponent.h"
#include "Game/World/P1AreaVolume.h"

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

void AP1FieldBoundaryWall::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    FitToAreaVolume();
}

void AP1FieldBoundaryWall::FitToAreaVolume()
{
#if WITH_EDITOR
    // 볼륨이 없으면 저장된 벽을 그대로 둔다. 쿠킹한 빌드에는 볼륨이 없으므로 에디터에서 맞춘 벽이 그대로 쓰인다.
    if (AreaVolume == nullptr)
        return;

    FittedCenter = AreaVolume->GetActorLocation();
    FittedHalfExtent = AreaVolume->GetHalfExtent();

    // 언리얼 좌표를 따른다. X 방향 반경이 depth, Y 방향 반경이 width다.
    const double DepthHalfExtent = FittedHalfExtent.X;
    const double WidthHalfExtent = FittedHalfExtent.Y;
    const FVector Base = FittedCenter + FVector(0.0, 0.0, WallHeight);

    // 벽은 바닥에서 서고 지역 볼륨 바깥에 붙는다. 액터가 다른 액터에 붙어 크기가 바뀌어 있어도 월드 기준으로 맞춘다.
    auto PlaceWall = [](UBoxComponent* Wall, const FVector& Location, const FVector& Extent)
        {
            Wall->SetWorldRotation(FRotator::ZeroRotator);
            Wall->SetWorldScale3D(FVector::OneVector);
            Wall->SetWorldLocation(Location);
            Wall->SetBoxExtent(Extent);
        };

    // 상단과 하단은 y 방향 끝에 서서 x 방향으로 지역의 depth만큼 막는다.
    PlaceWall(TopWall, Base + FVector(0.0, WidthHalfExtent + WallThickness, 0.0), FVector(DepthHalfExtent, WallThickness, WallHeight));
    PlaceWall(BottomWall, Base + FVector(0.0, -WidthHalfExtent - WallThickness, 0.0), FVector(DepthHalfExtent, WallThickness, WallHeight));

    // 좌측과 우측은 x 방향 끝에 서서 y 방향으로 지역의 width만큼 막는다.
    PlaceWall(LeftWall, Base + FVector(-DepthHalfExtent - WallThickness, 0.0, 0.0), FVector(WallThickness, WidthHalfExtent, WallHeight));
    PlaceWall(RightWall, Base + FVector(DepthHalfExtent + WallThickness, 0.0, 0.0), FVector(WallThickness, WidthHalfExtent, WallHeight));
#endif
}
