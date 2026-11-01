#include "Core/pch.h"
#include "Game/Combat/AttackArea.h"

// 클라이언트의 공격 범위 디버그 표시가 같은 규칙으로 그린다. 규칙을 바꾸면 클라이언트 계산과 공용 사례 파일도 함께 고쳐야
// 화면의 범위와 서버의 판정이 어긋나지 않는다.
namespace
{
    constexpr float DEGREES_TO_RADIANS = numbers::pi_v<float> / 180.f;

    /** 형상 기준점에서 본 점(앞 +x, 오른쪽 +y)과 형상 사이의 평면 거리. 형상 안이면 0이다. */
    float DistanceToShape(const AttackArea& area, float x, float y)
    {
        switch (area.shape)
        {
        case AttackShape::Circle:
            return (std::max)(0.f, hypot(x, y) - area.radius);

        case AttackShape::Sector:
        {
            const float halfAngle = area.angle * 0.5f;
            const float distance = hypot(x, y);
            if (halfAngle >= 180.f || distance == 0.f)
                return (std::max)(0.f, distance - area.radius);

            // 좌우가 대칭이므로 점을 오른쪽으로 접고, 오른쪽 가장자리 선분 하나와만 잰다.
            const float foldedY = abs(y);
            if (atan2(foldedY, x) <= halfAngle * DEGREES_TO_RADIANS)
                return (std::max)(0.f, distance - area.radius);

            // 각도 밖이면 가장 가까운 곳은 가장자리 선분(꼭짓점과 호의 끝 포함) 위에 있다. 호까지만 재면 호 끝 너머의 대상이 걸린다.
            const float edgeX = cos(halfAngle * DEGREES_TO_RADIANS);
            const float edgeY = sin(halfAngle * DEGREES_TO_RADIANS);
            const float along = clamp(x * edgeX + foldedY * edgeY, 0.f, area.radius);
            return hypot(x - edgeX * along, foldedY - edgeY * along);
        }

        case AttackShape::Rectangle:
        {
            // 변을 대상 반지름만큼 늘린 사각형으로 재면 모서리 대각선 밖의 대상이 걸린다. 가장 가까운 점까지 잰다.
            const float outsideX = (std::max)({ -x, 0.f, x - area.length });
            const float outsideY = (std::max)(0.f, abs(y) - area.width * 0.5f);
            return hypot(outsideX, outsideY);
        }
        }

        return (numeric_limits<float>::max)();
    }
}

bool AttackAreaMath::Overlaps(const AttackArea& area, const AttackerPose& attacker, const TargetCapsule& target)
{
    // 높이 범위는 공격자 발밑 기준이고, 대상은 발밑에서 반높이의 두 배까지 선다.
    const float areaBottom = attacker.foot.z + area.zMin;
    const float areaTop = attacker.foot.z + area.zMax;
    const float targetBottom = target.foot.z;
    const float targetTop = target.foot.z + target.halfHeight * 2.f;
    if (targetTop < areaBottom - OVERLAP_TOLERANCE || targetBottom > areaTop + OVERLAP_TOLERANCE)
        return false;

    // 대상 위치를 공격자 기준(앞 +x, 오른쪽 +y)으로 돌린 뒤 형상 기준점으로 옮긴다. yaw는 +x에서 +y 쪽으로 돈다.
    const float yawRadians = attacker.yaw * DEGREES_TO_RADIANS;
    const float forwardX = cos(yawRadians);
    const float forwardY = sin(yawRadians);
    const float dx = target.foot.x - attacker.foot.x;
    const float dy = target.foot.y - attacker.foot.y;
    const float localX = dx * forwardX + dy * forwardY - area.offsetX;
    const float localY = -dx * forwardY + dy * forwardX - area.offsetY;

    return DistanceToShape(area, localX, localY) <= target.radius + OVERLAP_TOLERANCE;
}

float AttackAreaMath::GetFrontReach(const AttackArea& area)
{
    switch (area.shape)
    {
    case AttackShape::Circle:
    case AttackShape::Sector:
        // 부채꼴은 앞 방향을 가운데에 두므로 각도와 무관하게 반지름만큼 앞으로 닿는다.
        return area.offsetX + area.radius;

    case AttackShape::Rectangle:
        return area.offsetX + area.length;
    }

    return area.offsetX;
}

float AttackAreaMath::GetPlaneReach(const AttackArea& area)
{
    // 형상을 기준점 둘레의 원으로 덮고 기준점까지의 거리를 더한다. 부채꼴과 직사각형에는 실제보다 넉넉하다.
    const float offset = hypot(area.offsetX, area.offsetY);
    switch (area.shape)
    {
    case AttackShape::Circle:
    case AttackShape::Sector:
        return offset + area.radius;

    case AttackShape::Rectangle:
        return offset + hypot(area.length, area.width * 0.5f);
    }

    return offset;
}
