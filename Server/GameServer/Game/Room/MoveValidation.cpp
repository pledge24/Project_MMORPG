#include "Core/pch.h"
#include "Game/Room/MoveValidation.h"

optional<string> MoveValidation::Validate(const vector2D& from, const vector2D& to, uint64 elapsedMs, const PlaneBounds& bounds)
{
    if (bounds.Contains(to.x, to.y) == false)
        return format("룸 경계 밖의 위치({}, {})", to.x, to.y);

    // 경과 시간에 상한을 두지 않는다. 버린 이동 뒤에 서버와 클라이언트의 위치가 벌어져도 시간이 흐르면 다시 받아들이기 위해서다.
    // 상한을 두면 한 번 벌어진 위치가 다시 맞지 않는다. 대신 오래 서 있다가 보낸 첫 이동은 멀리 갈 수 있다.
    const float seconds = static_cast<float>(elapsedMs) / 1000.f;
    const float allowed = PLAYER_MAX_SPEED * seconds * SPEED_TOLERANCE + DISTANCE_TOLERANCE;
    const float distance = MathUtil::Distance(from, to);
    if (distance > allowed)
        return format("{}ms 동안 {}cm를 이동(허용 {}cm)", elapsedMs, distance, allowed);

    return nullopt;
}
