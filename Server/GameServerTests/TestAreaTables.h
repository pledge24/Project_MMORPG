#pragma once
#include "Game/Data/Templates.h"

/**
 * 테스트가 기획 데이터 표에 지역과 포털 행을 넣는다.
 * 지금은 지역 하나가 룸 하나라서 지역 번호가 룸 번호다. 실제 표처럼 리스폰 지점은 마을에만 둔다.
 */
namespace TestAreaTables
{
    /** 맵 mapId 안의 지역. 중심은 (centerX, centerY, 0)이고 X와 Y 방향 반경이 halfExtent다. */
    inline DesignData::AreaRow MakeArea(int32 areaId, int32 mapId, float centerX = 0.f, float centerY = 0.f, float halfExtent = 0.f)
    {
        DesignData::AreaRow area;
        area.Id = areaId;
        area.MapId = mapId;
        area.CenterX = centerX;
        area.CenterY = centerY;
        area.HalfExtentX = halfExtent;
        area.HalfExtentY = halfExtent;
        return area;
    }

    /** 지역에 리스폰 지점을 둔다. */
    inline void SetRespawnPoint(DesignData::AreaRow& area, float x, float y, float z)
    {
        area.HasRespawnPoint = true;
        area.RespawnX = x;
        area.RespawnY = y;
        area.RespawnZ = z;
    }

    /** (srcX, srcY, 0)에 놓인 포털. 도착 좌표는 원점이다. */
    inline DesignData::PortalRow MakePortal(int32 portalId, int32 destAreaId, float srcX, float srcY, float radius)
    {
        DesignData::PortalRow portal;
        portal.Id = portalId;
        portal.DestAreaId = destAreaId;
        portal.SrcX = srcX;
        portal.SrcY = srcY;
        portal.Radius = radius;
        return portal;
    }

    /** 지역을 표에 넣고 그 지역의 맵도 맵 표에 넣는다. */
    inline void AddArea(GamedataTables& tables, const DesignData::AreaRow& area)
    {
        tables.areas[area.Id] = area;
        tables.maps[area.MapId].Id = area.MapId;
    }

    /** 리스폰 지점이 있는 지역을 마을로 넣는다. */
    inline void AddTown(GamedataTables& tables, const DesignData::AreaRow& town)
    {
        AddArea(tables, town);
        tables.townAreaId = town.Id;
    }
}
