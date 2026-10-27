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

    const Json& RequireObject(const Json& object, string_view key)
    {
        if (IsAbsent(object, key) || object.at(key).is_object() == false)
            throw RowError{ format("'{}'가 없거나 객체가 아니다", key) };

        return object.at(key);
    }

    TemplatePos RequirePos(const Json& object)
    {
        using namespace JsonProperty::Pos;

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

    /** 행 위치를 붙인 메시지. */
    string Locate(string_view fileName, size_t rowIndex, string_view cause)
    {
        return format("{} {}번째 행: {}", fileName, rowIndex + 1, cause);
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
                throw TableError{ Locate(fileName, i, error.cause) };
            }
        }
    }

    //~ 표별 변환

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

    /** 시작 맵은 지역 번호가 아니라 맵 번호다. 초기 진행 표는 아직 옛 변환기의 표라서 맵 표를 참조로 검사하지 못하므로 여기서 대조한다. */
    void CheckStartMap(const GamedataTables& tables)
    {
        // TODO: 초기 진행 표를 DataBuilder로 옮기면 시작 맵을 맵 표 참조 열로 바꾸고 이 대조를 지운다.
        const int32 startMapId = tables.initialProgress.startMapId;
        if (tables.maps.contains(startMapId) == false)
            throw TableError{ format("{}: 'startMapId' {}가 맵 표에 없다", GamedataFile::INITIAL_PROGRESS, startMapId) };
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
        tables.initialProgress = ParseInitialProgress(GamedataFile::INITIAL_PROGRESS, documents.initialProgress);
        tables.inventorySlotCounts = ParseInventorySections(GamedataFile::INVENTORY_SECTIONS, documents.inventorySections);
        tables.quickSlotCount = ParseQuickSlotCount(GamedataFile::QUICK_SLOTS, documents.quickSlots);

        tables.townAreaId = FindTownAreaId(tables.areas);
        tables.goldCurrencyId = FindGoldCurrencyId(tables.currencies);
        CheckStartMap(tables);
    }
    catch (const TableError& error)
    {
        return error.message;
    }

    return nullopt;
}
