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
    }
    catch (const TableError& error)
    {
        return error.message;
    }

    return nullopt;
}
