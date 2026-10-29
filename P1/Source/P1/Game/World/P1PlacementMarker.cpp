#include "Game/World/P1PlacementMarker.h"
#include "Components/ArrowComponent.h"

AP1PlacementMarker::AP1PlacementMarker()
{
    // 쿠킹에서 빠진다. PIE에는 남지만 보이지 않고 충돌하지 않는다.
    bIsEditorOnlyActor = true;
    PrimaryActorTick.bCanEverTick = false;
    SetCanBeDamaged(false);

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    Direction = CreateDefaultSubobject<UArrowComponent>(TEXT("Direction"));
    Direction->SetupAttachment(RootComponent);
}
