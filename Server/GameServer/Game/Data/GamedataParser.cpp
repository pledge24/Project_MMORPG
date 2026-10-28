#include "Core/pch.h"
#include "Game/Data/GamedataParser.h"

namespace
{
    /** 표 하나를 넘어서는 오류. 메시지에 위치가 이미 들어 있다. */
    struct TableError
    {
        string message;
    };

    //~ 표별 변환

    /**
     * DataBuilder가 생성한 표나 설정을 생성된 역직렬화(DesignData::Parse<표>)로 읽는다.
     * 기획 데이터의 규칙은 DataBuilder가 이미 검사했으므로 여기서는 형식만 본다.
     */
    template<typename Table>
    Table ParseGenerated(const Json& document, optional<string> (*parse)(const Json&, Table&))
    {
        Table table;
        if (optional<string> error = parse(document, OUT table))
            throw TableError{ error.value() };

        return table;
    }

    //~ 배치 데이터

    vector3D ReadPosition(const Json& object, string_view prefix)
    {
        using DesignData::Detail::Read;
        return vector3D(Read<float>(object, format("{}X", prefix)), Read<float>(object, format("{}Y", prefix)), Read<float>(object, format("{}Z", prefix)));
    }

    PlaneBounds ReadBounds(const Json& object)
    {
        using DesignData::Detail::Read;
        return PlaneBounds::FromCenter(Read<float>(object, "CenterX"), Read<float>(object, "CenterY"),
            Read<float>(object, "HalfExtentX"), Read<float>(object, "HalfExtentY"));
    }

    AreaPlacement ReadAreaPlacement(const Json& object)
    {
        AreaPlacement area;
        area.id = DesignData::Detail::Read<int32>(object, "Id");
        area.center = ReadPosition(object, "Center");
        area.bounds = ReadBounds(object);
        return area;
    }

    RespawnPointPlacement ReadRespawnPointPlacement(const Json& object)
    {
        RespawnPointPlacement respawn;
        respawn.areaId = DesignData::Detail::Read<int32>(object, "AreaId");
        respawn.pos = ReadPosition(object, "");
        respawn.yaw = DesignData::Detail::Read<float>(object, "Yaw");
        return respawn;
    }

    PortalPlacement ReadPortalPlacement(const Json& object)
    {
        PortalPlacement portal;
        portal.id = DesignData::Detail::Read<int32>(object, "Id");
        portal.src = ReadPosition(object, "Src");
        portal.radius = DesignData::Detail::Read<float>(object, "Radius");
        portal.dest = ReadPosition(object, "Dest");
        portal.destYaw = DesignData::Detail::Read<float>(object, "DestYaw");
        return portal;
    }

    /** 행을 번호로 넣는다. 다른 맵의 파일에 같은 번호가 있어도 겹친 것이다. */
    template<typename Placement>
    void AddPlacement(map<int32, Placement>& placements, int32 id, Placement placement, string_view kind)
    {
        if (placements.emplace(id, std::move(placement)).second == false)
            throw DesignData::Detail::FieldError{ format("{} {}이 겹친다", kind, id) };
    }

    /** 포털 표에서 그 포털이 속한 지역의 행을 찾는다. 없으면 nullptr. */
    const DesignData::AreaRow* FindPortalOwner(const DesignData::AreaTable& areas, int32 portalId)
    {
        for (const auto& [areaId, area] : areas)
        {
            for (const DesignData::PortalRow& portal : area.Portals)
            {
                if (portal.Id == portalId)
                    return &area;
            }
        }

        return nullptr;
    }

