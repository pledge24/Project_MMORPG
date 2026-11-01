#pragma once
#include "Game/Combat/AttackArea.h"
#include "Game/Combat/CombatTypes.h"
#include "Game/Data/Generated/DesignData.h"

/**
 * 액션 표의 값을 판정 타입으로 옮기는 변환.
 *
 * 판정(Game/Combat/)은 기획 데이터를 모르므로, 판정을 부르는 쪽이 이 변환으로 기획 데이터의 행을 넘긴다.
 */
namespace ActionData
{
    AttackArea ToAttackArea(const DesignData::ActionHitRow& hit);
    DamageType ToDamageType(DesignData::DamageType damageType);
    /** 액션의 타격 중 공격 범위가 앞으로 가장 멀리 닿는 거리(cm). 타격이 없으면 0. */
    float GetFrontReach(const DesignData::ActionRow& action);
}
