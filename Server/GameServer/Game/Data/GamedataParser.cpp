#include "Core/pch.h"
#include "Game/Data/GamedataParser.h"
#include "Game/Data/JsonProperty.h"

namespace
{
    /** 한 행 안의 오류. 어느 행인지는 행을 도는 쪽이 붙인다. */
    struct RowError
    {
        string cause;
    };

    /** 표 하나를 넘어서는 오류. 메시지에 위치가 이미 들어 있다. */
    struct TableError
    {
        string message;
    };

    /** 기획 원본은 열거형 이름에서 접두사를 뺀 값(GEAR)을 적는다. 이름 표는 protobuf가 만든 것을 쓴다. */
    constexpr string_view ITEM_TYPE_NAME_PREFIX = "ITEM_TYPE_";

    bool IsAbsent(const Json& object, string_view key)
    {
        auto it = object.find(key);
        return it == object.end() || it->is_null();
    }

    /** 필드를 T로 읽는다. 없거나 null이거나 타입이 틀리면 RowError를 던진다. */
    template<typename T>
    T Require(const Json& object, string_view key)
    {
        if (object.is_object() == false)
            throw RowError{ format("'{}'를 담은 값이 객체가 아니다", key) };

        if (IsAbsent(object, key))
            throw RowError{ format("'{}'가 없다", key) };

        const Json& value = object.at(key);
        if constexpr (is_same_v<T, bool>)
        {
            if (value.is_boolean() == false)
                throw RowError{ format("'{}'가 불리언이 아니다: {}", key, value.dump()) };
        }
        else if constexpr (is_integral_v<T>)
        {
            if (value.is_number_integer() == false)
                throw RowError{ format("'{}'가 정수가 아니다: {}", key, value.dump()) };
            // Windows 헤더의 min/max 매크로를 피하려고 괄호로 감싼다.
            if (value.get<int64>() < (numeric_limits<T>::min)() || value.get<int64>() > (numeric_limits<T>::max)())
                throw RowError{ format("'{}'가 범위를 벗어난다: {}", key, value.dump()) };
        }
        else if constexpr (is_floating_point_v<T>)
        {
            if (value.is_number() == false)
                throw RowError{ format("'{}'가 수가 아니다: {}", key, value.dump()) };
        }
        else if constexpr (is_same_v<T, string>)
        {
            if (value.is_string() == false)
                throw RowError{ format("'{}'가 문자열이 아니다: {}", key, value.dump()) };
        }

        return value.get<T>();
    }

    /** 필드가 없거나 null이면 fallback. 있으면 Require와 같이 검사한다. */
    template<typename T>
    T ReadOr(const Json& object, string_view key, T fallback)
    {
        return IsAbsent(object, key) ? fallback : Require<T>(object, key);
    }

    const Json& RequireObject(const Json& object, string_view key)
    {
        if (IsAbsent(object, key) || object.at(key).is_object() == false)
            throw RowError{ format("'{}'가 없거나 객체가 아니다", key) };

        return object.at(key);
    }

    const Json& RequireArray(const Json& object, string_view key)
    {
        if (IsAbsent(object, key) || object.at(key).is_array() == false)
            throw RowError{ format("'{}'가 없거나 배열이 아니다", key) };

        return object.at(key);
    }

    TemplatePos RequirePos(const Json& object)
    {
        using namespace JsonProperty::Map;

        return TemplatePos{ Require<float>(object, PosX), Require<float>(object, PosY), Require<float>(object, PosZ) };
    }

    /** 아이템 종류를 기획 원본의 표기(GEAR)로 적는다. */
    string ItemTypeName(Protocol::ItemType itemType)
    {
        return Protocol::ItemType_Name(itemType).substr(ITEM_TYPE_NAME_PREFIX.size());
    }

