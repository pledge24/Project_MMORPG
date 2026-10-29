#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"

/*--------------------------------------------------------------
    무작위 위치 테스트

    몬스터 스폰 위치와 배회 목적지는 그 몬스터가 나온 스폰 마커의 반경 안에서 뽑는다.
    반경 밖으로 나간 몬스터는 배치한 곳과 다른 곳에 나타나고, 추적 한계를 넘어 곧바로 돌아간다.

    픽스처 결합도: 없음. Room::GetRandomLocation에 중심과 반경만 넘긴다.
---------------------------------------------------------------*/

namespace
{
    constexpr float CENTER_X = 1000.f;
    constexpr float CENTER_Y = -2000.f;
    constexpr float RADIUS = 500.f;
    constexpr int32 DRAW_COUNT = 2000;
}

TEST(RandomLocationTest, RandomLocationStaysInsideRadius)
{
    for (int32 i = 0; i < DRAW_COUNT; i++)
    {
        const vector2D pos = Room::GetRandomLocation(vector2D(CENTER_X, CENTER_Y), RADIUS);
        ASSERT_LE(MathUtil::Distance(pos, vector2D(CENTER_X, CENTER_Y)), RADIUS + 0.01f) << "반경 밖에 몬스터를 둔다";
    }
}

// 반지름을 고르게 뽑으면 원의 가운데에 몰린다. 바깥 고리(반경의 절반 밖)는 넓이의 3/4이다.
TEST(RandomLocationTest, RandomLocationSpreadsOverArea)
{
    int32 outerCount = 0;
    for (int32 i = 0; i < DRAW_COUNT; i++)
    {
        const vector2D pos = Room::GetRandomLocation(vector2D(CENTER_X, CENTER_Y), RADIUS);
        if (MathUtil::Distance(pos, vector2D(CENTER_X, CENTER_Y)) > RADIUS * 0.5f)
            outerCount++;
    }

    EXPECT_GT(outerCount, DRAW_COUNT * 0.65f) << "가운데에 몰린다";
    EXPECT_LT(outerCount, DRAW_COUNT * 0.85f);
}

TEST(RandomLocationTest, ZeroRadiusIsCenter)
{
    const vector2D pos = Room::GetRandomLocation(vector2D(CENTER_X, CENTER_Y), 0.f);

    EXPECT_FLOAT_EQ(pos.x, CENTER_X);
    EXPECT_FLOAT_EQ(pos.y, CENTER_Y);
}
