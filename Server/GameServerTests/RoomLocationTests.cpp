#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"

/*--------------------------------------------------------------
    무작위 위치 테스트

    몬스터 스폰 위치와 배회 목적지는 그 몬스터가 스폰한 지역의 경계에서 여백을 뺀 안쪽에서 뽑는다.
    경계에 붙거나 밖으로 밀린 몬스터는 지역 사이의 빈 공간으로 걸어 나간다.

    픽스처 결합도: 없음. Room::GetRandomLocation에 경계만 넘긴다. 여백 값은 Room.h의 LOCATION_PADDING_X·Y와 같다.

    축: 언리얼 좌표를 따른다. 지역 볼륨의 X 방향 반경이 x 범위를, Y 방향 반경이 y 범위를 정한다.
---------------------------------------------------------------*/

namespace
{
    constexpr float CENTER_X = 1000.f;
    constexpr float CENTER_Y = -2000.f;
    constexpr float HALF_EXTENT = 1500.f;
    constexpr float PADDING = 1000.f;
    constexpr int32 DRAW_COUNT = 1000;

    PlaneBounds MakeBounds(float depthHalfExtent, float widthHalfExtent)
    {
        return PlaneBounds::FromCenter(CENTER_X, CENTER_Y, depthHalfExtent, widthHalfExtent);
    }
}

TEST(RandomLocationTest, RandomLocationStaysInsidePadding)
{
    const PlaneBounds bounds = MakeBounds(HALF_EXTENT, HALF_EXTENT);
    const float innerHalf = HALF_EXTENT - PADDING;

    for (int32 i = 0; i < DRAW_COUNT; i++)
    {
        const vector2D pos = Room::GetRandomLocation(bounds);
        ASSERT_GE(pos.x, CENTER_X - innerHalf) << "여백을 무시하면 몬스터가 지역 경계에 붙어 스폰된다";
        ASSERT_LE(pos.x, CENTER_X + innerHalf);
        ASSERT_GE(pos.y, CENTER_Y - innerHalf);
        ASSERT_LE(pos.y, CENTER_Y + innerHalf);
    }
}

TEST(RandomLocationTest, RandomLocationWithoutPaddingUsesWholeBounds)
{
    const PlaneBounds bounds = MakeBounds(HALF_EXTENT, HALF_EXTENT);

    for (int32 i = 0; i < DRAW_COUNT; i++)
    {
        const vector2D pos = Room::GetRandomLocation(bounds, false);
        ASSERT_GE(pos.x, CENTER_X - HALF_EXTENT);
        ASSERT_LE(pos.x, CENTER_X + HALF_EXTENT);
        ASSERT_GE(pos.y, CENTER_Y - HALF_EXTENT);
        ASSERT_LE(pos.y, CENTER_Y + HALF_EXTENT);
    }
}

TEST(RandomLocationTest, BoundsNarrowerThanPaddingUseCenter)
{
    const vector2D pos = Room::GetRandomLocation(MakeBounds(500.f, 3000.f));

    EXPECT_FLOAT_EQ(pos.x, CENTER_X) << "여백보다 좁은 축에서 범위가 뒤집히면 무작위 함수가 터진다";
    EXPECT_GE(pos.y, CENTER_Y - 2000.f);
    EXPECT_LE(pos.y, CENTER_Y + 2000.f);
}

TEST(RandomLocationAxisTest, DepthBoundsXAndWidthBoundsY)
{
    constexpr float DEPTH_HALF = 3000.f;
    constexpr float WIDTH_HALF = 500.f;

    const PlaneBounds bounds = MakeBounds(DEPTH_HALF, WIDTH_HALF);

    float maxOffsetX = 0.f;
    for (int32 i = 0; i < DRAW_COUNT; i++)
    {
        const vector2D pos = Room::GetRandomLocation(bounds, false);
        ASSERT_GE(pos.x, CENTER_X - DEPTH_HALF);
        ASSERT_LE(pos.x, CENTER_X + DEPTH_HALF);
        ASSERT_GE(pos.y, CENTER_Y - WIDTH_HALF) << "width가 x를 정하면 y가 지역 밖으로 나간다";
        ASSERT_LE(pos.y, CENTER_Y + WIDTH_HALF);
        maxOffsetX = max(maxOffsetX, abs(pos.x - CENTER_X));
    }

    // 1000번 뽑으면 x가 width 반폭을 넘는 값이 나와야 한다. 나오지 않으면 depth가 x 범위를 정하지 않는 것이다.
    EXPECT_GT(maxOffsetX, WIDTH_HALF);
}
