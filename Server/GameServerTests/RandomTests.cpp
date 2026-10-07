#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Utils/Utils.h"

/*--------------------------------------------------------------
    난수 범위 테스트

    GetRandom은 정수와 실수 모두 양끝을 포함한다. 몬스터 보상처럼 최솟값과 최댓값을
    기획 데이터에서 그대로 넘기는 호출부가 있어서, 최댓값이 나오지 않거나 두 값이
    같을 때 정의되지 않은 동작이 되면 보상이 틀린다.

    픽스처 결합도: 없음.
---------------------------------------------------------------*/

namespace
{
    constexpr int32 DRAW_COUNT = 1000;
}

TEST(RandomTest, IntegerRangeWithSameBoundsReturnsThatValue)
{
    for (int32 i = 0; i < DRAW_COUNT; i++)
        EXPECT_EQ(Utils::GetRandom<int64>(5, 5), 5);
}

TEST(RandomTest, IntegerRangeIncludesMax)
{
    bool sawMin = false;
    bool sawMax = false;
    for (int32 i = 0; i < DRAW_COUNT; i++)
    {
        const int32 value = Utils::GetRandom(0, 1);
        ASSERT_GE(value, 0);
        ASSERT_LE(value, 1);

        sawMin |= (value == 0);
        sawMax |= (value == 1);
    }

    EXPECT_TRUE(sawMin);
    EXPECT_TRUE(sawMax) << "최댓값이 나오지 않으면 보상의 maxExp·maxGold가 영영 나오지 않는다";
}
