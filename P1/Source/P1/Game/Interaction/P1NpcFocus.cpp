#include "Game/Interaction/P1NpcFocus.h"

int32 P1NpcFocus::PickFocus(TConstArrayView<FP1AimHit> Hits, const FVector& PlayerLocation, float Range, bool bBattleMode)
{
    // 전투 모드의 왼쪽 클릭은 공격이다. 상호작용 표시를 띄우지 않는다.
    if (bBattleMode)
        return INDEX_NONE;

    int32 Front = INDEX_NONE;
    for (int32 Index = 0; Index < Hits.Num(); ++Index)
    {
        if (Front == INDEX_NONE || Hits[Index].Distance < Hits[Front].Distance)
            Front = Index;
    }

    // 플레이어가 보는 것은 가장 앞의 것이다. 그것이 NPC가 아니거나 멀면 뒤의 NPC로 넘어가지 않는다.
    if (Front == INDEX_NONE || Hits[Front].bNpc == false)
        return INDEX_NONE;

    // 서버도 높이를 빼고 평면 거리로 거래를 판정한다.
    if (FVector::DistSquared2D(PlayerLocation, Hits[Front].ActorLocation) > FMath::Square(Range))
        return INDEX_NONE;

    return Front;
}
