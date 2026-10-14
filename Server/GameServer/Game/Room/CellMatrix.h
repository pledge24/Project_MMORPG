#pragma once

/**
 * 룸을 같은 크기의 칸으로 나눈 공간 색인.
 *
 * 어느 위치 근처에 있는 엔티티를 룸 전체를 훑지 않고 찾는 데 쓴다.
 */
class CellMatrix
{
public:
    /** 경계를 cellSize의 배수로 바깥쪽에 맞춰 칸을 만든다. */
    void Init(float minX, float maxX, float minY, float maxY, float cellSize);

    /** 모든 칸을 비우고 entities의 위치로 다시 채운다. 격자 밖의 엔티티는 넣지 않는다. */
    void Update(const vector<pair<int64, vector2D>>& entities);

    /** pos가 가리키는 칸에서 엔티티를 뺀다. 다른 칸에 들어 있으면 다음 Update까지 남는다. */
    void Remove(int64 entityId, const vector2D& pos);

    /**
     * 중심에서 가로세로 range 안의 상자에 걸친 칸들의 엔티티 번호를 돌려준다.
     * 상자는 격자 안으로 잘린다. 실제 거리는 호출자가 다시 잰다.
     */
    vector<int64> QueryRange(const vector2D& center, float range) const;

private:
    using Cell = set<int64>;   // 특정 영역에 있는 EntityId

    /** pos가 든 칸의 (x, y) 인덱스. 격자 밖이면 nullopt. */
    optional<pair<int32, int32>> FindCellIndices(const vector2D& pos) const;

private:
    vector<vector<Cell>> _cells;
    vector2D _offset = vector2D::GetZeroVector();
    float _cellSize = 1.f;
};
