#pragma once

/**
 * Room을 일정 크기의 사각형(Cell)로 자른 공간 색인.
 * - 특정 위치에서 인접한 Entity를 탐색할 때 사용한다. 
 * - Cell은 해당 범위 내에 존재하는 Entity들의 집합으로, 
 * Room::Update -> CellMatrix::Update에 의해 주기적으로 갱신된다.
 */
class CellMatrix
{
public:
    /** 경계를 cellSize의 배수로 바깥쪽에 맞춰 칸을 만든다. */
    void Init(float minX, float maxX, float minY, float maxY, float cellSize);

    /** 모든 Cell들을 갱신한다. */
    void Update(const vector<pair<int64, vector2D>>& entities);

    /** pos가 가리키는 칸에서 엔티티를 제거한다. */
    void Remove(int64 entityId, const vector2D& pos);

    /**
     * 중심에서 가로세로 range 안의 상자에 걸친 칸들의 엔티티 번호를 돌려준다.
     * 상자는 격자 안으로 잘린다. 실제 거리는 호출자가 다시 잰다.
     */
    vector<int64> QueryRange(const vector2D& center, float range) const;

private:
    using Cell = set<int64>;   // 특정 영역에 있는 EntityId

    /** 위치에 해당하는 Cell 위치를 찾는다. 격자 밖이면 nullopt. */
    optional<pair<int32, int32>> FindCellIndices(const vector2D& pos) const;

private:
    vector<vector<Cell>> _cells;
    vector2D _offset = vector2D::GetZeroVector();
    float _cellSize = 1.f;
};
