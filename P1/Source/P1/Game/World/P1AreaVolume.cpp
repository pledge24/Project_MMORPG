#include "Game/World/P1AreaVolume.h"
#include "EngineUtils.h"
#include "Game/World/P1FieldBoundaryWall.h"

#if WITH_EDITOR
void AP1AreaVolume::PostEditMove(bool bFinished)
{
    Super::PostEditMove(bFinished);

    if (bFinished)
        RefitBoundaryWalls();
}

void AP1AreaVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    RefitBoundaryWalls();
}
#endif

void AP1AreaVolume::RefitBoundaryWalls()
{
#if WITH_EDITOR
    // 볼륨을 옮기거나 키워도 벽의 생성 스크립트는 다시 돌지 않는다. 벽이 옛 크기로 남으면 클라이언트가 서버와 다른 곳에서 막힌다.
    UWorld* World = GetWorld();
    if (World == nullptr)
        return;

    for (TActorIterator<AP1FieldBoundaryWall> It(World); It; ++It)
    {
        if (It->GetAreaVolume() == this)
            It->RerunConstructionScripts();
    }
#endif
}