    /** 배치 데이터의 행이 기획 데이터의 그 맵에 있는 번호를 가리키는지 본다. 위치와 겹침은 추출이 검사했다. */
    void CheckPlacementIds(const MapPlacement& mapPlacement, const vector<AreaPlacement>& areas,
        const vector<RespawnPointPlacement>& respawnPoints, const vector<PortalPlacement>& portals, const GamedataTables& tables)
    {
        using DesignData::Detail::FieldError;
        const int32 mapId = mapPlacement.mapId;
        if (tables.maps.contains(mapId) == false)
            throw FieldError{ format("맵 {}이 맵 표에 없다", mapId) };

        for (const AreaPlacement& area : areas)
        {
            auto row = tables.areas.find(area.id);
            if (row == tables.areas.end())
                throw FieldError{ format("지역 {}이 지역 표에 없다", area.id) };
            if (row->second.MapId != mapId)
                throw FieldError{ format("지역 {}은 지역 표에서 맵 {}에 있다", area.id, row->second.MapId) };
        }

        for (const RespawnPointPlacement& respawn : respawnPoints)
        {
            auto row = tables.areas.find(respawn.areaId);
            if (row == tables.areas.end() || row->second.MapId != mapId)
                throw FieldError{ format("리스폰 지점의 지역 {}이 이 맵의 지역이 아니다", respawn.areaId) };
            if (row->second.HasRespawnPoint == false)
                throw FieldError{ format("지역 {}은 지역 표에서 리스폰 지점이 없는 지역이다", respawn.areaId) };
        }

        for (const PortalPlacement& portal : portals)
        {
            const DesignData::AreaRow* owner = FindPortalOwner(tables.areas, portal.id);
            if (owner == nullptr)
                throw FieldError{ format("포털 {}이 포털 표에 없다", portal.id) };
            if (owner->MapId != mapId)
                throw FieldError{ format("포털 {}은 포털 표에서 맵 {}의 지역 {}에 있다", portal.id, owner->MapId, owner->Id) };
        }
    }

    /** 맵 하나의 배치 데이터를 읽고 번호를 대조해 표에 더한다. 맵 표와 지역 표를 먼저 읽어 둬야 한다. */
    void ParsePlacement(const string& fileName, const Json& document, GamedataTables& tables)
    {
        using DesignData::Detail::Read;
        using DesignData::Detail::ReadRows;

        try
        {
            if (document.is_object() == false)
                throw DesignData::Detail::FieldError{ "객체가 아니다" };

            MapPlacement mapPlacement;
            mapPlacement.mapId = Read<int32>(document, "MapId");
            mapPlacement.bounds = ReadBounds(document);
            mapPlacement.cellSize = Read<float>(document, "CellSize");
            vector<AreaPlacement> areas = ReadRows<AreaPlacement>(document, "Areas", ReadAreaPlacement);
            vector<RespawnPointPlacement> respawnPoints = ReadRows<RespawnPointPlacement>(document, "RespawnPoints", ReadRespawnPointPlacement);
            vector<PortalPlacement> portals = ReadRows<PortalPlacement>(document, "Portals", ReadPortalPlacement);

            CheckPlacementIds(mapPlacement, areas, respawnPoints, portals, tables);

            for (AreaPlacement& area : areas)
                AddPlacement(tables.areaPlacements, area.id, std::move(area), "지역");
            for (RespawnPointPlacement& respawn : respawnPoints)
                AddPlacement(tables.respawnPoints, respawn.areaId, std::move(respawn), "리스폰 지점의 지역");
            for (PortalPlacement& portal : portals)
                AddPlacement(tables.portalPlacements, portal.id, std::move(portal), "포털");
            AddPlacement(tables.mapPlacements, mapPlacement.mapId, std::move(mapPlacement), "맵");
        }
        catch (const DesignData::Detail::FieldError& error)
        {
            throw TableError{ format("{}: {}", fileName, error.cause) };
        }
    }

