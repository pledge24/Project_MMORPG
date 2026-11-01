#pragma once
#include "Game/Data/Templates.h"

/**
 * 테스트가 몬스터 표의 몬스터에 크리처 판정 데이터와 일반 공격 액션의 타이밍을 함께 넣는다.
 * 몬스터는 둘 중 하나라도 없으면 만들어지지 않는다. 실제 데이터처럼 일반 공격의 타격은 하나다.
 */
namespace TestCreatureCombat
{
    /** 첫 타격 시점이 hitTime초인 액션 타이밍. */
    inline ActionTiming MakeTiming(float hitTime)
    {
        ActionTiming timing;
        timing.montage = "/Game/Test/AM_TestAttack.AM_TestAttack";
        timing.section = "Default";
        timing.length = hitTime + 0.5f;
        timing.hitTimes = { hitTime };
        return timing;
    }

    /** 몬스터 번호에 더해 그 몬스터의 일반 공격 액션 번호를 만든다. 몬스터 번호와 겹치지 않아야 서버가 액션 번호로 찾는지 드러난다. */
    constexpr int32 ACTION_ID_OFFSET = 100000;

    /**
     * 몬스터 표에 먼저 넣은 templateId 몬스터의 판정 데이터를 넣는다. 일반 공격 액션은 몬스터마다 따로 두려고
     * 그 몬스터의 NormalAttackAction을 templateId + ACTION_ID_OFFSET으로 바꾼다.
     */
    inline void AddMonster(GamedataTables& tables, int32 templateId, float hitTime = 0.2f)
    {
        CreatureCombat combat;
        combat.capsuleRadius = 34.f;
        combat.capsuleHalfHeight = 88.f;
        tables.monsterCombats[templateId] = combat;

        const int32 actionId = templateId + ACTION_ID_OFFSET;
        tables.monsters.at(templateId).NormalAttackAction = actionId;
        tables.actionTimings[actionId] = MakeTiming(hitTime);
    }
}
