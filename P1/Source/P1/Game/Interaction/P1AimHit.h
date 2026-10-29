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
