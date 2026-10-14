#pragma once

#include "CoreMinimal.h"

/**
 * 원격 크리처의 이동 보정 결과.
 *
 * 화면에 보이는 원격 크리처가 서버의 위치와 방향에서 멀어지지 않도록, 매 틱 적용할 위치와 회전을 계산한다.
 */
struct P1_API FP1MoveCorrection
{
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;

    /** 보간 없이 서버 위치로 순간이동했으면 true. */
    bool bSnapped = false;

    /**
     * 한 틱 분량의 보정을 계산한다. 높이는 보정하지 않으므로 결과의 Z는 ClientLocation의 Z다.
     * bInAction이면 루트 모션에 맡기고, 순간이동할 때만 위치와 회전을 바꾼다.
     */
    static FP1MoveCorrection Compute(
        const FVector& ClientLocation,
        const FRotator& ClientRotation,
        const FVector& ServerLocation,
        float ServerYaw,
        const FVector& MoveDirection,
        bool bCorrectRotation,
        bool bInAction,
        float DeltaSeconds);
};
