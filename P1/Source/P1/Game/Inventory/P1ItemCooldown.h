#pragma once

#include "CoreMinimal.h"

/** 아이템 하나의 재사용 대기다. 시각은 모두 같은 실시간 시계의 초 단위다. */
struct P1_API FP1ItemCooldown
{
    /** 재사용 대기가 쓰는 시계다. 레벨이 바뀌어도 이어지는 실시간 초다. */
    static double GetClockSeconds() { return FPlatformTime::Seconds(); }

    double StartSeconds = 0.0;

    /** 초 단위다. 0 이하이면 대기가 없다. */
    double DurationSeconds = 0.0;

    /** 0 이상이다. 대기가 끝났으면 0이다. */
    double GetRemainingSeconds(double NowSeconds) const;

    /** 대기 막대에 쓰는 남은 비율이다. 시작하면 1이고 끝나면 0이다. 대기가 없으면 0이다. */
    float GetRemainingRatio(double NowSeconds) const;

    /** 남은 시간이 0보다 크면 참이다. */
    bool IsCoolingDown(double NowSeconds) const;
};
