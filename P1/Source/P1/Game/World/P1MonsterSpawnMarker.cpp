#include "Game/World/P1MonsterSpawnMarker.h"
#include "Components/SphereComponent.h"
#include "Engine/CollisionProfile.h"

AP1MonsterSpawnMarker::AP1MonsterSpawnMarker()
{
    RadiusPreview = CreateDefaultSubobject<USphereComponent>(TEXT("RadiusPreview"));
    RadiusPreview->SetupAttachment(RootComponent);
    RadiusPreview->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
    RadiusPreview->SetHiddenInGame(true);
    RadiusPreview->SetSphereRadius(Radius);
}

void AP1MonsterSpawnMarker::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    // 액터의 크기를 반경에 곱하지 않는다. 서버가 받는 반경은 Radius 하나다.
    RadiusPreview->SetSphereRadius(Radius);
    RadiusPreview->SetWorldScale3D(FVector::OneVector);
}