    /** 아이템 종류 이름(GEAR)을 읽는다. 아이템 종류가 아니거나 NONE이면 RowError를 던진다. */
    Protocol::ItemType RequireItemType(const Json& object, string_view key)
    {
        const string name = Require<string>(object, key);

        Protocol::ItemType itemType = Protocol::ITEM_TYPE_NONE;
        if (Protocol::ItemType_Parse(string(ITEM_TYPE_NAME_PREFIX) + name, &itemType) == false
            || itemType == Protocol::ITEM_TYPE_NONE)
            throw RowError{ format("'{}' \"{}\"는 아이템 종류가 아니다", key, name) };

        return itemType;
    }

    /** 행 위치를 붙인 메시지. templateId를 읽을 수 있으면 함께 적는다. */
    string Locate(string_view fileName, size_t rowIndex, const Json& row, string_view cause)
    {
        string location = format("{} {}번째 행", fileName, rowIndex + 1);
        if (row.is_object() && row.contains("templateId") && row.at("templateId").is_number_integer())
            location += format("(templateId {})", row.at("templateId").get<int64>());

        return format("{}: {}", location, cause);
    }

    /**
     * 문서의 행마다 parseRow를 부르고, 행에서 난 오류에 위치를 붙여 TableError로 던진다.
     * parseRow는 (행, 행 번호)를 받는다.
     */
    template<typename ParseRow>
    void ForEachRow(string_view fileName, const Json& document, ParseRow parseRow)
    {
        if (document.is_array() == false)
            throw TableError{ format("{}: 최상위 값이 행 배열이 아니다", fileName) };

        for (size_t i = 0; i < document.size(); i++)
        {
            const Json& row = document[i];
            try
            {
                if (row.is_object() == false)
                    throw RowError{ "행이 객체가 아니다" };

                parseRow(row, i);
            }
            catch (const RowError& error)
            {
                throw TableError{ Locate(fileName, i, row, error.cause) };
            }
        }
    }

    /** 같은 templateId가 두 번 나오면 RowError. */
    template<typename Table, typename Template>
    void InsertUnique(Table& table, int32 templateId, Template&& value)
    {
        if (table.emplace(templateId, std::forward<Template>(value)).second == false)
            throw RowError{ format("templateId {}가 중복된다", templateId) };
    }

    //~ 표별 변환

    map<int32, MapTemplate> ParseMaps(string_view fileName, const Json& document)
    {
        using namespace JsonProperty::Map;

        map<int32, MapTemplate> maps;
        ForEachRow(fileName, document, [&](const Json& row, size_t)
            {
                MapTemplate mapTemplate;
                mapTemplate.templateId = Require<int32>(row, TemplateId);
                mapTemplate.mapId = Require<int32>(row, MapId);
                mapTemplate.center = RequirePos(RequireObject(row, CenterPos));
                mapTemplate.depthHalfExtent = Require<float>(row, DepthHalfExtent);
                mapTemplate.widthHalfExtent = Require<float>(row, WidthHalfExtent);

                if (Require<bool>(row, HasRespawnPoint))
                    mapTemplate.respawnPoint = RequirePos(RequireObject(row, RespawnPoint));

                for (const Json& portalRow : RequireArray(RequireObject(row, Portals), Lists))
                {
                    const Json& dst = RequireObject(portalRow, Dst);

                    PortalTemplate portal;
                    portal.portalId = Require<int32>(portalRow, PortalId);
                    portal.srcPos = RequirePos(RequireObject(portalRow, Src));
                    portal.dstRoomId = Require<int32>(dst, TemplateId);
                    portal.dstPos = RequirePos(dst);
                    portal.dstYaw = Require<float>(dst, Yaw);
                    mapTemplate.portals.push_back(portal);
                }

                // 0 이하면 어디서도 포털을 탈 수 없다. 포털이 없는 맵도 같은 열을 가지므로 모든 행에서 본다.
                mapTemplate.portalRadius = Require<float>(row, PortalRadius);
                if (mapTemplate.portalRadius <= 0.f)
                    throw RowError{ format("'portalRadius'가 0보다 커야 한다: {}", mapTemplate.portalRadius) };

                for (const Json& monsterId : RequireArray(row, MonsterIds))
                {
                    if (monsterId.is_number_integer() == false)
                        throw RowError{ format("'monsterIds'에 정수가 아닌 값이 있다: {}", monsterId.dump()) };

                    mapTemplate.monsterIds.push_back(monsterId.get<int32>());
                }

                mapTemplate.maxMonsterCount = ReadOr<int32>(row, MaxMonsterCount, 0);

                InsertUnique(maps, mapTemplate.templateId, std::move(mapTemplate));
            });

        return maps;
    }

