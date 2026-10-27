#pragma once
#include "Game/Data/Templates.h"

/**
 * 테스트가 기획 데이터 표에 레벨 행을 넣는다.
 * 실제 표처럼 레벨 표의 행과 직업별 능력치 표의 행을 함께 넣는다. 마지막 레벨의 필요 경험치를 0으로 두어야 실제 표와 같다.
 */
namespace TestLevelTables
{
    /** 레벨 표에 레벨을 넣고, 그 레벨의 능력치 행을 돌려준다. 능력치는 0으로 시작한다. */
    inline DesignData::ClassLevelStatRow& AddLevel(GamedataTables& tables, Protocol::CharacterClass characterClass, int32 level, int64 expRequirement = 0)
    {
        DesignData::LevelRow& levelRow = tables.levels[level];
        levelRow.Level = level;
        levelRow.ExpRequirement = expRequirement;

        DesignData::ClassLevelStatRow& stat = tables.classLevelStats[{ characterClass, level }];
        stat.Class = characterClass;
        stat.Level = level;
        return stat;
    }

    /** 전사의 레벨을 넣는다. 테스트 대부분이 전사 하나로 돈다. */
    inline DesignData::ClassLevelStatRow& AddWarriorLevel(GamedataTables& tables, int32 level, int64 expRequirement = 0)
    {
        return AddLevel(tables, Protocol::CHARACTER_CLASS_WARRIOR, level, expRequirement);
    }
}
