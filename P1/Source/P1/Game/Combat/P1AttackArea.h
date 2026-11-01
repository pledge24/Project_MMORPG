#pragma once

#include "CoreMinimal.h"

struct FP1ActionHitRow;

/**
 * 타격 하나의 공격 범위를 서버와 같은 규칙으로 계산한다.
 *
 * 디버그 표시가 서버의 판정 범위를 그대로 그리게 하는 기하 계산이다. 클라이언트는 이 계산으로 피격을 판정하지 않는다.
 * 거리는 cm, 각도는 도, 공격자 기준 +X가 앞, +Y가 오른쪽, 높이는 공격자 발밑 기준이다.
 */
namespace P1AttackArea
{
    /** 경계 판정에 더하는 여유(cm). 서버의 값과 같아야 경계에 닿은 대상의 답이 같다. */
    inline constexpr float OverlapTolerance = 0.01f;

    /**
     * 발밑이 TargetFoot이고 반지름과 반높이가 그만큼인 캡슐에 공격 범위가 닿으면 true. 경계에 닿기만 해도 걸린다.
     * AttackerYaw는 +X에서 +Y 쪽으로 도는 방향(도)이다.
     */
    P1_API bool Overlaps(const FP1ActionHitRow& Hit, const FVector& AttackerFoot, float AttackerYaw,
        const FVector& TargetFoot, float TargetRadius, float TargetHalfHeight);

    /**
     * 공격 범위의 평면 윤곽선. 공격자 기준 좌표의 꼭짓점을 차례로 담고, 마지막 꼭짓점은 첫 꼭짓점으로 닫힌다.
     * 원은 ArcSegments개, 부채꼴의 호는 ArcSegments 구간으로 나눈다. 형상이 없으면 빈 배열.
     */
    P1_API TArray<FVector2D> MakeOutline(const FP1ActionHitRow& Hit, int32 ArcSegments);

    /** 공격자 기준 평면 좌표 Local을 월드로 옮긴다. 높이는 공격자 발밑에서 Height만큼 위다. */
    P1_API FVector ToWorld(const FVector2D& Local, const FVector& AttackerFoot, float AttackerYaw, float Height);
}
