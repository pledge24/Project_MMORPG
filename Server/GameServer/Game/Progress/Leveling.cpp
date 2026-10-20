#include "Core/pch.h"
#include "Game/Progress/Leveling.h"

namespace
{
    // 사망 페널티로 잃는 비율. 그 레벨 요구치를 기준으로 한다.
    constexpr int64 DEATH_PENALTY_PERCENT = 10;
}

int64 Leveling::GetMaxTotalExp(const ClassLevelTable& table)
{
    // 최대 레벨 행의 요구치는 오를 레벨이 없으므로 더하지 않는다.
    int64 sum = 0;
    for (int32 level = 1; level < table.GetMaxLevel(); level++)
        sum += table.Find(level)->expRequirement;

    return sum;
}

int64 Leveling::ClampTotalExp(const ClassLevelTable& table, int64 totalExp)
{
    return clamp<int64>(totalExp, 0, GetMaxTotalExp(table));
}

LevelExp Leveling::Locate(const ClassLevelTable& table, int64 totalExp)
{
    const int64 clampedExp = ClampTotalExp(table, totalExp);

    // 앞 레벨부터 요구치를 빼 나간다. 요구치 합이 누적 경험치와 같으면 그 레벨에 오른 것이다.
    LevelExp result;
    result.expInLevel = clampedExp;
    while (result.level < table.GetMaxLevel())
    {
        const int64 requirement = table.Find(result.level)->expRequirement;
        if (result.expInLevel < requirement)
            break;

        result.expInLevel -= requirement;
        result.level++;
    }

    result.expRequirement = table.Find(result.level)->expRequirement;
    return result;
}

int64 Leveling::AddExp(const ClassLevelTable& table, int64 totalExp, int64 gainedExp)
{
    return ClampTotalExp(table, totalExp + gainedExp);
}

int64 Leveling::ApplyDeathPenalty(const ClassLevelTable& table, int64 totalExp)
{
    // 레벨 안에 쌓인 경험치보다 많이 잃지 않는다. 레벨이 시작하는 값 밑으로 내려가면 레벨이 떨어진다.
    const LevelExp levelExp = Locate(table, totalExp);
    const int64 lossExp = min(levelExp.expRequirement * DEATH_PENALTY_PERCENT / 100, levelExp.expInLevel);

    return ClampTotalExp(table, totalExp) - lossExp;
}
