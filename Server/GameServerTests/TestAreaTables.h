#pragma once
#include "Game/Data/Templates.h"

/**
 * 테스트가 기획 데이터 표의 지역과 포털 행을 배치 데이터와 함께 넣는다.
 * 지금은 지역 하나가 룸 하나라서 지역 번호가 룸 번호다. 실제 표처럼 리스폰 지점은 마을에만 둔다.
 */
namespace TestAreaTables
{
    /** 지역 표의 행과 그 지역의 배치 데이터. 포털은 행의 Portals와 portals에 같은 순서로 있다. */
    struct TestArea
    {
        DesignData::AreaRow row;
        AreaPlacement placement;
        optional<RespawnPointPlacement> respawnPoint;
        vector<PortalPlacement> portals;
    };

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

    /** 지역을 표에 넣고 그 지역의 맵도 맵 표에 넣는다. */
    inline void AddArea(GamedataTables& tables, const TestArea& area)
    {
        tables.areas[area.row.Id] = area.row;
        tables.maps[area.row.MapId].Id = area.row.MapId;
        tables.areaPlacements[area.row.Id] = area.placement;
        if (area.respawnPoint.has_value())
            tables.respawnPoints[area.row.Id] = area.respawnPoint.value();
        for (const PortalPlacement& portal : area.portals)
            tables.portalPlacements[portal.id] = portal;
    }

    /** 리스폰 지점이 있는 지역을 마을로 넣는다. */
    inline void AddTown(GamedataTables& tables, const TestArea& town)
    {
        AddArea(tables, town);
        tables.townAreaId = town.row.Id;
    }
}
