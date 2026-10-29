#pragma once
#include "Game/Data/Templates.h"

/**
 * 테스트가 기획 데이터 표의 지역과 포털 행을 배치 데이터와 함께 넣는다.
 * 지역을 넣으면 그 맵의 배치 데이터도 지역을 덮도록 넓힌다. 실제 표처럼 리스폰 지점은 마을에만 둔다.
 */
namespace TestAreaTables
{
    /**
     * 맵의 셀 크기를 따로 정하지 않은 테스트가 쓰는 셀 크기(cm). 테스트의 맵 전체가 이웃한 셀 안에 들어가므로,
     * 시야를 시험하지 않는 테스트에서는 룸의 모든 플레이어가 서로를 본다.
     */
    constexpr float WIDE_CELL_SIZE = 1000000.f;

    /** 지역 표의 행과 그 지역의 배치 데이터. 포털은 행의 Portals와 portals에 같은 순서로 있다. */
    struct TestArea
    {
        DesignData::AreaRow row;
        AreaPlacement placement;
        optional<RespawnPointPlacement> respawnPoint;
        vector<PortalPlacement> portals;
    };

    /** 맵 하나의 배치 데이터. */
    inline MapPlacement MakeMap(int32 mapId, const PlaneBounds& bounds, float cellSize = WIDE_CELL_SIZE)
    {
        MapPlacement map;
        map.mapId = mapId;
        map.bounds = bounds;
        map.cellSize = cellSize;
        return map;
    }

    /** 맵 mapId 안의 지역. 중심은 (centerX, centerY, 0)이고 X와 Y 방향 반경이 halfExtent다. */
    inline TestArea MakeArea(int32 areaId, int32 mapId, float centerX = 0.f, float centerY = 0.f, float halfExtent = 0.f)
    {
        TestArea area;
        area.row.Id = areaId;
        area.row.MapId = mapId;
        area.placement.id = areaId;
        area.placement.center = vector3D(centerX, centerY, 0.f);
        area.placement.bounds = PlaneBounds::FromCenter(centerX, centerY, halfExtent, halfExtent);
        return area;
    }

    /** 지역에 리스폰 지점을 둔다. */
    inline void SetRespawnPoint(TestArea& area, float x, float y, float z)
    {
        area.row.HasRespawnPoint = true;
        area.respawnPoint = RespawnPointPlacement{ area.row.Id, vector3D(x, y, z), 0.f };
    }

    /** 지역의 (srcX, srcY, 0)에 포털을 놓는다. 도착 위치는 원점이고, 돌려준 배치 데이터로 바꾼다. */
    inline PortalPlacement& AddPortal(TestArea& area, int32 portalId, int32 destAreaId, float srcX, float srcY, float radius)
    {
        DesignData::PortalRow row;
        row.Id = portalId;
        row.DestAreaId = destAreaId;
        area.row.Portals.push_back(row);

        PortalPlacement placement;
        placement.id = portalId;
        placement.src = vector3D(srcX, srcY, 0.f);
        placement.radius = radius;
        area.portals.push_back(placement);
        return area.portals.back();
    }

    /** 지역을 표에 넣고 그 지역의 맵도 맵 표에 넣는다. 맵의 경계는 그 맵의 지역을 모두 덮도록 넓힌다. */
    inline void AddArea(GamedataTables& tables, const TestArea& area)
    {
        tables.areas[area.row.Id] = area.row;
        tables.maps[area.row.MapId].Id = area.row.MapId;
        tables.areaPlacements[area.row.Id] = area.placement;
        if (area.respawnPoint.has_value())
            tables.respawnPoints[area.row.Id] = area.respawnPoint.value();
        for (const PortalPlacement& portal : area.portals)
            tables.portalPlacements[portal.id] = portal;

        const PlaneBounds& areaBounds = area.placement.bounds;
        auto [it, inserted] = tables.mapPlacements.try_emplace(area.row.MapId, MakeMap(area.row.MapId, areaBounds));
        if (inserted == false)
        {
            PlaneBounds& mapBounds = it->second.bounds;
            mapBounds.minX = (std::min)(mapBounds.minX, areaBounds.minX);
            mapBounds.maxX = (std::max)(mapBounds.maxX, areaBounds.maxX);
            mapBounds.minY = (std::min)(mapBounds.minY, areaBounds.minY);
            mapBounds.maxY = (std::max)(mapBounds.maxY, areaBounds.maxY);
        }
    }

    /** 리스폰 지점이 있는 지역을 마을로 넣는다. */
    inline void AddTown(GamedataTables& tables, const TestArea& town)
    {
        AddArea(tables, town);
        tables.townAreaId = town.row.Id;
    }
}
