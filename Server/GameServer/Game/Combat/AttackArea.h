#pragma once
#include "Utils/Utils.h"

/** 공격 범위의 평면 형상. 3D 형상이나 회전한 형상은 종류를 더해 넓힌다. */
enum class AttackShape : uint8
{
    Circle,
    Sector,
    Rectangle,
};

/**
 * 타격 하나의 공격 범위. 거리는 cm, 각도는 도다.
 * 공격자 기준으로 +X가 앞, +Y가 오른쪽이고, 높이는 공격자 발밑 기준이다.
 */
struct AttackArea
{
    AttackShape shape = AttackShape::Circle;

    /** 원과 부채꼴의 반지름. */
    float radius = 0.f;
    /** 부채꼴의 전체 각도. 앞 방향을 가운데에 두고 좌우로 절반씩 벌어진다. */
    float angle = 0.f;
    /** 직사각형의 앞 방향 길이. 기준점에서 앞으로 뻗는다. */
    float length = 0.f;
    /** 직사각형의 좌우 폭. 기준점을 가운데에 둔다. */
    float width = 0.f;

    /** 형상의 기준점(원의 중심, 부채꼴의 꼭짓점, 직사각형의 뒤쪽 변 가운데)을 공격자에게서 옮긴 거리. */
    float offsetX = 0.f;
    float offsetY = 0.f;

    float zMin = 0.f;
    float zMax = 0.f;
};

/** 공격자의 발밑 위치와 방향. */
struct AttackerPose
{
    vector3D foot;
    float yaw = 0.f;
};

/** 대상 크리처의 발밑 위치와 캡슐. 캡슐은 발밑에서 반높이의 두 배까지 선다. */
struct TargetCapsule
{
    vector3D foot;
    float radius = 0.f;
    float halfHeight = 0.f;
};

/**
 * 공격 범위의 기하 판정.
 *
 * 공격 범위가 대상 캡슐에 닿는지와 범위가 앞으로 닿는 거리를 계산한다. 룸과 기획 데이터를 모르는 순수 함수다.
 * 클라이언트의 공격 범위 디버그 표시가 같은 규칙으로 그리므로, 규칙을 바꾸면 클라이언트와 공용 테스트 사례도 고친다.
 */
namespace AttackAreaMath
{
    /** 경계 판정에 더하는 여유(cm). 공격자를 돌린 계산의 부동소수 오차로 경계에 닿은 대상이 갈리지 않게 한다. */
    constexpr float OVERLAP_TOLERANCE = 0.01f;

    /** 평면에서 대상 원이 형상에 닿고 높이 구간이 겹치면 true. 경계에 닿기만 해도 걸린다. */
    bool Overlaps(const AttackArea& area, const AttackerPose& attacker, const TargetCapsule& target);

    /** 공격자 기준으로 범위가 앞으로 가장 멀리 닿는 거리(cm). 대상의 반지름은 더하지 않는다. */
    float GetFrontReach(const AttackArea& area);
}
