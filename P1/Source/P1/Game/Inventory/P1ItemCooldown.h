#pragma once

#include "CoreMinimal.h"

/**
 * 아이템 하나의 재사용 대기.
 *
 * 재사용 대기의 시작과 길이를 담고, 주어진 시각에 대기가 얼마나 남았는지 알려 준다.
 * 시각은 모두 GetClockSeconds 시계의 초 단위다.
 */
struct P1_API FP1ItemCooldown
{
    /** 재사용 대기가 쓰는 시계. 레벨이 바뀌어도 이어지는 실시간 초. */
    static double GetClockSeconds() { return FPlatformTime::Seconds(); }

    double StartSeconds = 0.0;

    /** 초 단위. 0 이하이면 대기가 없다. */
    double DurationSeconds = 0.0;

    /** 대기가 끝났으면 0. 음수는 돌려주지 않는다. */
    double GetRemainingSeconds(double NowSeconds) const;

    /** 대기 막대에 쓰는 남은 비율. 시작하면 1, 끝났거나 대기가 없으면 0. */
    float GetRemainingRatio(double NowSeconds) const;

    bool IsCoolingDown(double NowSeconds) const;
};