    /**
     * DataBuilder가 생성한 표를 생성된 역직렬화(DesignData::Parse<표>)로 읽는다.
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

    InitialProgressTemplate ParseInitialProgress(string_view fileName, const Json& document)
    {
        using namespace JsonProperty::InitialProgress;

        vector<InitialProgressTemplate> rows;
        ForEachRow(fileName, document, [&](const Json& row, size_t)
            {
                InitialProgressTemplate initial;
                initial.startMapId = Require<int32>(row, StartMapId);
                initial.startPos = RequirePos(RequireObject(row, StartPos));
                initial.startYaw = Require<float>(row, StartYaw);

                // 재화 수량은 음수가 될 수 없다. DB도 CHECK 제약으로 막는다.
                initial.startGold = Require<int64>(row, StartGold);
                if (initial.startGold < 0)
                    throw RowError{ format("'startGold'가 0보다 작다: {}", initial.startGold) };

                rows.push_back(initial);
            });

        // 새 캐릭터의 시작 값은 하나로 정해져야 한다.
        if (rows.size() != 1)
            throw TableError{ format("{}: 행이 하나여야 하는데 {}개다", fileName, rows.size()) };

        return rows.front();
    }

    /** 아이템 종류(Protocol::ItemType) → 그 구역의 칸 수. */
    unordered_map<int32, int32> ParseInventorySections(string_view fileName, const Json& document)
    {
        using namespace JsonProperty::InventorySection;

        unordered_map<int32, int32> slotCounts;
        ForEachRow(fileName, document, [&](const Json& row, size_t)
            {
                const Protocol::ItemType itemType = RequireItemType(row, ItemType);

                const int32 slotCount = Require<int32>(row, SlotCount);
                if (slotCount <= 0)
                    throw RowError{ format("'slotCount'가 0보다 커야 한다: {}", slotCount) };

                if (slotCounts.emplace(itemType, slotCount).second == false)
                    throw RowError{ format("아이템 종류 {}의 구역이 중복된다", ItemTypeName(itemType)) };
            });

        // 아이템은 자기 종류의 구역에만 들어간다. 구역이 없는 종류의 아이템은 인벤토리에 넣을 곳이 없다.
        for (int32 value = Protocol::ItemType_MIN; value <= Protocol::ItemType_MAX; value++)
        {
            if (Protocol::ItemType_IsValid(value) == false || value == Protocol::ITEM_TYPE_NONE)
                continue;

            if (slotCounts.contains(value) == false)
                throw TableError{ format("{}: 아이템 종류 {}의 구역이 없다", fileName, ItemTypeName(static_cast<Protocol::ItemType>(value))) };
        }

        return slotCounts;
    }

    int32 ParseQuickSlotCount(string_view fileName, const Json& document)
    {
        using namespace JsonProperty::QuickSlot;

        vector<int32> slotCounts;
        ForEachRow(fileName, document, [&](const Json& row, size_t)
            {
                // DB의 QuickSlot.SlotIndex가 TINYINT라 255번보다 뒤의 퀵슬롯은 저장하지 못한다.
                const int32 slotCount = Require<int32>(row, SlotCount);
                if (slotCount < 1 || slotCount > UINT8_MAX + 1)
                    throw RowError{ format("'slotCount'가 1 이상 {} 이하여야 한다: {}", UINT8_MAX + 1, slotCount) };

                slotCounts.push_back(slotCount);
            });

        // 서버와 클라이언트가 같은 개수를 써야 하므로 값이 하나로 정해져야 한다.
        if (slotCounts.size() != 1)
            throw TableError{ format("{}: 행이 하나여야 하는데 {}개다", fileName, slotCounts.size()) };

        return slotCounts.front();
    }

