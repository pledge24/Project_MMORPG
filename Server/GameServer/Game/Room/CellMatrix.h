#pragma once
#include "Utils/PlaneBounds.h"

/** 셀 행렬의 칸 하나를 가리키는 (x, y) 인덱스. */
struct CellIndex
{
    int32 x = 0;
    int32 y = 0;

    bool operator==(const CellIndex& other) const = default;
};

/**
 * 룸을 같은 크기의 칸으로 나눈 공간 색인.
 *
 * 엔티티마다 든 칸을 기억해, 어느 위치 근처의 엔티티와 어느 칸에서 보이는 엔티티를 룸 전체를 훑지 않고 찾는다.
 * 자기 칸과 이웃 8칸이 시야의 단위다.
 */
class CellMatrix
{
public:
    /** 경계를 cellSize의 배수로 바깥쪽에 맞춰 칸을 만든다. 들어 있던 엔티티는 모두 잊는다. */
    void Init(const PlaneBounds& bounds, float cellSize);

    /** pos가 든 칸. 격자 밖의 좌표는 가장 가까운 칸으로 붙인다. */
    CellIndex ToCell(const vector2D& pos) const;

    /** 엔티티를 cell에 둔다. 다른 칸에 있었으면 그 칸에서 뺀다. */
    void Place(int64 entityId, const CellIndex& cell);
    void Remove(int64 entityId);
    /** 엔티티가 든 칸. 없으면 nullopt. */
    optional<CellIndex> FindCell(int64 entityId) const;

    /** cell과 이웃 8칸의 엔티티 번호. 격자 밖의 이웃은 건너뛴다. */
    vector<int64> QueryNeighbors(const CellIndex& cell) const;
    /**
     * 중심에서 가로세로 range 안의 상자에 걸친 칸들의 엔티티 번호를 돌려준다.
     * 상자는 격자 안으로 잘린다. 실제 거리는 호출자가 다시 잰다.
     */
    vector<int64> QueryRange(const vector2D& center, float range) const;

    /** 두 칸이 같거나 이웃하면 true. 그 두 칸의 엔티티는 서로를 본다. */
    static bool IsNeighbor(const CellIndex& a, const CellIndex& b);

private:
    using Cell = set<int64>;   // 특정 영역에 있는 EntityId

    void AppendCell(int32 indexX, int32 indexY, OUT vector<int64>& entityIds) const;

private:
    vector<vector<Cell>> _cells;
    /** 엔티티 번호 → 든 칸. */
    unordered_map<int64, CellIndex> _entityCells;
    vector2D _offset = vector2D::GetZeroVector();
    float _cellSize = 1.f;
};
