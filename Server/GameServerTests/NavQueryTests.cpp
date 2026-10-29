#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Navigation/NavQuery.h"
#include "TestNavLevel.h"

/*--------------------------------------------------------------
    지형 질의 테스트

    룸은 맵의 navmesh에 길찾기, 표면을 따른 이동, 직선 통과, 바닥 높이를 묻는다. 위치의 z는 캡슐 중심이다.
    테스트 레벨은 TestNavLevel.h에 있다. 가운데 벽(x -50 ~ 50, y -1000 ~ 400)이 있고 y 400 위쪽으로 돌아갈 수 있다.

    픽스처 결합도: 없음. 테스트 레벨을 빌드해 질의 객체 하나를 만든다.
---------------------------------------------------------------*/

namespace
{
    constexpr float Z = TestNavLevel::STANDING_Z;
    // 에이전트 반경만큼 깎인 navmesh와 복셀 크기를 생각한 오차(cm).
    constexpr float EDGE_TOLERANCE = 20.f;
    // navmesh는 바닥보다 복셀 높이(5cm) 두 칸까지 위에 놓인다.
    constexpr float HEIGHT_ERROR = 15.f;

    NavQuery MakeQuery()
    {
        return NavQuery(TestNavLevel::Load());
    }
}

TEST(NavQueryTest, PathGoesAroundWall)
{
    NavQuery query = MakeQuery();

    const vector<vector3D> path = query.FindPath({ -500.f, -500.f, Z }, { 500.f, -500.f, Z });

    ASSERT_GE(path.size(), 3u) << "벽을 돌아가려면 꺾여야 한다";
    EXPECT_NEAR(path.front().x, -500.f, 1.f);
    EXPECT_NEAR(path.back().x, 500.f, 1.f);
    EXPECT_NEAR(path.back().y, -500.f, 1.f);

    float highestY = -1000.f;
    for (const vector3D& corner : path)
    {
        highestY = max(highestY, corner.y);
        EXPECT_NEAR(corner.z, Z, HEIGHT_ERROR) << "꺾이는 점은 바닥 위 캡슐 중심 높이다";
    }
    EXPECT_GT(highestY, 400.f) << "벽 끝(y 400)을 돌아간다";
}

TEST(NavQueryTest, PathInOpenIsStraight)
{
    NavQuery query = MakeQuery();

    const vector<vector3D> path = query.FindPath({ -500.f, -500.f, Z }, { -500.f, 500.f, Z });

    EXPECT_EQ(path.size(), 2u);
}

TEST(NavQueryTest, PathFromOffMeshIsEmpty)
{
    NavQuery query = MakeQuery();

    EXPECT_TRUE(query.FindPath({ 5000.f, 0.f, Z }, { -500.f, 0.f, Z }).empty());
}

TEST(NavQueryTest, MoveAlongSurfaceStopsAtWall)
{
    NavQuery query = MakeQuery();

    const optional<vector3D> end = query.MoveAlongSurface({ -300.f, -500.f, Z }, { 300.f, -500.f, Z });

    ASSERT_TRUE(end.has_value());
    // 벽의 왼쪽 면(-50)에서 에이전트 반경(42)만큼 떨어진 곳에서 멈춘다.
    EXPECT_NEAR(end->x, -50.f - 42.f, EDGE_TOLERANCE);
    EXPECT_NEAR(end->y, -500.f, 1.f);
    EXPECT_NEAR(end->z, Z, HEIGHT_ERROR);
}

TEST(NavQueryTest, MoveAlongSurfaceInOpenReachesEnd)
{
    NavQuery query = MakeQuery();

    const optional<vector3D> end = query.MoveAlongSurface({ -300.f, -500.f, Z }, { -300.f, 0.f, Z });

    ASSERT_TRUE(end.has_value());
    EXPECT_NEAR(end->x, -300.f, 1.f);
    EXPECT_NEAR(end->y, 0.f, 1.f);
}

TEST(NavQueryTest, RaycastThroughWallIsBlocked)
{
    NavQuery query = MakeQuery();

    const optional<vector3D> hit = query.Raycast({ -300.f, -500.f, Z }, { 300.f, -500.f, Z });

    ASSERT_TRUE(hit.has_value());
    EXPECT_NEAR(hit->x, -50.f - 42.f, EDGE_TOLERANCE);
}

TEST(NavQueryTest, RaycastInOpenIsClear)
{
    NavQuery query = MakeQuery();

    EXPECT_EQ(query.Raycast({ -300.f, -500.f, Z }, { -300.f, 500.f, Z }), nullopt);
    EXPECT_EQ(query.Raycast({ -300.f, 700.f, Z }, { 300.f, 700.f, Z }), nullopt) << "벽 끝 너머(y 700)는 막히지 않는다";
}

TEST(NavQueryTest, FloorHeightFollowsFloorAndRamp)
{
    NavQuery query = MakeQuery();

    const optional<float> floor = query.FindFloorHeight({ -500.f, 0.f, Z });
    ASSERT_TRUE(floor.has_value());
    EXPECT_NEAR(floor.value(), 0.f, HEIGHT_ERROR);

    // 경사로는 x 500에서 0, x 900에서 100까지 오른다. x 700이면 50이다.
    const optional<float> ramp = query.FindFloorHeight({ 700.f, -600.f, 50.f + Z });
    ASSERT_TRUE(ramp.has_value());
    EXPECT_NEAR(ramp.value(), 50.f, HEIGHT_ERROR);
}

TEST(NavQueryTest, FloorHeightOutsideLevelIsMissing)
{
    NavQuery query = MakeQuery();

    EXPECT_EQ(query.FindFloorHeight({ 3000.f, 0.f, Z }), nullopt);
    EXPECT_EQ(query.FindFloorHeight({ -500.f, 0.f, Z + 1000.f }), nullopt) << "캡슐 하나보다 멀리 아래의 면은 그 위치의 바닥이 아니다";
}
