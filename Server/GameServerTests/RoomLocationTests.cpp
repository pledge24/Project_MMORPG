#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/Room.h"

/*--------------------------------------------------------------
    룸 무작위 위치 테스트

    몬스터 스폰 위치와 배회 목적지는 룸 경계에서 여백을 뺀 안쪽에서 뽑는다. 경계에
    붙거나 밖으로 밀린 몬스터는 셀 행렬에 들어가지 않아 탐지에서 빠진다.

    픽스처 결합도: Room::Create에 최소한의 룸 데이터를 넘긴다. Start()는 부르지 않으므로
    몬스터 스폰과 틱 타이머는 돌지 않는다. 여백 값은 Room.h의 LOCATION_PADDING_X·Y와 같다.

    축: 언리얼 좌표를 따라 x가 depth, y가 width다. 맵 데이터의 depthHalfExtent가 x 범위를,
    widthHalfExtent가 y 범위를 정한다.
---------------------------------------------------------------*/

namespace
{
    constexpr float CENTER_X = 1000.f;
    constexpr float CENTER_Y = -2000.f;
    constexpr float HALF_EXTENT = 1500.f;
    constexpr float PADDING = 1000.f;
    constexpr int32 DRAW_COUNT = 1000;

    RoomRef MakeRoom(float depthHalfExtent, float widthHalfExtent)
    {
        using namespace JsonProperty::Map;

        Json roomData;
        roomData[string(TemplateId)] = 99;
        roomData[string(CenterPos)][string(PosX)] = CENTER_X;
        roomData[string(CenterPos)][string(PosY)] = CENTER_Y;
        roomData[string(CenterPos)][string(PosZ)] = 0.f;
        roomData[string(DepthHalfExtent)] = depthHalfExtent;
        roomData[string(WidthHalfExtent)] = widthHalfExtent;
        roomData[string(MonsterIds)] = Json::array();
        roomData[string(HasRespawnPoint)] = false;

        return Room::Create(roomData);
    }
}

class RoomLocationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        room = MakeRoom(HALF_EXTENT, HALF_EXTENT);
        ASSERT_NE(room, nullptr);
    }

    RoomRef room;
};

TEST_F(RoomLocationTest, RandomLocationStaysInsidePadding)
{
    const float innerHalf = HALF_EXTENT - PADDING;

    for (int32 i = 0; i < DRAW_COUNT; i++)
    {
        const vector2D pos = room->GetRandomLocation();
        ASSERT_GE(pos.x, CENTER_X - innerHalf) << "여백을 무시하면 몬스터가 룸 경계에 붙어 스폰된다";
        ASSERT_LE(pos.x, CENTER_X + innerHalf);
        ASSERT_GE(pos.y, CENTER_Y - innerHalf);
        ASSERT_LE(pos.y, CENTER_Y + innerHalf);
    }
}

TEST_F(RoomLocationTest, RandomLocationWithoutPaddingUsesWholeRoom)
{
    for (int32 i = 0; i < DRAW_COUNT; i++)
    {
        const vector2D pos = room->GetRandomLocation(false);
        ASSERT_GE(pos.x, CENTER_X - HALF_EXTENT);
        ASSERT_LE(pos.x, CENTER_X + HALF_EXTENT);
        ASSERT_GE(pos.y, CENTER_Y - HALF_EXTENT);
        ASSERT_LE(pos.y, CENTER_Y + HALF_EXTENT);
    }
}

TEST(RoomAxisTest, DepthBoundsXAndWidthBoundsY)
{
    constexpr float DEPTH_HALF = 3000.f;
    constexpr float WIDTH_HALF = 500.f;

    RoomRef room = MakeRoom(DEPTH_HALF, WIDTH_HALF);
    ASSERT_NE(room, nullptr);

    float maxOffsetX = 0.f;
    for (int32 i = 0; i < DRAW_COUNT; i++)
    {
        const vector2D pos = room->GetRandomLocation(false);
        ASSERT_GE(pos.x, CENTER_X - DEPTH_HALF);
        ASSERT_LE(pos.x, CENTER_X + DEPTH_HALF);
        ASSERT_GE(pos.y, CENTER_Y - WIDTH_HALF) << "width가 x를 정하면 y가 룸 밖으로 나간다";
        ASSERT_LE(pos.y, CENTER_Y + WIDTH_HALF);
        maxOffsetX = max(maxOffsetX, abs(pos.x - CENTER_X));
    }

    // 1000번 뽑으면 x가 width 반폭을 넘는 값이 나와야 한다. 나오지 않으면 depth가 x 범위를 정하지 않는 것이다.
    EXPECT_GT(maxOffsetX, WIDTH_HALF);
}
