#pragma once
#include "Game/Data/Templates.h"

/**
 * 테스트가 몬스터 표의 몬스터에 크리처 판정 데이터를 함께 넣는다.
 * 몬스터는 판정 데이터가 없으면 만들어지지 않는다. 실제 데이터처럼 공격 몽타주 하나에 섹션 하나, 타격 시점 하나다.
 */
namespace TestCreatureCombat
{
    /** 첫 공격의 타격 시점이 hitTime초인 판정 데이터. */
    inline CreatureCombat MakeCombat(float hitTime)
    {
        AttackSectionCombat section;
        section.name = "Default";
        section.length = hitTime + 0.5f;
        section.hitTimes = { hitTime };

        AttackCombat attack;
        attack.montage = "/Game/Test/AM_TestAttack.AM_TestAttack";
        attack.sections = { section };

        CreatureCombat combat;
        combat.capsuleRadius = 34.f;
        combat.capsuleHalfHeight = 88.f;
        combat.attacks = { attack };
        return combat;
    }

    inline void AddMonster(GamedataTables& tables, int32 templateId, float hitTime = 0.2f)
    {
        tables.monsterCombats[templateId] = MakeCombat(hitTime);
    }
}
