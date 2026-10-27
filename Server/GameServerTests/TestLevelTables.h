#pragma once
#include "Game/Data/Templates.h"

/**
 * 테스트가 기획 데이터 표에 레벨 행을 넣는다.
 * 실제 표처럼 레벨 표의 행과 직업별 능력치 표의 행을 함께 넣는다. 마지막 레벨의 필요 경험치를 0으로 두어야 실제 표와 같다.
 */
namespace TestLevelTables
{
    /** 레벨 표에 레벨을 넣고, 전사의 그 레벨 능력치 행을 돌려준다. 능력치는 0으로 시작한다. */
    inline DesignData::ClassLevelStatRow& AddWarriorLevel(GamedataTables& tables, int32 level, int64 expRequirement = 0)
    {
        DesignData::LevelRow& levelRow = tables.levels[level];
        levelRow.Level = level;
        levelRow.ExpRequirement = expRequirement;

        DesignData::ClassLevelStatRow& stat = tables.classLevelStats[{ Protocol::CHARACTER_CLASS_WARRIOR, level }];
        stat.Class = Protocol::CHARACTER_CLASS_WARRIOR;
        stat.Level = level;
        return stat;
    }
}
