#pragma once

#include "CoreMinimal.h"
#include "Game/Interaction/P1AimHit.h"

/**
 * 화면 중앙 조준으로 초점을 맞출 NPC를 고르는 판정.
 *
 * 월드를 모르는 판정만 둔다. 스윕을 쏘고 결과를 액터에 반영하는 일은 상호작용 컴포넌트가 한다.
 */
namespace P1NpcFocus
{
    /**
     * 초점을 맞출 맞힘의 순번. 맞힌 것 가운데 가장 앞의 것이 사거리 안의 NPC일 때만 그것이고, 아니면 INDEX_NONE.
     * 사거리는 플레이어와 NPC의 평면 거리로 잰다. 전투 모드에서는 늘 INDEX_NONE.
     */
    P1_API int32 PickFocus(TConstArrayView<FP1AimHit> Hits, const FVector& PlayerLocation, float Range, bool bBattleMode);
}
