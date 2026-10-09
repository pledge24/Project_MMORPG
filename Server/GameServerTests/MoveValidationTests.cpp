#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/MoveValidation.h"

/*--------------------------------------------------------------
    이동 위치 판정 테스트

    서버는 클라이언트가 보낸 위치를 그대로 썼다. 조작한 클라이언트는 룸 안 어디로든 순간이동하고 그 위치로
    저장됐다(TD-045). 허용 거리는 최대 속도 × 경과 시간 × 여유 + 고정 여유다.

    픽스처 결합도: 없음. 순수 함수만 부른다.
---------------------------------------------------------------*/

namespace
{
    const MoveValidation::Bounds BOUNDS{ -5000.f, 5000.f, -5000.f, 5000.f };

    /** elapsedMs 동안 허용하는 거리. */
    float AllowedDistance(uint64 elapsedMs)
    {
        const float seconds = static_cast<float>(elapsedMs) / 1000.f;
        return MoveValidation::PLAYER_MAX_SPEED * seconds * MoveValidation::SPEED_TOLERANCE + MoveValidation::DISTANCE_TOLERANCE;
    }
}

TEST(MoveValidation, MoveWithinAllowedDistanceIsAccepted)
{
    constexpr uint64 ELAPSED_MS = 200;
    const vector2D to{ AllowedDistance(ELAPSED_MS) - 1.f, 0.f };

    EXPECT_EQ(MoveValidation::Validate({ 0.f, 0.f }, to, ELAPSED_MS, BOUNDS), nullopt);
}

TEST(MoveValidation, MoveFartherThanAllowedDistanceIsRejected)
{
    constexpr uint64 ELAPSED_MS = 200;
    const vector2D to{ AllowedDistance(ELAPSED_MS) + 1.f, 0.f };

    EXPECT_NE(MoveValidation::Validate({ 0.f, 0.f }, to, ELAPSED_MS, BOUNDS), nullopt);
}

TEST(MoveValidation, AllowedDistanceGrowsWithElapsedTime)
{
    // 버린 이동 뒤에 벌어진 위치도 시간이 흐르면 다시 받아들인다. 상한을 두면 한 번 벌어진 클라이언트의 이동을 계속 버린다.
    const vector2D to{ 2000.f, 0.f };

    EXPECT_NE(MoveValidation::Validate({ 0.f, 0.f }, to, 1000, BOUNDS), nullopt);
    EXPECT_EQ(MoveValidation::Validate({ 0.f, 0.f }, to, 3000, BOUNDS), nullopt);
}

TEST(MoveValidation, HeightIsNotMeasured)
{
    // 평면 거리만 잰다. 높이는 vector2D에 없다. 대각선은 평면 거리로 잰다.
    const float side = (AllowedDistance(200) - 1.f) / sqrtf(2.f);

    EXPECT_EQ(MoveValidation::Validate({ 0.f, 0.f }, { side, side }, 200, BOUNDS), nullopt);
}

TEST(MoveValidation, PositionOutsideRoomIsRejected)
{
    const vector2D from{ 4990.f, 0.f };

    EXPECT_EQ(MoveValidation::Validate(from, { 5000.f, 0.f }, 200, BOUNDS), nullopt) << "경계 위는 안쪽이다";
    EXPECT_NE(MoveValidation::Validate(from, { 5001.f, 0.f }, 200, BOUNDS), nullopt);
    EXPECT_NE(MoveValidation::Validate({ 0.f, -4990.f }, { 0.f, -5001.f }, 200, BOUNDS), nullopt);
}
