#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * 기획 데이터의 공격 범위를 월드에 그리는 디버그 표시.
 *
 * 기획자가 액션의 공격 범위를 화면에서 보고 수치를 정하게 한다. 화면에 그린 범위가 서버의 판정 범위다.
 * 모든 함수는 게임 스레드 전용이고, 디버그 그리기가 없는 빌드에서는 아무것도 그리지 않는다.
 */
namespace P1AttackAreaDebugDraw
{
    /**
     * p1.Debug.ShowAttackAreas가 켜져 있으면 액션의 HitIndex번 타격의 공격 범위를 그린다. 액션 재생이 타격 시점마다 부른다.
     * AttackerFoot은 공격자의 발밑(캡슐 중심이 아니다), AttackerYaw는 공격자의 방향(도)이다.
     */
    P1_API void DrawHitIfEnabled(const UWorld* World, int32 ActionId, int32 HitIndex, const FVector& AttackerFoot, float AttackerYaw);

    /** 액션의 타격마다 공격 범위를 그리고 그린 타격 수를 돌려준다. 액션 행이 없으면 0. */
    P1_API int32 DrawAction(const UWorld* World, int32 ActionId, const FVector& AttackerFoot, float AttackerYaw);
}
