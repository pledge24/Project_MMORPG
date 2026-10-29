#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/MoveValidation.h"
#include "Game/Navigation/NavQuery.h"
#include "TestNavLevel.h"

/*--------------------------------------------------------------
    이동 위치 판정 테스트

    서버는 클라이언트가 보낸 위치를 그대로 썼다. 조작한 클라이언트는 룸 안 어디로든 순간이동하고 그 위치로
    저장됐다. 허용 거리는 최대 속도 × 경과 시간 × 여유 + 고정 여유다.
    지형을 알게 된 뒤로는 벽을 통과한 이동과 바닥에서 뜬 위치도 거절한다. 지형은 TestNavLevel.h의 테스트 레벨이다.

    픽스처 결합도: 없음. 순수 함수와 테스트 레벨의 질의 객체만 쓴다.
---------------------------------------------------------------*/

namespace
{
    const PlaneBounds BOUNDS{ -5000.f, 5000.f, -5000.f, 5000.f };
    constexpr float Z = TestNavLevel::STANDING_Z;

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
    const vector3D to{ AllowedDistance(ELAPSED_MS) - 1.f, 0.f, 0.f };

    EXPECT_EQ(MoveValidation::Validate({ 0.f, 0.f, 0.f }, to, ELAPSED_MS, BOUNDS, nullptr), nullopt);
}

TEST(MoveValidation, MoveFartherThanAllowedDistanceIsRejected)
{
    constexpr uint64 ELAPSED_MS = 200;
    const vector3D to{ AllowedDistance(ELAPSED_MS) + 1.f, 0.f, 0.f };

    EXPECT_NE(MoveValidation::Validate({ 0.f, 0.f, 0.f }, to, ELAPSED_MS, BOUNDS, nullptr), nullopt);
}

TEST(MoveValidation, AllowedDistanceGrowsWithElapsedTime)
{
    // 버린 이동 뒤에 벌어진 위치도 시간이 흐르면 다시 받아들인다. 상한을 두면 한 번 벌어진 클라이언트의 이동을 계속 버린다.
    const vector3D to{ 2000.f, 0.f, 0.f };

    EXPECT_NE(MoveValidation::Validate({ 0.f, 0.f, 0.f }, to, 1000, BOUNDS, nullptr), nullopt);
    EXPECT_EQ(MoveValidation::Validate({ 0.f, 0.f, 0.f }, to, 3000, BOUNDS, nullptr), nullopt);
}

TEST(MoveValidation, SpeedIsMeasuredOnPlane)
{
    // 속도는 평면 거리로 잰다. 대각선은 평면 거리이고, 높이 차이는 거리에 들지 않는다.
    const float side = (AllowedDistance(200) - 1.f) / sqrtf(2.f);

    EXPECT_EQ(MoveValidation::Validate({ 0.f, 0.f, 0.f }, { side, side, 500.f }, 200, BOUNDS, nullptr), nullopt);
}

TEST(MoveValidation, PositionOutsideRoomIsRejected)
{
    const vector3D from{ 4990.f, 0.f, 0.f };

    EXPECT_EQ(MoveValidation::Validate(from, { 5000.f, 0.f, 0.f }, 200, BOUNDS, nullptr), nullopt) << "경계 위는 안쪽이다";
    EXPECT_NE(MoveValidation::Validate(from, { 5001.f, 0.f, 0.f }, 200, BOUNDS, nullptr), nullopt);
    EXPECT_NE(MoveValidation::Validate({ 0.f, -4990.f, 0.f }, { 0.f, -5001.f, 0.f }, 200, BOUNDS, nullptr), nullopt);
}

TEST(MoveValidationTerrain, MoveOnFloorIsAccepted)
{
    NavQuery terrain(TestNavLevel::Load());

    EXPECT_EQ(MoveValidation::Validate({ -500.f, -500.f, Z }, { -500.f, -350.f, Z }, 200, BOUNDS, &terrain), nullopt);
}

TEST(MoveValidationTerrain, MoveThroughWallIsRejected)
{
    // 벽(x -50 ~ 50)을 사이에 둔 두 점. 속도로는 갈 수 있는 거리다.
    NavQuery terrain(TestNavLevel::Load());
    const vector3D from{ -95.f, -500.f, Z };
    const vector3D to{ 95.f, -500.f, Z };
    ASSERT_LT(to.x - from.x, AllowedDistance(200));

    const optional<string> rejection = MoveValidation::Validate(from, to, 200, BOUNDS, &terrain);

    ASSERT_NE(rejection, nullopt);
    EXPECT_NE(rejection->find("벽"), string::npos) << rejection.value();
}

TEST(MoveValidationTerrain, MoveSlidingAlongWallIsAccepted)
{
    // 벽에 붙어 미끄러지는 캡슐의 중심은 반경만큼 깎인 navmesh의 가장자리에 걸친다.
    NavQuery terrain(TestNavLevel::Load());

    EXPECT_EQ(MoveValidation::Validate({ -92.f, -500.f, Z }, { -92.f, -350.f, Z }, 200, BOUNDS, &terrain), nullopt);
}

TEST(MoveValidationTerrain, FloatingPositionIsRejected)
{
    NavQuery terrain(TestNavLevel::Load());

    const optional<string> rejection = MoveValidation::Validate({ -500.f, -500.f, Z }, { -500.f, -400.f, Z + 300.f }, 200, BOUNDS, &terrain);

    ASSERT_NE(rejection, nullopt);
    EXPECT_NE(rejection->find("바닥"), string::npos) << rejection.value();
}

TEST(MoveValidationTerrain, PositionSunkBelowFloorIsRejected)
{
    NavQuery terrain(TestNavLevel::Load());

    EXPECT_NE(MoveValidation::Validate({ -500.f, -500.f, Z }, { -500.f, -400.f, Z - 150.f }, 200, BOUNDS, &terrain), nullopt);
}

TEST(MoveValidationTerrain, PositionWithoutFloorIsRejected)
{
    // 바닥(x 1000까지) 밖으로 나간 위치. 룸 경계 안이고 속도로도 갈 수 있다.
    NavQuery terrain(TestNavLevel::Load());

    EXPECT_NE(MoveValidation::Validate({ 960.f, 0.f, Z }, { 1100.f, 0.f, Z }, 200, BOUNDS, &terrain), nullopt);
}

TEST(MoveValidationTerrain, MoveUpRampIsAccepted)
{
    // 경사로의 x 600에서 높이는 25, x 700에서 50이다. 캡슐 중심은 그만큼 오른다.
    NavQuery terrain(TestNavLevel::Load());

    EXPECT_EQ(MoveValidation::Validate({ 600.f, -600.f, 25.f + Z }, { 700.f, -600.f, 50.f + Z }, 200, BOUNDS, &terrain), nullopt);
    EXPECT_EQ(MoveValidation::Validate({ 700.f, -600.f, 50.f + Z }, { 600.f, -600.f, 25.f + Z }, 200, BOUNDS, &terrain), nullopt);
}
