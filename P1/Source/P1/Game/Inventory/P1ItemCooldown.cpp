#include "Game/Inventory/P1ItemCooldown.h"

double FP1ItemCooldown::GetRemainingSeconds(double NowSeconds) const
{
    return FMath::Max(0.0, StartSeconds + DurationSeconds - NowSeconds);
}

float FP1ItemCooldown::GetRemainingRatio(double NowSeconds) const
{
    if (DurationSeconds <= 0.0)
        return 0.f;

    return static_cast<float>(GetRemainingSeconds(NowSeconds) / DurationSeconds);
}

bool FP1ItemCooldown::IsCoolingDown(double NowSeconds) const
{
    return GetRemainingSeconds(NowSeconds) > 0.0;
}
