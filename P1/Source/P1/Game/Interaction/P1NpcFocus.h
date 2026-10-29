#pragma once

#include "CoreMinimal.h"

/** 조준 스윕이 맞힌 것 하나. */
struct FP1AimHit
{
    /** 스윕 시작점에서 맞힌 곳까지의 거리(cm). */
    float Distance = 0.f;

    /** 맞힌 액터의 위치. NPC면 캡슐 중심이다. */
    FVector ActorLocation = FVector::ZeroVector;

    /** 맞힌 것이 상호작용할 수 있는 NPC인지. 벽, 바닥, 몬스터는 거짓이다. */
    bool bNpc = false;
};

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
