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

    /** 직업 조건은 소문자 직업 이름(warrior)을 적는다. 이 값이면 모든 직업이 쓴다. */
    constexpr string_view ANY_CLASS = "all";

    /** 착용 조건의 직업 이름 → 직업. */
    const unordered_map<string_view, Protocol::CharacterClass> CLASS_BY_NAME = {
        { "warrior", Protocol::CLASS_TYPE_WARRIOR },
        { "mage", Protocol::CLASS_TYPE_MAGE },
    };

    /** 장비의 세부 종류(itemSubtype) → 착용 부위. */
    const unordered_map<string_view, Protocol::GearType> GEAR_TYPE_BY_SUBTYPE = {
        { JsonProperty::Item::GearSubtype_Helmet, Protocol::GEAR_TYPE_HELMET },
        { JsonProperty::Item::GearSubtype_Chest, Protocol::GEAR_TYPE_CHEST },
        { JsonProperty::Item::GearSubtype_Legs, Protocol::GEAR_TYPE_LEGS },
        { JsonProperty::Item::GearSubtype_Arms, Protocol::GEAR_TYPE_ARMS },
        { JsonProperty::Item::GearSubtype_Boots, Protocol::GEAR_TYPE_BOOTS },
        { JsonProperty::Item::GearSubtype_Sword, Protocol::GEAR_TYPE_WEAPON },
    };

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

    ClassLevelTable ParseLevels(string_view fileName, const Json& document)
    {
        using namespace JsonProperty::LevelTable;

        vector<LevelTemplate> rows;
        ForEachRow(fileName, document, [&](const Json& row, size_t index)
            {
                LevelTemplate level;
                level.level = Require<int32>(row, Level);
                // 레벨 표는 행 번호가 곧 레벨이다. 최대 레벨을 마지막 행에서 읽으므로 빠진 레벨이 없어야 한다.
                if (level.level != static_cast<int32>(index) + 1)
                    throw RowError{ format("'level'이 {}여야 하는데 {}다. 레벨은 1부터 빠짐없이 이어져야 한다", index + 1, level.level) };

                level.maxHp = Require<int32>(row, MaxHp);
                level.maxMp = Require<int32>(row, MaxMp);
                level.physicalAttack = Require<int32>(row, PhysicalAttack);
                level.magicalAttack = Require<int32>(row, MagicalAttack);
                level.expRequirement = Require<int64>(row, ExpRequirement);
                level.maxHpIncrement = Require<int64>(row, MaxHp_Increment);
                level.maxMpIncrement = Require<int64>(row, MaxMp_Increment);
                level.paIncrement = Require<int64>(row, PA_Increment);
                level.maIncrement = Require<int64>(row, MA_Increment);
                rows.push_back(level);
            });

        if (rows.empty())
            throw TableError{ format("{}: 행이 하나도 없다", fileName) };

        return ClassLevelTable(std::move(rows));
    }

    unordered_map<int32, ItemTemplate> ParseItems(string_view fileName, const Json& document)
    {
        using namespace JsonProperty::Item;

        unordered_map<int32, ItemTemplate> items;
        ForEachRow(fileName, document, [&](const Json& row, size_t)
            {
                ItemTemplate item;
                item.templateId = Require<int32>(row, TemplateId);

                const string itemTypeName = Require<string>(row, ItemType);
                if (Protocol::ItemType_Parse(string(ITEM_TYPE_NAME_PREFIX) + itemTypeName, &item.itemType) == false
                    || item.itemType == Protocol::ITEM_TYPE_NONE)
                    throw RowError{ format("'itemType' \"{}\"는 아이템 종류가 아니다", itemTypeName) };

                const string subtype = Require<string>(row, ItemSubtype);
                if (item.itemType == Protocol::ITEM_TYPE_GEAR)
                {
                    auto gearTypeIt = GEAR_TYPE_BY_SUBTYPE.find(subtype);
                    if (gearTypeIt == GEAR_TYPE_BY_SUBTYPE.end())
                        throw RowError{ format("장비의 'itemSubtype' \"{}\"에 맞는 착용 부위가 없다", subtype) };

                    item.gearType = gearTypeIt->second;
                }

                item.levelRequirement = Require<int32>(row, LevelRequirement);

                const string className = Require<string>(row, ClassRequirement);
                if (className != ANY_CLASS)
                {
                    auto classIt = CLASS_BY_NAME.find(className);
                    if (classIt == CLASS_BY_NAME.end())
                        throw RowError{ format("'classRequirement' \"{}\"는 직업이 아니다", className) };

                    item.classRequirement = classIt->second;
                }

                item.buyPrice = Require<int64>(row, BuyPrice);
                item.sellPrice = Require<int64>(row, SellPrice);
                item.sellable = Require<bool>(row, Sellable);

                item.maxStack = Require<int32>(row, MaxStack);
                if (item.maxStack < 1)
                    throw RowError{ format("'maxStack'이 1보다 작다: {}", item.maxStack) };

                // 기획표는 대기가 없는 아이템에 -1을 적는다.
                const double cooldownSeconds = Require<double>(row, Cooldown);
                item.cooldownMs = cooldownSeconds > 0 ? static_cast<uint64>(cooldownSeconds * 1000) : 0;

                item.hp = ReadOr<int32>(row, Hp, 0);
                item.mp = ReadOr<int32>(row, Mp, 0);
                item.physicalAttack = ReadOr<int32>(row, PhysicalAttack, 0);
                item.magicalAttack = ReadOr<int32>(row, MagicalAttack, 0);
                item.hpRestoreRatio = ReadOr<double>(row, HpRestore, 0.0);
                item.mpRestoreRatio = ReadOr<double>(row, MpRestore, 0.0);

                InsertUnique(items, item.templateId, std::move(item));
            });

        return items;
    }

    unordered_map<int32, MonsterTemplate> ParseMonsters(string_view fileName, const Json& document)
    {
        using namespace JsonProperty::Monster;

        unordered_map<int32, MonsterTemplate> monsters;
        ForEachRow(fileName, document, [&](const Json& row, size_t)
            {
                MonsterTemplate monster;
                monster.templateId = Require<int32>(row, TemplateId);
                monster.maxHp = Require<int32>(row, MaxHp);
                monster.baseAttack = Require<int32>(row, BaseAttack);
                monster.movementSpeed = Require<float>(row, MonsterSpeed);
                monster.isTargeting = Require<bool>(row, IsTargeting);

                const Json& expReward = RequireObject(row, ExpReward);
                monster.minExp = Require<int64>(expReward, MinExp);
                monster.maxExp = Require<int64>(expReward, MaxExp);
                if (monster.minExp > monster.maxExp)
                    throw RowError{ format("'minExp' {}가 'maxExp' {}보다 크다", monster.minExp, monster.maxExp) };

                const Json& goldReward = RequireObject(row, GoldReward);
                monster.minGold = Require<int64>(goldReward, MinGold);
                monster.maxGold = Require<int64>(goldReward, MaxGold);
                if (monster.minGold > monster.maxGold)
                    throw RowError{ format("'minGold' {}가 'maxGold' {}보다 크다", monster.minGold, monster.maxGold) };

                monster.attackInterval = Require<float>(row, AttackInterval);
                monster.tryAttackRange = Require<float>(row, TryAttackRange);
                monster.detectionRange = Require<float>(row, DetectionRange);
                monster.chasingMaxRange = Require<float>(row, ChasingMaxRange);

                InsertUnique(monsters, monster.templateId, std::move(monster));
            });

        return monsters;
    }

    map<int32, MapTemplate> ParseMaps(string_view fileName, const Json& document)
    {
        using namespace JsonProperty::Map;

        map<int32, MapTemplate> maps;
        ForEachRow(fileName, document, [&](const Json& row, size_t)
            {
                MapTemplate mapTemplate;
                mapTemplate.templateId = Require<int32>(row, TemplateId);
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
                    portal.dstRoomId = Require<int32>(dst, TemplateId);
                    portal.dstPos = RequirePos(dst);
                    portal.dstYaw = Require<float>(dst, Yaw);
                    mapTemplate.portals.push_back(portal);
                }

                for (const Json& monsterId : RequireArray(row, MonsterIds))
                {
                    if (monsterId.is_number_integer() == false)
                        throw RowError{ format("'monsterIds'에 정수가 아닌 값이 있다: {}", monsterId.dump()) };

                    mapTemplate.monsterIds.push_back(monsterId.get<int32>());
                }

                mapTemplate.maxMonsterCount = ReadOr<int32>(row, MaxMonsterCount, 0);
                mapTemplate.monsterRespawnTime = ReadOr<float>(row, MonsterRespawnTime, 0.f);

                InsertUnique(maps, mapTemplate.templateId, std::move(mapTemplate));
            });

        return maps;
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
}

optional<string> GamedataParser::Parse(const GamedataDocuments& documents, OUT GamedataTables& tables)
{
    try
    {
        tables.classLevelTables[Protocol::CLASS_TYPE_WARRIOR] = ParseLevels(GamedataFile::WARRIOR_LEVELS, documents.warriorLevels);
        tables.items = ParseItems(GamedataFile::ITEMS, documents.items);
        tables.monsters = ParseMonsters(GamedataFile::MONSTERS, documents.monsters);
        tables.maps = ParseMaps(GamedataFile::MAPS, documents.maps);

        CheckMapReferences(tables);
        tables.townRoomId = FindTownRoomId(tables.maps);
    }
    catch (const TableError& error)
    {
        return error.message;
    }

    return nullopt;
}
