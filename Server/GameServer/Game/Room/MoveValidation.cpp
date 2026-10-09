#include "Core/pch.h"
#include "Game/Room/MoveValidation.h"

optional<string> MoveValidation::Validate(const vector2D& from, const vector2D& to, uint64 elapsedMs, const Bounds& bounds)
{
    if (to.x < bounds.minX || to.x > bounds.maxX || to.y < bounds.minY || to.y > bounds.maxY)
        return format("룸 경계 밖의 위치({}, {})", to.x, to.y);

    const float seconds = static_cast<float>(min(elapsedMs, MAX_ELAPSED_MS)) / 1000.f;
    const float allowed = PLAYER_MAX_SPEED * seconds * SPEED_TOLERANCE + DISTANCE_TOLERANCE;
    const float distance = MathUtil::Distance(from, to);
    if (distance > allowed)
        return format("{}ms 동안 {}cm를 이동(허용 {}cm)", elapsedMs, distance, allowed);

    return nullopt;
}
