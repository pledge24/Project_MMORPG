#pragma once

#include "CoreMinimal.h"

/**
 * 원격 크리처의 화면 위치와 회전을 서버 상태 쪽으로 끌어당긴 결과다.
 * 액터를 모르는 계산이라 게임 스레드가 아닌 곳에서도 부를 수 있다.
 */
struct P1_API FP1MoveCorrection
{
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;

    /** 거리가 기준 이상이라 보간 없이 서버 위치로 옮겼으면 true다. */
    bool bSnapped = false;

    /**
     * 한 틱 분량의 보정을 계산한다.
     * ServerLocation의 Z는 쓰지 않는다. 결과의 Z는 언제나 ClientLocation의 Z다.
     * MoveDirection이 0 벡터면 정지 중으로 보고 서버 위치를 향한다. 이동 중이면 서버 위치를 지나
     * MoveDirection과 평행한 직선 위의, 현재 위치에서 가장 가까운 점을 향한다.
     * bCorrectRotation이 false면 순간이동할 때 말고는 회전을 바꾸지 않는다.
     * DeltaSeconds는 초 단위다.
     */
    static FP1MoveCorrection Compute(
        const FVector& ClientLocation,
        const FRotator& ClientRotation,
        const FVector& ServerLocation,
        float ServerYaw,
        const FVector& MoveDirection,
        bool bCorrectRotation,
        float DeltaSeconds);
};
