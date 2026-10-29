#include "Game/World/P1PlacementVolume.h"
#include "Components/BoxComponent.h"

AP1PlacementVolume::AP1PlacementVolume()
{
    Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
    Bounds->SetupAttachment(RootComponent);
    Bounds->SetBoxExtent(FVector(500.f, 500.f, 200.f));
    Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Bounds->SetHiddenInGame(true);
}

void AP1PlacementVolume::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // 상자를 높이의 절반만큼 올려 액터의 위치가 바닥에 오게 한다. 배치하는 사람은 바닥에 액터를 놓으면 된다.
    Bounds->SetRelativeLocation(FVector(0.f, 0.f, Bounds->GetUnscaledBoxExtent().Z));
}

FVector2D AP1PlacementVolume::GetHalfExtent() const
{
    const FVector Extent = Bounds->GetUnscaledBoxExtent() * GetActorScale3D();
    return FVector2D(Extent.X, Extent.Y);
}