    /**
     * 기획 데이터의 맵, 지역, 포털이 모두 배치되어 있는지 본다. 마을에는 리스폰 지점이 있어야 한다.
     * 하나라도 빠지면 그 룸이나 포털, 리스폰이 서버에서 깨지므로 부팅을 멈춘다.
     */
    void CheckEveryRowPlaced(const GamedataTables& tables)
    {
        for (const auto& [mapId, map] : tables.maps)
        {
            if (tables.mapPlacements.contains(mapId) == false)
                throw TableError{ format("맵 {}의 배치 데이터가 없다. 레벨에서 추출을 돌린다", mapId) };
        }

        for (const auto& [areaId, area] : tables.areas)
        {
            if (tables.areaPlacements.contains(areaId) == false)
                throw TableError{ format("지역 {}의 배치 데이터가 없다. 맵 {}의 레벨에 지역 볼륨을 둔다", areaId, area.MapId) };
            if (area.HasRespawnPoint && tables.respawnPoints.contains(areaId) == false)
                throw TableError{ format("지역 {}에 리스폰 지점의 배치 데이터가 없다", areaId) };

            for (const DesignData::PortalRow& portal : area.Portals)
            {
                if (tables.portalPlacements.contains(portal.Id) == false)
                    throw TableError{ format("포털 {}의 배치 데이터가 없다. 맵 {}의 레벨에 포털을 둔다", portal.Id, area.MapId) };
            }
        }
    }

    //~ 표 사이의 검사

    /**
     * 리스폰 지점이 있는 지역이 마을이다. 마을이 하나뿐인지는 DataBuilder가 검사하므로 여기서는 세지 않는다.
     * 그래도 마을이 없으면 사망한 플레이어와 범위 밖에서 접속한 플레이어를 보낼 곳이 없으므로 부팅을 멈춘다.
     */
    int32 FindTownAreaId(const DesignData::AreaTable& areas)
    {
        for (const auto& [areaId, area] : areas)
        {
            if (area.HasRespawnPoint)
                return areaId;
        }

        throw TableError{ format("{}: 리스폰 지점이 있는 지역(마을)이 없다", DesignData::AREA_FILE) };
    }

    /**
     * 서버는 재화 종류로 골드를 찾는다. GOLD인 행이 하나뿐인지는 DataBuilder가 검사하므로 여기서는 세지 않는다.
     * 그래도 골드가 없으면 DB에 쓸 재화 번호가 없어 서버가 돌 수 없으므로 부팅을 멈춘다.
     */
    int32 FindGoldCurrencyId(const DesignData::CurrencyTable& currencies)
    {
        for (const auto& [currencyId, currency] : currencies)
        {
            if (currency.Type == DesignData::CurrencyType::GOLD)
                return currencyId;
        }

        throw TableError{ format("{}: 재화 종류가 GOLD인 재화가 없다", DesignData::CURRENCY_FILE) };
    }
}

optional<string> GamedataParser::Parse(const GamedataDocuments& documents, OUT GamedataTables& tables)
{
    try
    {
        tables.currencies = ParseGenerated(documents.currencies, DesignData::ParseCurrency);
        tables.items = ParseGenerated(documents.items, DesignData::ParseItem);
        tables.itemEquipStats = ParseGenerated(documents.itemEquipStats, DesignData::ParseItemEquipStat);
        tables.itemConsumeEffects = ParseGenerated(documents.itemConsumeEffects, DesignData::ParseItemConsumeEffect);
        tables.levels = ParseGenerated(documents.levels, DesignData::ParseLevel);
        tables.classLevelStats = ParseGenerated(documents.classLevelStats, DesignData::ParseClassLevelStat);
        tables.monsters = ParseGenerated(documents.monsters, DesignData::ParseMonster);
        tables.dropGroups = ParseGenerated(documents.dropGroups, DesignData::ParseDropGroup);
        tables.maps = ParseGenerated(documents.maps, DesignData::ParseMap);
        tables.areas = ParseGenerated(documents.areas, DesignData::ParseArea);
        tables.initialProgress = ParseGenerated(documents.initialProgress, DesignData::ParseProgress);
        tables.townAreaId = FindTownAreaId(tables.areas);
        tables.goldCurrencyId = FindGoldCurrencyId(tables.currencies);

        for (const auto& [fileName, document] : documents.placements)
            ParsePlacement(fileName, document, tables);
        CheckEveryRowPlaced(tables);
    }
    catch (const TableError& error)
    {
        return error.message;
    }

    return nullopt;
}
