#pragma once
#include "Game/Data/Templates.h"

/** 누적 경험치가 가리키는 레벨과, 클라이언트에 보낼 그 레벨 안의 값. */
struct LevelExp
{
    int32 level = 1;
    /** 레벨에 오른 뒤 쌓인 경험치. 최대 레벨에서는 0이다. */
    int64 expInLevel = 0;
    /** 이 레벨에서 다음 레벨로 가는 데 필요한 경험치. 레벨 표의 값 그대로다. */
    int64 expRequirement = 0;
};

/**
 * 경험치와 레벨의 규칙.
 *
 * 누적 경험치와 레벨 표로 레벨을 정하고, 보상과 사망 페널티가 누적 경험치를 어떻게 바꾸는지 판정한다.
 * 레벨 표는 행이 하나 이상이고 레벨이 1부터 빠짐없이 이어져야 한다.
 */
namespace Leveling
{
    /** 최대 레벨에 이르는 누적 경험치. 누적 경험치는 이 값을 넘지 않는다. */
    int64 GetMaxTotalExp(const DesignData::LevelTable& table);
    /** 누적 경험치를 0 ~ 최대 레벨에 이르는 값 사이로 자른다. */
    int64 ClampTotalExp(const DesignData::LevelTable& table, int64 totalExp);
    /** 레벨 L은 1 ~ L-1 레벨 요구치의 합이 누적 경험치 이하인 가장 큰 값이다. 범위 밖의 값은 잘라서 본다. */
    LevelExp Locate(const DesignData::LevelTable& table, int64 totalExp);
    /** 보상을 더한 누적 경험치. 최대 레벨에 이르고 남는 경험치는 버린다. */
    int64 AddExp(const DesignData::LevelTable& table, int64 totalExp, int64 gainedExp);
    /** 마을 리스폰의 사망 페널티를 적용한 누적 경험치. 레벨은 내려가지 않는다. */
    int64 ApplyDeathPenalty(const DesignData::LevelTable& table, int64 totalExp);
}
