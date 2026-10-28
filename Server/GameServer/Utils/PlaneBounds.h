#pragma once

/**
 * 평면의 축 정렬 경계.
 *
 * 맵과 지역의 경계 계산과 포함 판정을 한 곳에 둔다. 룸 배정, 룸 경계, 이동 판정이 같은 규칙을 쓴다.
 */
struct PlaneBounds
{
    float minX = 0.f;
    float maxX = 0.f;
    float minY = 0.f;
    float maxY = 0.f;

    /** 중심과 X, Y 방향 반경으로 만든다. */
    static PlaneBounds FromCenter(float centerX, float centerY, float halfExtentX, float halfExtentY)
    {
        return PlaneBounds{ centerX - halfExtentX, centerX + halfExtentX, centerY - halfExtentY, centerY + halfExtentY };
    }

    /** 경계 위의 점은 안쪽이다. 높이는 보지 않는다. */
    bool Contains(float x, float y) const
    {
        return x >= minX && x <= maxX && y >= minY && y <= maxY;
    }
};
