#include "Core/pch.h"
#include "Game/Room/CellMatrix.h"

void CellMatrix::Init(const PlaneBounds& bounds, float cellSize)
{
    _cellSize = cellSize;

    float snappedMinX = static_cast<float>(std::floor(bounds.minX / cellSize)) * cellSize;
    float snappedMaxX = static_cast<float>(std::ceil(bounds.maxX / cellSize)) * cellSize;
    float snappedMinY = static_cast<float>(std::floor(bounds.minY / cellSize)) * cellSize;
    float snappedMaxY = static_cast<float>(std::ceil(bounds.maxY / cellSize)) * cellSize;

    // 경계가 한 점이어도 칸이 하나는 있어야 엔티티를 둘 수 있다.
    int32 cellCountX = (std::max)(1, static_cast<int32>((snappedMaxX - snappedMinX) / cellSize));
    int32 cellCountY = (std::max)(1, static_cast<int32>((snappedMaxY - snappedMinY) / cellSize));

    _cells.assign(cellCountX, vector<Cell>(cellCountY));
    _entityCells.clear();
    _offset = vector2D(snappedMinX, snappedMinY);
}

CellIndex CellMatrix::ToCell(const vector2D& pos) const
{
    const int32 cellCountX = static_cast<int32>(_cells.size());
    const int32 cellCountY = _cells.empty() ? 0 : static_cast<int32>(_cells[0].size());

    // 격자 밖의 엔티티도 어느 칸엔가 있어야 누군가에게 보인다. 이동 판정이 맵 경계 밖을 막으므로 대개 몬스터가 밀려난 경우다.
    // 최대 경계 위의 좌표는 나눗셈 결과가 칸 수와 같아지므로 마지막 칸에 넣는다.
    const int32 indexX = static_cast<int32>(std::floor((pos.x - _offset.x) / _cellSize));
    const int32 indexY = static_cast<int32>(std::floor((pos.y - _offset.y) / _cellSize));

    return CellIndex{ std::clamp(indexX, 0, (std::max)(0, cellCountX - 1)), std::clamp(indexY, 0, (std::max)(0, cellCountY - 1)) };
}

void CellMatrix::Place(int64 entityId, const CellIndex& cell)
{
    if (_cells.empty() || _cells[0].empty())
        return;

    auto it = _entityCells.find(entityId);
    if (it != _entityCells.end())
    {
        if (it->second == cell)
            return;

        _cells[it->second.x][it->second.y].erase(entityId);
        it->second = cell;
    }
    else
    {
        _entityCells.emplace(entityId, cell);
    }

    _cells[cell.x][cell.y].insert(entityId);
}

void CellMatrix::Remove(int64 entityId)
{
    auto it = _entityCells.find(entityId);
    if (it == _entityCells.end())
        return;

    _cells[it->second.x][it->second.y].erase(entityId);
    _entityCells.erase(it);
}

optional<CellIndex> CellMatrix::FindCell(int64 entityId) const
{
    auto it = _entityCells.find(entityId);
    if (it == _entityCells.end())
        return nullopt;

    return it->second;
}

vector<int64> CellMatrix::QueryNeighbors(const CellIndex& cell) const
{
    vector<int64> entityIds;
    if (_cells.empty() || _cells[0].empty())
        return entityIds;

    const int32 cellCountX = static_cast<int32>(_cells.size());
    const int32 cellCountY = static_cast<int32>(_cells[0].size());
    for (int32 indexX = (std::max)(0, cell.x - 1); indexX <= (std::min)(cellCountX - 1, cell.x + 1); indexX++)
    {
        for (int32 indexY = (std::max)(0, cell.y - 1); indexY <= (std::min)(cellCountY - 1, cell.y + 1); indexY++)
            AppendCell(indexX, indexY, OUT entityIds);
    }

    return entityIds;
}

vector<int64> CellMatrix::QueryRange(const vector2D& center, float range) const
{
    vector<int64> entityIds;
    if (_cells.empty() || _cells[0].empty())
        return entityIds;

    // 상자의 두 모서리를 격자 안으로 붙이면 넘친 부분은 버려지고 안쪽 칸만 남는다.
    const CellIndex boxMin = ToCell(vector2D(center.x - range, center.y - range));
    const CellIndex boxMax = ToCell(vector2D(center.x + range, center.y + range));

    for (int32 indexX = boxMin.x; indexX <= boxMax.x; indexX++)
    {
        for (int32 indexY = boxMin.y; indexY <= boxMax.y; indexY++)
            AppendCell(indexX, indexY, OUT entityIds);
    }

    return entityIds;
}

bool CellMatrix::IsNeighbor(const CellIndex& a, const CellIndex& b)
{
    return std::abs(a.x - b.x) <= 1 && std::abs(a.y - b.y) <= 1;
}

void CellMatrix::AppendCell(int32 indexX, int32 indexY, OUT vector<int64>& entityIds) const
{
    const Cell& cell = _cells[indexX][indexY];
    entityIds.insert(entityIds.end(), cell.begin(), cell.end());
}
