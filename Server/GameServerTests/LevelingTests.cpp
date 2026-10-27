#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Progress/Leveling.h"

/*--------------------------------------------------------------
    레벨 판정 테스트

    경험치는 누적값으로 저장하고, 레벨은 누적 경험치와 레벨 표로 다시 계산한다. 레벨 L은
    1 ~ L-1 레벨 요구치의 합이 누적 경험치 이하인 가장 큰 값이다. 클라이언트에는 레벨 안의
    경험치와 그 레벨의 요구치를 보낸다.

    픽스처 결합도: 없음. 레벨 표를 직접 만들어 넘긴다.
---------------------------------------------------------------*/

namespace
{
    // 레벨 1 → 2에 100, 2 → 3에 200, 3 → 4에 300. 레벨 4가 최대 레벨이다.
    // 레벨이 시작하는 누적 경험치는 0, 100, 300, 600이다.
    DesignData::LevelTable MakeLevelTable()
    {
        DesignData::LevelTable table;
        for (int64 requirement : { 100, 200, 300, 0 })
        {
            const int32 level = static_cast<int32>(table.size()) + 1;
            table[level] = DesignData::LevelRow{ level, requirement };
        }

        return table;
    }

    constexpr int64 MAX_TOTAL_EXP = 600;
}

TEST(LevelingTest, ZeroExpIsLevelOne)
{
    const LevelExp levelExp = Leveling::Locate(MakeLevelTable(), 0);

    EXPECT_EQ(levelExp.level, 1);
    EXPECT_EQ(levelExp.expInLevel, 0);
    EXPECT_EQ(levelExp.expRequirement, 100);
}

TEST(LevelingTest, ExpEqualToRequirementSumReachesLevel)
{
    const DesignData::LevelTable table = MakeLevelTable();

    const LevelExp level2 = Leveling::Locate(table, 100);
    EXPECT_EQ(level2.level, 2);
    EXPECT_EQ(level2.expInLevel, 0) << "레벨에 막 오른 순간에는 레벨 안의 경험치가 0이다";
    EXPECT_EQ(level2.expRequirement, 200);

    EXPECT_EQ(Leveling::Locate(table, 300).level, 3);
}

TEST(LevelingTest, ExpOneBelowRequirementSumStaysAtLowerLevel)
{
    const DesignData::LevelTable table = MakeLevelTable();

    const LevelExp level1 = Leveling::Locate(table, 99);
    EXPECT_EQ(level1.level, 1);
    EXPECT_EQ(level1.expInLevel, 99);

    const LevelExp level2 = Leveling::Locate(table, 299);
    EXPECT_EQ(level2.level, 2);
    EXPECT_EQ(level2.expInLevel, 199) << "클라이언트에는 누적값이 아니라 레벨 안의 경험치를 보낸다";
    EXPECT_EQ(level2.expRequirement, 200);
}

TEST(LevelingTest, OneRewardRaisesSeveralLevels)
{
    const DesignData::LevelTable table = MakeLevelTable();

    const int64 totalExp = Leveling::AddExp(table, 50, 400);

    EXPECT_EQ(totalExp, 450);
    const LevelExp levelExp = Leveling::Locate(table, totalExp);
    EXPECT_EQ(levelExp.level, 3);
    EXPECT_EQ(levelExp.expInLevel, 150);
    EXPECT_EQ(levelExp.expRequirement, 300);
}

TEST(LevelingTest, MaxTotalExpIsSumOfRequirementsBelowMaxLevel)
{
    EXPECT_EQ(Leveling::GetMaxTotalExp(MakeLevelTable()), MAX_TOTAL_EXP) << "최대 레벨 행의 요구치는 더하지 않는다";
}

TEST(LevelingTest, ExpDoesNotAccumulateAtMaxLevel)
{
    const DesignData::LevelTable table = MakeLevelTable();

    EXPECT_EQ(Leveling::AddExp(table, 590, 100), MAX_TOTAL_EXP) << "최대 레벨에 이르고 남는 경험치는 버린다";
    EXPECT_EQ(Leveling::AddExp(table, MAX_TOTAL_EXP, 30), MAX_TOTAL_EXP);

    const LevelExp levelExp = Leveling::Locate(table, MAX_TOTAL_EXP);
    EXPECT_EQ(levelExp.level, 4);
    EXPECT_EQ(levelExp.expInLevel, 0) << "최대 레벨에서는 레벨 안의 경험치가 쌓이지 않는다";
}

TEST(LevelingTest, ClampCutsExpBeyondMaxLevel)
{
    const DesignData::LevelTable table = MakeLevelTable();

    EXPECT_EQ(Leveling::ClampTotalExp(table, MAX_TOTAL_EXP + 1), MAX_TOTAL_EXP);
    EXPECT_EQ(Leveling::ClampTotalExp(table, MAX_TOTAL_EXP), MAX_TOTAL_EXP);
    EXPECT_EQ(Leveling::ClampTotalExp(table, -1), 0) << "음수 경험치로 레벨 1 밑을 가리키면 안 된다";
}

TEST(LevelingTest, LocateBeyondMaxLevelStaysAtMaxLevel)
{
    const LevelExp levelExp = Leveling::Locate(MakeLevelTable(), MAX_TOTAL_EXP + 50);

    EXPECT_EQ(levelExp.level, 4);
    EXPECT_EQ(levelExp.expInLevel, 0);
}

/* 사망 페널티는 그 레벨 요구치의 10%다. 레벨 안에 쌓인 경험치보다 많이 잃지 않으므로 레벨이 내려가지 않는다. */

TEST(LevelingTest, DeathPenaltyLosesTenPercentOfRequirement)
{
    // 레벨 2, 레벨 안 30, 요구치 200
    EXPECT_EQ(Leveling::ApplyDeathPenalty(MakeLevelTable(), 130), 110);
}

TEST(LevelingTest, DeathPenaltyDoesNotLowerLevel)
{
    const DesignData::LevelTable table = MakeLevelTable();

    // 레벨 2, 레벨 안 10. 요구치의 10%인 20을 다 잃으면 레벨 1로 내려간다.
    const int64 totalExp = Leveling::ApplyDeathPenalty(table, 110);

    EXPECT_EQ(totalExp, 100);
    EXPECT_EQ(Leveling::Locate(table, totalExp).level, 2);
    EXPECT_EQ(Leveling::ApplyDeathPenalty(table, 100), 100) << "레벨이 시작하는 값에서는 잃을 경험치가 없다";
}

TEST(LevelingTest, DeathPenaltyAtMaxLevelKeepsExp)
{
    EXPECT_EQ(Leveling::ApplyDeathPenalty(MakeLevelTable(), MAX_TOTAL_EXP), MAX_TOTAL_EXP);
}
