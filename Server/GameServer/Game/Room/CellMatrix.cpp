#include "pch.h"
#include "CellMatrix.h"

void CellMatrix::Init(float minX, float maxX, float minY, float maxY, float cellSize)
{
    _cellSize = cellSize;

    float snappedMinX = static_cast<float>(std::floor(minX / cellSize)) * cellSize;
    float snappedMaxX = static_cast<float>(std::ceil(maxX / cellSize)) * cellSize;
    float snappedMinY = static_cast<float>(std::floor(minY / cellSize)) * cellSize;
    float snappedMaxY = static_cast<float>(std::ceil(maxY / cellSize)) * cellSize;

    int32 cellCountX = static_cast<int32>((snappedMaxX - snappedMinX) / cellSize);
    int32 cellCountY = static_cast<int32>((snappedMaxY - snappedMinY) / cellSize);

    _cells.assign(cellCountX, vector<Cell>(cellCountY));
    _offset = vector2D(snappedMinX, snappedMinY);
}

void CellMatrix::Rebuild(const vector<pair<int64, vector2D>>& entities)
{
    for (auto& column : _cells)
    {
        for (Cell& cell : column)
            cell.clear();
    }

    for (const auto& [entityId, pos] : entities)
    {
        optional<pair<int32, int32>> indices = FindCellIndices(pos);
        if (indices.has_value() == false)
        {
            wcout << L"유효하지 않은 위치" << '\n';
            continue;
        }

        _cells[indices->first][indices->second].insert(entityId);
    }
}

void CellMatrix::Remove(int64 entityId, const vector2D& pos)
{
    optional<pair<int32, int32>> indices = FindCellIndices(pos);
    if (indices.has_value())
        _cells[indices->first][indices->second].erase(entityId);
}

vector<int64> CellMatrix::QueryRange(const vector2D& center, float range) const
{
    vector<int64> entityIds;

    if (_cells.empty() || _cells[0].empty())
        return entityIds;

    // 상자를 격자 안으로 자른다. 넘친 부분에는 칸이 없다.
    const float maxX = _offset.x + _cellSize * static_cast<float>(_cells.size());
    const float maxY = _offset.y + _cellSize * static_cast<float>(_cells[0].size());
    const vector2D boxMin(std::clamp(center.x - range, _offset.x, maxX), std::clamp(center.y - range, _offset.y, maxY));
    const vector2D boxMax(std::clamp(center.x + range, _offset.x, maxX), std::clamp(center.y + range, _offset.y, maxY));

    optional<pair<int32, int32>> minIndices = FindCellIndices(boxMin);
    optional<pair<int32, int32>> maxIndices = FindCellIndices(boxMax);
    if (minIndices.has_value() == false || maxIndices.has_value() == false)
        return entityIds;

    for (int32 indexX = minIndices->first; indexX <= maxIndices->first; indexX++)
    {
        for (int32 indexY = minIndices->second; indexY <= maxIndices->second; indexY++)
        {
            const Cell& cell = _cells[indexX][indexY];
            entityIds.insert(entityIds.end(), cell.begin(), cell.end());
        }
    }

    return entityIds;
}

optional<pair<int32, int32>> CellMatrix::FindCellIndices(const vector2D& pos) const
{
    if (_cells.empty() || _cells[0].empty())
        return nullopt;

    float offsetX = pos.x - _offset.x;
    float offsetY = pos.y - _offset.y;

    if (offsetX < 0.f || offsetY < 0.f)
        return nullopt;

    const int32 cellCountX = static_cast<int32>(_cells.size());
    const int32 cellCountY = static_cast<int32>(_cells[0].size());
    if (offsetX > _cellSize * cellCountX || offsetY > _cellSize * cellCountY)
        return nullopt;

    // 최대 경계 위의 좌표는 나눗셈 결과가 칸 수와 같아진다. 마지막 칸에 넣는다.
    int32 indexX = (std::min)(static_cast<int32>(offsetX / _cellSize), cellCountX - 1);
    int32 indexY = (std::min)(static_cast<int32>(offsetY / _cellSize), cellCountY - 1);

    return make_pair(indexX, indexY);
}
