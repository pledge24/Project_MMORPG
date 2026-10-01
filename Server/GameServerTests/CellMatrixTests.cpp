#include "pch.h"
#include <gtest/gtest.h>
#include "CellMatrix.h"

/*--------------------------------------------------------------
    셀 행렬 테스트

    몬스터는 셀 행렬로 가까운 플레이어를 찾는다. 룸 경계가 셀 크기의 배수에 맞으면 최대 경계 위의
    좌표가 칸 번호를 넘쳐 격자 밖으로 판정됐고, 탐색 상자가 그 경계에 닿은 몬스터는 플레이어를
    찾지 못했다.

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
        cellMatrix.Init(MIN, MAX, MIN, MAX, CELL);
    }

    CellMatrix cellMatrix;
};

TEST_F(CellMatrixTest, QueryReturnsOnlyEntitiesInOverlappingCells)
{
    cellMatrix.Rebuild({ { ENTITY_A, vector2D(500.f, 500.f) }, { ENTITY_B, vector2D(2500.f, 2500.f) } });

    const vector<int64> found = cellMatrix.QueryRange(vector2D(400.f, 400.f), 300.f);

    EXPECT_TRUE(Contains(found, ENTITY_A));
    EXPECT_FALSE(Contains(found, ENTITY_B)) << "탐색 상자에 걸치지 않은 칸의 엔티티는 돌려주지 않는다";
}

TEST_F(CellMatrixTest, CellBoundaryBelongsToUpperCell)
{
    cellMatrix.Rebuild({ { ENTITY_A, vector2D(999.f, 0.f) }, { ENTITY_B, vector2D(1000.f, 0.f) } });

    // 상자 [100, 900]은 첫 칸에만 걸친다.
    const vector<int64> found = cellMatrix.QueryRange(vector2D(500.f, 500.f), 400.f);

    EXPECT_TRUE(Contains(found, ENTITY_A));
    EXPECT_FALSE(Contains(found, ENTITY_B)) << "칸 경계 위의 좌표는 다음 칸에 속한다";
}

TEST_F(CellMatrixTest, EntitiesOnMinAndMaxEdgesAreIndexed)
{
    cellMatrix.Rebuild({ { ENTITY_A, vector2D(MIN, MIN) }, { ENTITY_B, vector2D(MAX, MAX) } });

    EXPECT_TRUE(Contains(cellMatrix.QueryRange(vector2D(MIN, MIN), 100.f), ENTITY_A));
    EXPECT_TRUE(Contains(cellMatrix.QueryRange(vector2D(MAX, MAX), 100.f), ENTITY_B))
        << "최대 경계 위의 좌표를 격자 밖으로 보면 그 플레이어는 탐지되지 않는다";
}

TEST_F(CellMatrixTest, QueryNearEdgeIsClampedIntoMatrix)
{
    cellMatrix.Rebuild({ { ENTITY_A, vector2D(2900.f, 2900.f) }, { ENTITY_B, vector2D(100.f, 100.f) } });

    EXPECT_TRUE(Contains(cellMatrix.QueryRange(vector2D(2900.f, 2900.f), 1500.f), ENTITY_A))
        << "탐색 상자가 격자를 넘으면 넘친 부분만 버리고 안쪽 칸은 본다";
    EXPECT_TRUE(Contains(cellMatrix.QueryRange(vector2D(100.f, 100.f), 1500.f), ENTITY_B));
}

TEST_F(CellMatrixTest, EntitiesOutsideMatrixAreNotIndexed)
{
    cellMatrix.Rebuild({ { ENTITY_A, vector2D(-500.f, 500.f) }, { ENTITY_B, vector2D(500.f, 4500.f) } });

    const vector<int64> found = cellMatrix.QueryRange(vector2D(1500.f, 1500.f), 5000.f);

    EXPECT_TRUE(found.empty());
}

TEST_F(CellMatrixTest, RemovedEntityIsNotReturned)
{
    cellMatrix.Rebuild({ { ENTITY_A, vector2D(500.f, 500.f) }, { ENTITY_B, vector2D(600.f, 600.f) } });

    cellMatrix.Remove(ENTITY_A, vector2D(500.f, 500.f));

    const vector<int64> found = cellMatrix.QueryRange(vector2D(500.f, 500.f), 100.f);
    EXPECT_FALSE(Contains(found, ENTITY_A));
    EXPECT_TRUE(Contains(found, ENTITY_B));
}

TEST_F(CellMatrixTest, RebuildForgetsPreviousPositions)
{
    cellMatrix.Rebuild({ { ENTITY_A, vector2D(500.f, 500.f) } });
    cellMatrix.Rebuild({ { ENTITY_A, vector2D(2500.f, 2500.f) } });

    EXPECT_FALSE(Contains(cellMatrix.QueryRange(vector2D(500.f, 500.f), 100.f), ENTITY_A));
    EXPECT_TRUE(Contains(cellMatrix.QueryRange(vector2D(2500.f, 2500.f), 100.f), ENTITY_A));
}
