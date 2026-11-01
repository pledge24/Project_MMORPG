#pragma once
#include "Game/Combat/AttackArea.h"
#include "Game/Combat/CombatTypes.h"
#include "Game/Data/CreatureCombat.h"
#include "Game/Data/Generated/DesignData.h"

/**
 * 액션 표와 크리처 판정 데이터의 값을 판정 타입으로 옮기는 변환.
 *
 * 판정(Game/Combat/)은 기획 데이터를 모르므로, 판정을 부르는 쪽이 이 변환으로 기획 데이터의 행을 넘긴다.
 */
namespace ActionData
{
    AttackArea ToAttackArea(const DesignData::ActionHitRow& hit);
    DamageType ToDamageType(DesignData::DamageType damageType);
    /** 액션의 타격 중 공격 범위가 앞으로 가장 멀리 닿는 거리(cm). 타격이 없으면 0. */
    float GetFrontReach(const DesignData::ActionRow& action);
    /**
     * startFoot에서 yaw(도) 방향으로 시작한 액션의 타격 hitIndex가 쓰는 공격자 자세. 기준 위치는 루트 모션으로 나간 곳이다.
     * hitIndex는 timing의 타격 시점 안이어야 한다. 루트 모션이 없거나 sampleRate가 0 이하면 시작 위치 그대로다.
     */
    AttackerPose MakeHitPose(const vector3D& startFoot, float yaw, const ActionTiming& timing, int32 hitIndex, int32 sampleRate);
}
