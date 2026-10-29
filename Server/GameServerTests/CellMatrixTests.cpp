#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Game/Room/CellMatrix.h"

/*--------------------------------------------------------------
    셀 행렬 테스트

    룸은 셀 행렬로 두 가지를 정한다. 몬스터가 가까운 플레이어를 찾는 근접 탐색과, 누가 누구를 보는지 정하는 시야다.
    룸 경계가 셀 크기의 배수에 맞으면 최대 경계 위의 좌표가 칸 번호를 넘쳐 격자 밖으로 판정됐고, 탐색 상자가 그
    경계에 닿은 몬스터는 플레이어를 찾지 못했다. 시야에서는 격자 밖의 엔티티가 어느 칸에도 없으면 아무도 그 엔티티를
    보지 못하므로, 격자 밖의 좌표는 가장 가까운 칸에 붙인다.

    픽스처 결합도: 없음. 경계가 셀 크기의 배수에 맞는 3×3 격자를 쓴다.
---------------------------------------------------------------*/

namespace
{
    constexpr float MIN = 0.f;
    constexpr float MAX = 3000.f;
    constexpr float CELL = 1000.f;

    constexpr int64 ENTITY_A = 1;
    constexpr int64 ENTITY_B = 2;

    bool Contains(const vector<int64>& entityIds, int64 entityId)
    {
        return std::find(entityIds.begin(), entityIds.end(), entityId) != entityIds.end();
    }
}

class CellMatrixTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        cellMatrix.Init(PlaneBounds{ MIN, MAX, MIN, MAX }, CELL);
    }

    void PlaceAt(int64 entityId, float x, float y)
    {
        cellMatrix.Place(entityId, cellMatrix.ToCell(vector2D(x, y)));
    }

    CellMatrix cellMatrix;
};

TEST_F(CellMatrixTest, QueryReturnsOnlyEntitiesInOverlappingCells)
{
    PlaceAt(ENTITY_A, 500.f, 500.f);
    PlaceAt(ENTITY_B, 2500.f, 2500.f);

    const vector<int64> found = cellMatrix.QueryRange(vector2D(400.f, 400.f), 300.f);

    EXPECT_TRUE(Contains(found, ENTITY_A));
    EXPECT_FALSE(Contains(found, ENTITY_B)) << "탐색 상자에 걸치지 않은 칸의 엔티티는 돌려주지 않는다";
}

TEST_F(CellMatrixTest, CellBoundaryBelongsToUpperCell)
{
    PlaceAt(ENTITY_A, 999.f, 0.f);
    PlaceAt(ENTITY_B, 1000.f, 0.f);

    // 상자 [100, 900]은 첫 칸에만 걸친다.
    const vector<int64> found = cellMatrix.QueryRange(vector2D(500.f, 500.f), 400.f);

    EXPECT_TRUE(Contains(found, ENTITY_A));
    EXPECT_FALSE(Contains(found, ENTITY_B)) << "칸 경계 위의 좌표는 다음 칸에 속한다";
}

TEST_F(CellMatrixTest, EntitiesOnMinAndMaxEdgesAreIndexed)
{
    PlaceAt(ENTITY_A, MIN, MIN);
    PlaceAt(ENTITY_B, MAX, MAX);

    EXPECT_TRUE(Contains(cellMatrix.QueryRange(vector2D(MIN, MIN), 100.f), ENTITY_A));
    EXPECT_TRUE(Contains(cellMatrix.QueryRange(vector2D(MAX, MAX), 100.f), ENTITY_B))
        << "최대 경계 위의 좌표를 격자 밖으로 보면 그 플레이어는 탐지되지 않는다";
}

TEST_F(CellMatrixTest, QueryNearEdgeIsClampedIntoMatrix)
{
    PlaceAt(ENTITY_A, 2900.f, 2900.f);
    PlaceAt(ENTITY_B, 100.f, 100.f);

    EXPECT_TRUE(Contains(cellMatrix.QueryRange(vector2D(2900.f, 2900.f), 1500.f), ENTITY_A))
        << "탐색 상자가 격자를 넘으면 넘친 부분만 버리고 안쪽 칸은 본다";
    EXPECT_TRUE(Contains(cellMatrix.QueryRange(vector2D(100.f, 100.f), 1500.f), ENTITY_B));
}

TEST_F(CellMatrixTest, PositionsOutsideMatrixSnapToNearestCell)
{
    EXPECT_EQ(cellMatrix.ToCell(vector2D(-500.f, 500.f)), (CellIndex{ 0, 0 }));
    EXPECT_EQ(cellMatrix.ToCell(vector2D(500.f, 4500.f)), (CellIndex{ 0, 2 }))
        << "격자 밖의 엔티티가 어느 칸에도 없으면 아무에게도 보이지 않는다";
}

TEST_F(CellMatrixTest, RemovedEntityIsNotReturned)
{
    PlaceAt(ENTITY_A, 500.f, 500.f);
    PlaceAt(ENTITY_B, 600.f, 600.f);

    cellMatrix.Remove(ENTITY_A);

    const vector<int64> found = cellMatrix.QueryRange(vector2D(500.f, 500.f), 100.f);
    EXPECT_FALSE(Contains(found, ENTITY_A));
    EXPECT_TRUE(Contains(found, ENTITY_B));
    EXPECT_FALSE(cellMatrix.FindCell(ENTITY_A).has_value());
}

TEST_F(CellMatrixTest, PlacingAgainMovesEntityOutOfPreviousCell)
{
    PlaceAt(ENTITY_A, 500.f, 500.f);
    PlaceAt(ENTITY_A, 2500.f, 2500.f);

    EXPECT_FALSE(Contains(cellMatrix.QueryRange(vector2D(500.f, 500.f), 100.f), ENTITY_A));
    EXPECT_TRUE(Contains(cellMatrix.QueryRange(vector2D(2500.f, 2500.f), 100.f), ENTITY_A));
    EXPECT_EQ(cellMatrix.FindCell(ENTITY_A), (CellIndex{ 2, 2 }));
}

TEST_F(CellMatrixTest, NeighborsAreOwnCellAndEightAround)
{
    PlaceAt(ENTITY_A, 1500.f, 1500.f);
    PlaceAt(ENTITY_B, 2500.f, 2500.f);

    EXPECT_TRUE(Contains(cellMatrix.QueryNeighbors(CellIndex{ 0, 0 }), ENTITY_A)) << "대각선 이웃도 보인다";
    EXPECT_FALSE(Contains(cellMatrix.QueryNeighbors(CellIndex{ 0, 0 }), ENTITY_B)) << "두 칸 떨어진 칸은 보이지 않는다";
    EXPECT_TRUE(Contains(cellMatrix.QueryNeighbors(CellIndex{ 2, 2 }), ENTITY_A));
    EXPECT_TRUE(Contains(cellMatrix.QueryNeighbors(CellIndex{ 2, 2 }), ENTITY_B)) << "격자 모서리의 칸도 자기 칸을 본다";
}

TEST(CellIndexTest, NeighborhoodIsSymmetricWithinOneCell)
{
    EXPECT_TRUE(CellMatrix::IsNeighbor(CellIndex{ 1, 1 }, CellIndex{ 1, 1 }));
    EXPECT_TRUE(CellMatrix::IsNeighbor(CellIndex{ 1, 1 }, CellIndex{ 2, 0 }));
    EXPECT_TRUE(CellMatrix::IsNeighbor(CellIndex{ 2, 0 }, CellIndex{ 1, 1 }));
    EXPECT_FALSE(CellMatrix::IsNeighbor(CellIndex{ 0, 0 }, CellIndex{ 2, 1 }));
}
