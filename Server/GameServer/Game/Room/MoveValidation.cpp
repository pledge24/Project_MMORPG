#include "Core/pch.h"
#include "Game/Room/MoveValidation.h"
#include "Game/Navigation/NavQuery.h"

optional<string> MoveValidation::Validate(const vector3D& from, const vector3D& to, uint64 elapsedMs, const PlaneBounds& bounds, NavQuery* terrain)
{
    if (bounds.Contains(to.x, to.y) == false)
        return format("룸 경계 밖의 위치({}, {})", to.x, to.y);

    // 경과 시간에 상한을 두지 않는다. 버린 이동 뒤에 서버와 클라이언트의 위치가 벌어져도 시간이 흐르면 다시 받아들이기 위해서다.
    // 상한을 두면 한 번 벌어진 위치가 다시 맞지 않는다. 대신 오래 서 있다가 보낸 첫 이동은 멀리 갈 수 있다.
    const float seconds = static_cast<float>(elapsedMs) / 1000.f;
    const float allowed = PLAYER_MAX_SPEED * seconds * SPEED_TOLERANCE + DISTANCE_TOLERANCE;
    const float distance = MathUtil::Distance(vector2D(from.x, from.y), vector2D(to.x, to.y));
    if (distance > allowed)
        return format("{}ms 동안 {}cm를 이동(허용 {}cm)", elapsedMs, distance, allowed);

    if (terrain == nullptr)
        return nullopt;

    // 도착 위치 아래에 걸을 수 있는 면이 있고, 캡슐이 그 면 위에 서 있어야 한다. 점프가 없으므로 떠 있는 위치는 조작이다.
    const optional<float> floor = terrain->FindFloorHeight(to);
    if (floor.has_value() == false)
        return format("걸을 수 있는 바닥이 없는 위치({}, {}, {})", to.x, to.y, to.z);

    const float standingZ = floor.value() + terrain->GetAgentHalfHeight();
    if (abs(to.z - standingZ) > HEIGHT_TOLERANCE)
        return format("바닥 위 높이 {}와 {}cm 떨어진 위치({}, {}, {})", standingZ, to.z - standingZ, to.x, to.y, to.z);

    // 출발 위치에서 표면을 따라 도착 위치로 가 본다. 벽에 막혀 멈춘 점이 도착 위치에서 멀면 벽을 통과했다.
    // 벽에 붙어 미끄러지는 캡슐은 navmesh 가장자리를 따라 미끄러져 도착 위치 가까이에서 멈추므로 받아들인다.
    const optional<vector3D> reached = terrain->MoveAlongSurface(from, to);
    if (reached.has_value() == false)
        return format("출발 위치({}, {}, {}) 근처에 걸을 수 있는 바닥이 없다", from.x, from.y, from.z);

    const float gap = MathUtil::Distance(vector2D(reached->x, reached->y), vector2D(to.x, to.y));
    if (gap > WALL_TOLERANCE)
        return format("벽을 통과한 이동({}, {}) → ({}, {}). 벽에 막힌 점({}, {})", from.x, from.y, to.x, to.y, reached->x, reached->y);

    return nullopt;
}
