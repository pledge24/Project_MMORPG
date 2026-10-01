#pragma once
#include "Utils.h"

/*--------------------------------------------------------------
    CellMatrix

    룸을 일정 크기의 셀로 자른 공간 색인이다. 엔티티 번호와 위치만 알고, 엔티티가 무엇인지는 모른다.
    몬스터가 가까운 플레이어를 찾는 근접 탐색에 쓴다. 어떤 엔티티를 대상으로 삼을지는 룸이 판정한다.
---------------------------------------------------------------*/

class CellMatrix
{
public:
    // 경계를 cellSize의 배수로 바깥쪽에 맞춰 칸을 만든다.
    void Init(float minX, float maxX, float minY, float maxY, float cellSize);

    // 칸을 비우고 다시 채운다. 격자 밖 위치의 엔티티는 넣지 않는다.
    void Rebuild(const vector<pair<int64, vector2D>>& entities);

    // pos가 가리키는 칸에서 엔티티를 뺀다. 격자 밖 위치면 아무것도 하지 않는다.
    void Remove(int64 entityId, const vector2D& pos);

    // 중심에서 가로세로 range 안의 상자에 걸친 칸들의 엔티티 번호를 돌려준다.
    // 상자는 격자 안으로 잘린다. 실제 거리는 호출자가 다시 잰다.
    vector<int64> QueryRange(const vector2D& center, float range) const;

private:
    using Cell = set<int64>;   // 특정 영역에 있는 EntityId

    // 격자 밖이면 nullopt.
    optional<pair<int32, int32>> FindCellIndices(const vector2D& pos) const;

private:
    vector<vector<Cell>> _cells;
    vector2D _offset = vector2D::GetZeroVector();
    float _cellSize = 1.f;
};