    //~ 표 사이의 검사

    /** 포털 목적지와 스폰 몬스터가 다른 표에 있는지 본다. */
    void CheckMapReferences(const GamedataTables& tables)
    {
        for (const auto& [mapId, mapTemplate] : tables.maps)
        {
            for (const PortalTemplate& portal : mapTemplate.portals)
            {
                if (tables.maps.contains(portal.dstRoomId) == false)
                    throw TableError{ format("{} templateId {}: 포털 {}의 목적지 룸 {}이 맵 표에 없다",
                        GamedataFile::MAPS, mapId, portal.portalId, portal.dstRoomId) };
            }

            for (int32 monsterId : mapTemplate.monsterIds)
            {
                if (tables.monsters.contains(monsterId) == false)
                    throw TableError{ format("{} templateId {}: 'monsterIds'의 몬스터 {}이 몬스터 표에 없다",
                        GamedataFile::MAPS, mapId, monsterId) };
            }
        }
    }

    /** 리스폰 지점이 있는 맵이 마을이다. 사망한 플레이어가 돌아갈 곳이 하나로 정해져야 한다. */
    int32 FindTownRoomId(const map<int32, MapTemplate>& maps)
    {
        vector<int32> townIds;
        for (const auto& [mapId, mapTemplate] : maps)
        {
            if (mapTemplate.respawnPoint.has_value())
                townIds.push_back(mapId);
        }

        if (townIds.size() != 1)
            throw TableError{ format("{}: 리스폰 지점이 있는 맵(마을)이 하나여야 하는데 {}개다", GamedataFile::MAPS, townIds.size()) };

        return townIds.front();
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

    /** 시작 맵은 룸 번호(templateId)가 아니라 맵 번호(mapId)다. 그 맵의 룸이 맵 표에 있어야 새 캐릭터가 들어간다. */
    void CheckStartMap(const GamedataTables& tables)
    {
        const int32 startMapId = tables.initialProgress.startMapId;
        for (const auto& [roomId, mapTemplate] : tables.maps)
        {
            if (mapTemplate.mapId == startMapId)
                return;
        }

        throw TableError{ format("{}: 'startMapId' {}가 맵 표의 mapId에 없다", GamedataFile::INITIAL_PROGRESS, startMapId) };
    }
}

optional<string> GamedataParser::Parse(const GamedataDocuments& documents, OUT GamedataTables& tables)
{
    try
    {
        tables.maps = ParseMaps(GamedataFile::MAPS, documents.maps);

        tables.currencies = ParseGenerated(documents.currencies, DesignData::ParseCurrency);
        tables.items = ParseGenerated(documents.items, DesignData::ParseItem);
        tables.itemEquipStats = ParseGenerated(documents.itemEquipStats, DesignData::ParseItemEquipStat);
        tables.itemConsumeEffects = ParseGenerated(documents.itemConsumeEffects, DesignData::ParseItemConsumeEffect);
        tables.levels = ParseGenerated(documents.levels, DesignData::ParseLevel);
        tables.classLevelStats = ParseGenerated(documents.classLevelStats, DesignData::ParseClassLevelStat);
        tables.monsters = ParseGenerated(documents.monsters, DesignData::ParseMonster);
        tables.dropGroups = ParseGenerated(documents.dropGroups, DesignData::ParseDropGroup);
        tables.initialProgress = ParseInitialProgress(GamedataFile::INITIAL_PROGRESS, documents.initialProgress);
        tables.inventorySlotCounts = ParseInventorySections(GamedataFile::INVENTORY_SECTIONS, documents.inventorySections);
        tables.quickSlotCount = ParseQuickSlotCount(GamedataFile::QUICK_SLOTS, documents.quickSlots);

        CheckMapReferences(tables);
        tables.townRoomId = FindTownRoomId(tables.maps);
        tables.goldCurrencyId = FindGoldCurrencyId(tables.currencies);
        CheckStartMap(tables);
    }
    catch (const TableError& error)
    {
        return error.message;
    }

    return nullopt;
}
