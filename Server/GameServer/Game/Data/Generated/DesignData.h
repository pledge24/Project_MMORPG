// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once
#include "Protocol/Common.pb.h"
#include "Protocol/Item.pb.h"

/** DataBuilder가 생성한 기획 데이터의 타입과 역직렬화. */
namespace DesignData
{
    /** 원본 해시로 만든 데이터 버전을 담은 파일. */
    constexpr string_view VERSION_FILE = "DataVersion.json";
    constexpr string_view CURRENCY_FILE = "Currency.json";
    constexpr string_view ITEM_FILE = "Item.json";
    constexpr string_view ITEM_EQUIP_STAT_FILE = "ItemEquipStat.json";
    constexpr string_view ITEM_CONSUME_EFFECT_FILE = "ItemConsumeEffect.json";
    constexpr string_view LEVEL_FILE = "Level.json";
    constexpr string_view CLASS_LEVEL_STAT_FILE = "ClassLevelStat.json";
    constexpr string_view MONSTER_FILE = "Monster.json";
    constexpr string_view DROP_GROUP_FILE = "DropGroup.json";
    constexpr string_view MAP_FILE = "Map.json";
    constexpr string_view AREA_FILE = "Area.json";
    constexpr string_view PROGRESS_FILE = "Progress.json";

    //~ 열거형

    /** 셀에 적는 값 이름(WARRIOR, MAGE)을 Protocol::CharacterClass로 읽는다. 모르는 이름이면 false. */
    inline bool ParseEnumName(string_view name, OUT Protocol::CharacterClass& value)
    {
        static const unordered_map<string_view, Protocol::CharacterClass> VALUES = {
            { "WARRIOR", Protocol::CHARACTER_CLASS_WARRIOR },
            { "MAGE", Protocol::CHARACTER_CLASS_MAGE },
        };

        auto it = VALUES.find(name);
        if (it == VALUES.end())
            return false;

        value = it->second;
        return true;
    }

    enum class CurrencyType : int32
    {
        GOLD = 1,
    };

    /** 셀에 적는 값 이름(GOLD)을 CurrencyType로 읽는다. 모르는 이름이면 false. */
    inline bool ParseEnumName(string_view name, OUT CurrencyType& value)
    {
        static const unordered_map<string_view, CurrencyType> VALUES = {
            { "GOLD", CurrencyType::GOLD },
        };

        auto it = VALUES.find(name);
        if (it == VALUES.end())
            return false;

        value = it->second;
        return true;
    }

    enum class DropMethod : int32
    {
        INDEPENDENT = 1,
        WEIGHTED = 2,
    };

    /** 셀에 적는 값 이름(INDEPENDENT, WEIGHTED)을 DropMethod로 읽는다. 모르는 이름이면 false. */
    inline bool ParseEnumName(string_view name, OUT DropMethod& value)
    {
        static const unordered_map<string_view, DropMethod> VALUES = {
            { "INDEPENDENT", DropMethod::INDEPENDENT },
            { "WEIGHTED", DropMethod::WEIGHTED },
        };

        auto it = VALUES.find(name);
        if (it == VALUES.end())
            return false;

        value = it->second;
        return true;
    }

    /** 셀에 적는 값 이름(HELMET, CHEST, LEGS, ARMS, BOOTS, WEAPON)을 Protocol::GearPart로 읽는다. 모르는 이름이면 false. */
    inline bool ParseEnumName(string_view name, OUT Protocol::GearPart& value)
    {
        static const unordered_map<string_view, Protocol::GearPart> VALUES = {
            { "HELMET", Protocol::GEAR_PART_HELMET },
            { "CHEST", Protocol::GEAR_PART_CHEST },
            { "LEGS", Protocol::GEAR_PART_LEGS },
            { "ARMS", Protocol::GEAR_PART_ARMS },
            { "BOOTS", Protocol::GEAR_PART_BOOTS },
            { "WEAPON", Protocol::GEAR_PART_WEAPON },
        };

        auto it = VALUES.find(name);
        if (it == VALUES.end())
            return false;

        value = it->second;
        return true;
    }

    /** 셀에 적는 값 이름(GEAR, CONSUMABLE, MISCELLANEOUS)을 Protocol::ItemType로 읽는다. 모르는 이름이면 false. */
    inline bool ParseEnumName(string_view name, OUT Protocol::ItemType& value)
    {
        static const unordered_map<string_view, Protocol::ItemType> VALUES = {
            { "GEAR", Protocol::ITEM_TYPE_GEAR },
            { "CONSUMABLE", Protocol::ITEM_TYPE_CONSUMABLE },
            { "MISCELLANEOUS", Protocol::ITEM_TYPE_MISCELLANEOUS },
        };

        auto it = VALUES.find(name);
        if (it == VALUES.end())
            return false;

        value = it->second;
        return true;
    }

    enum class PortalType : int32
    {
        AREA = 1,
    };

    /** 셀에 적는 값 이름(AREA)을 PortalType로 읽는다. 모르는 이름이면 false. */
    inline bool ParseEnumName(string_view name, OUT PortalType& value)
    {
        static const unordered_map<string_view, PortalType> VALUES = {
            { "AREA", PortalType::AREA },
        };

        auto it = VALUES.find(name);
        if (it == VALUES.end())
            return false;

        value = it->second;
        return true;
    }

    //~ 역직렬화 공통
    namespace Detail
    {
        /** 행 하나 안의 형식 오류. 어느 행인지는 행을 도는 쪽이 붙인다. */
        struct FieldError
        {
            string cause;
        };

        template<typename T> struct IsVector : false_type {};
        template<typename T> struct IsVector<vector<T>> : true_type {};
        template<typename T> struct IsOptional : false_type {};
        template<typename T> struct IsOptional<optional<T>> : true_type {};

        /** 값 하나를 T로 읽는다. 타입이 틀리면 FieldError를 던진다. */
        template<typename T>
        T ReadValue(const Json& value, string_view key)
        {
            if constexpr (IsVector<T>::value)
            {
                if (value.is_array() == false)
                    throw FieldError{ format("'{}'가 배열이 아니다: {}", key, value.dump()) };

                T items;
                for (const Json& item : value)
                    items.push_back(ReadValue<typename T::value_type>(item, key));
                return items;
            }
            else if constexpr (IsOptional<T>::value)
            {
                if (value.is_null())
                    return nullopt;
                return ReadValue<typename T::value_type>(value, key);
            }
            else if constexpr (is_enum_v<T>)
            {
                // 열거형마다 위에 생성한 ParseEnumName이 값 이름을 읽는다.
                T result{};
                if (value.is_string() == false || ParseEnumName(value.get_ref<const string&>(), OUT result) == false)
                    throw FieldError{ format("'{}' {}는 열거형 값이 아니다", key, value.dump()) };
                return result;
            }
            else if constexpr (is_same_v<T, bool>)
            {
                if (value.is_boolean() == false)
                    throw FieldError{ format("'{}'가 불이 아니다: {}", key, value.dump()) };
                return value.get<bool>();
            }
            else if constexpr (is_integral_v<T>)
            {
                if (value.is_number_integer() == false)
                    throw FieldError{ format("'{}'가 정수가 아니다: {}", key, value.dump()) };
                // Windows 헤더의 min/max 매크로를 피하려고 괄호로 감싼다.
                if (value.get<int64>() < (numeric_limits<T>::min)() || value.get<int64>() > (numeric_limits<T>::max)())
                    throw FieldError{ format("'{}'가 범위를 벗어난다: {}", key, value.dump()) };
                return value.get<T>();
            }
            else if constexpr (is_floating_point_v<T>)
            {
                if (value.is_number() == false)
                    throw FieldError{ format("'{}'가 수가 아니다: {}", key, value.dump()) };
                return value.get<T>();
            }
            else
            {
                static_assert(is_same_v<T, string>, "생성된 필드의 타입은 정수, 실수, 문자열, 불, 열거형과 그 목록, 선택 값이다");
                if (value.is_string() == false)
                    throw FieldError{ format("'{}'가 문자열이 아니다: {}", key, value.dump()) };
                return value.get<string>();
            }
        }

        /** 객체의 필드를 T로 읽는다. 없거나 타입이 틀리면 FieldError를 던진다. */
        template<typename T>
        T Read(const Json& object, string_view key)
        {
            auto it = object.find(key);
            if (it == object.end())
                throw FieldError{ format("'{}'가 없다", key) };
            return ReadValue<T>(*it, key);
        }

        /** 객체의 배열 필드를 원소마다 readRow로 읽는다. 중첩한 표의 행에 쓴다. 틀린 원소가 있으면 몇 번째인지 붙여 FieldError를 던진다. */
        template<typename Row, typename ReadRow>
        vector<Row> ReadRows(const Json& object, string_view key, ReadRow readRow)
        {
            auto it = object.find(key);
            if (it == object.end())
                throw FieldError{ format("'{}'가 없다", key) };
            if (it->is_array() == false)
                throw FieldError{ format("'{}'가 배열이 아니다: {}", key, it->dump()) };

            vector<Row> rows;
            for (size_t index = 0; index < it->size(); ++index)
            {
                try
                {
                    const Json& item = (*it)[index];
                    if (item.is_object() == false)
                        throw FieldError{ "객체가 아니다" };

                    rows.push_back(readRow(item));
                }
                catch (const FieldError& error)
                {
                    throw FieldError{ format("'{}' {}번째 원소: {}", key, index + 1, error.cause) };
                }
            }

            return rows;
        }

        /** 행 배열을 키 → 행의 맵으로 읽는다. 실패하면 "파일 N번째 행: 사유". */
        template<typename Table, typename ReadRow>
        optional<string> ParseTable(string_view fileName, const Json& document, OUT Table& table, ReadRow readRow)
        {
            if (document.is_array() == false)
                return format("{}: 행 배열이 아니다", fileName);

            for (size_t index = 0; index < document.size(); ++index)
            {
                try
                {
                    const Json& object = document[index];
                    if (object.is_object() == false)
                        throw FieldError{ "행이 객체가 아니다" };

                    auto [key, row] = readRow(object);
                    if (table.emplace(std::move(key), std::move(row)).second == false)
                        throw FieldError{ "키가 겹친다" };
                }
                catch (const FieldError& error)
                {
                    return format("{} {}번째 행: {}", fileName, index + 1, error.cause);
                }
            }

            return nullopt;
        }

        /** 객체 하나를 읽는다. 실패하면 "파일: 사유". */
        template<typename ReadFields>
        optional<string> ParseObject(string_view fileName, const Json& document, ReadFields readFields)
        {
            if (document.is_object() == false)
                return format("{}: 객체가 아니다", fileName);

            try
            {
                readFields(document);
            }
            catch (const FieldError& error)
            {
                return format("{}: {}", fileName, error.cause);
            }

            return nullopt;
        }
    }

    /** 데이터 버전 파일을 읽는다. 실패하면 사유를 돌려준다. */
    inline optional<string> ParseDataVersion(const Json& document, OUT string& version)
    {
        return Detail::ParseObject(VERSION_FILE, document, [&](const Json& object)
        {
            version = Detail::Read<string>(object, "Version");
        });
    }

    //~ 표

    /** DropEntry 표(DropEntry.xlsx)의 한 행. DropGroup 행의 Entries 아래에 키 순서로 실린다. */
    struct DropEntryRow
    {
        /** 나오는 아이템 */
        int32 ItemId = 0;
        /** INDEPENDENT 그룹에서 이 항목이 나올 확률. 0 초과 1 이하다. WEIGHTED 그룹은 비워 둔다 */
        optional<float> Rate;
        /** WEIGHTED 그룹에서 이 항목이 뽑힐 가중치. 1 이상이다. INDEPENDENT 그룹은 비워 둔다 */
        optional<int32> Weight;
        /** 나오는 수량의 최솟값. 1 이상이고 MaxCount보다 크지 않다 */
        int32 MinCount = 0;
        /** 나오는 수량의 최댓값 */
        int32 MaxCount = 0;
    };

    /** DropGroup 행의 Entries 원소 하나를 읽는다. 틀리면 Detail::FieldError를 던진다. */
    inline DropEntryRow ReadDropEntryRow(const Json& object)
    {
        DropEntryRow row;
        row.ItemId = Detail::Read<int32>(object, "ItemId");
        row.Rate = Detail::Read<optional<float>>(object, "Rate");
        row.Weight = Detail::Read<optional<int32>>(object, "Weight");
        row.MinCount = Detail::Read<int32>(object, "MinCount");
        row.MaxCount = Detail::Read<int32>(object, "MaxCount");
        return row;
    }

    /** Portal 표(Portal.xlsx)의 한 행. Area 행의 Portals 아래에 키 순서로 실린다. */
    struct PortalRow
    {
        /** 포털 번호. 클라이언트가 포털 이동을 요청할 때 보낸다 */
        int32 Id = 0;
        /** 포털 유형. 지금은 지역 사이를 잇는 AREA 하나다 */
        PortalType Type = {};
        /** 포털 위치의 X 좌표 */
        float SrcX = 0.f;
        /** 포털 위치의 Y 좌표 */
        float SrcY = 0.f;
        /** 포털 위치의 Z 좌표 */
        float SrcZ = 0.f;
        /** 포털을 탈 수 있는 평면 거리. 0보다 크다 */
        float Radius = 0.f;
        /** 도착 지역 */
        int32 DestAreaId = 0;
        /** 도착 위치의 X 좌표 */
        float DestX = 0.f;
        /** 도착 위치의 Y 좌표 */
        float DestY = 0.f;
        /** 도착 위치의 Z 좌표 */
        float DestZ = 0.f;
        /** 도착한 뒤의 방향(도) */
        float DestYaw = 0.f;
    };

    /** Area 행의 Portals 원소 하나를 읽는다. 틀리면 Detail::FieldError를 던진다. */
    inline PortalRow ReadPortalRow(const Json& object)
    {
        PortalRow row;
        row.Id = Detail::Read<int32>(object, "Id");
        row.Type = Detail::Read<PortalType>(object, "Type");
        row.SrcX = Detail::Read<float>(object, "SrcX");
        row.SrcY = Detail::Read<float>(object, "SrcY");
        row.SrcZ = Detail::Read<float>(object, "SrcZ");
        row.Radius = Detail::Read<float>(object, "Radius");
        row.DestAreaId = Detail::Read<int32>(object, "DestAreaId");
        row.DestX = Detail::Read<float>(object, "DestX");
        row.DestY = Detail::Read<float>(object, "DestY");
        row.DestZ = Detail::Read<float>(object, "DestZ");
        row.DestYaw = Detail::Read<float>(object, "DestYaw");
        return row;
    }

    /** Currency 표(Currency.xlsx)의 한 행. */
    struct CurrencyRow
    {
        /** 재화 번호. DB의 CurrencyId로 쓴다 */
        int32 Id = 0;
        /** 재화 이름 */
        string Name;
        /** 재화 종류. 서버가 종류로 골드를 찾는다. GOLD인 행은 하나뿐이다 */
        CurrencyType Type = {};
    };

    /** Id → 행. 키 순서로 돈다. */
    using CurrencyTable = map<int32, CurrencyRow>;

    /** Currency.json을 읽는다. 실패하면 사유를 돌려주고, table에는 일부만 채워졌을 수 있다. */
    inline optional<string> ParseCurrency(const Json& document, OUT CurrencyTable& table)
    {
        return Detail::ParseTable(CURRENCY_FILE, document, OUT table, [](const Json& object)
        {
            CurrencyRow row;
            row.Id = Detail::Read<int32>(object, "Id");
            row.Name = Detail::Read<string>(object, "Name");
            row.Type = Detail::Read<CurrencyType>(object, "Type");

            auto key = row.Id;
            return pair{ std::move(key), std::move(row) };
        });
    }

    /** Item 표(Item.xlsx)의 한 행. */
    struct ItemRow
    {
        /** 아이템 번호. 장비 1000번대, 소모품 2000번대, 기타 3000번대다 */
        int32 Id = 0;
        /** 아이템 종류. 장비는 ItemEquipStat에, 소모품은 ItemConsumeEffect에 행이 있다 */
        Protocol::ItemType Type = {};
        /** 착용할 수 있는 최소 레벨 */
        int32 LevelRequirement = 0;
        /** 착용할 수 있는 직업. 비어 있으면 모든 직업이다 */
        vector<Protocol::CharacterClass> AllowedClasses;
        /** 상점 구매 가격(골드) */
        int64 BuyPrice = 0;
        /** 상점 판매 가격(골드) */
        int64 SellPrice = 0;
        /** 상점에 팔 수 있는지 */
        bool Sellable = false;
        /** 한 칸에 쌓을 수 있는 최대 개수. 1 이상이다 */
        int32 MaxStack = 0;
    };

    /** Id → 행. 키 순서로 돈다. */
    using ItemTable = map<int32, ItemRow>;

    /** Item.json을 읽는다. 실패하면 사유를 돌려주고, table에는 일부만 채워졌을 수 있다. */
    inline optional<string> ParseItem(const Json& document, OUT ItemTable& table)
    {
        return Detail::ParseTable(ITEM_FILE, document, OUT table, [](const Json& object)
        {
            ItemRow row;
            row.Id = Detail::Read<int32>(object, "Id");
            row.Type = Detail::Read<Protocol::ItemType>(object, "Type");
            row.LevelRequirement = Detail::Read<int32>(object, "LevelRequirement");
            row.AllowedClasses = Detail::Read<vector<Protocol::CharacterClass>>(object, "AllowedClasses");
            row.BuyPrice = Detail::Read<int64>(object, "BuyPrice");
            row.SellPrice = Detail::Read<int64>(object, "SellPrice");
            row.Sellable = Detail::Read<bool>(object, "Sellable");
            row.MaxStack = Detail::Read<int32>(object, "MaxStack");

            auto key = row.Id;
            return pair{ std::move(key), std::move(row) };
        });
    }

    /** ItemEquipStat 표(ItemEquipStat.xlsx)의 한 행. */
    struct ItemEquipStatRow
    {
        /** 장비 아이템 번호 */
        int32 ItemId = 0;
        /** 착용 부위 */
        Protocol::GearPart Part = {};
        /** 착용하면 오르는 최대 HP */
        int32 Hp = 0;
        /** 착용하면 오르는 최대 MP */
        int32 Mp = 0;
        /** 착용하면 오르는 물리 공격력 */
        int32 PhysicalAttack = 0;
        /** 착용하면 오르는 마법 공격력 */
        int32 MagicalAttack = 0;
    };

    /** ItemId → 행. 키 순서로 돈다. */
    using ItemEquipStatTable = map<int32, ItemEquipStatRow>;

    /** ItemEquipStat.json을 읽는다. 실패하면 사유를 돌려주고, table에는 일부만 채워졌을 수 있다. */
    inline optional<string> ParseItemEquipStat(const Json& document, OUT ItemEquipStatTable& table)
    {
        return Detail::ParseTable(ITEM_EQUIP_STAT_FILE, document, OUT table, [](const Json& object)
        {
            ItemEquipStatRow row;
            row.ItemId = Detail::Read<int32>(object, "ItemId");
            row.Part = Detail::Read<Protocol::GearPart>(object, "Part");
            row.Hp = Detail::Read<int32>(object, "Hp");
            row.Mp = Detail::Read<int32>(object, "Mp");
            row.PhysicalAttack = Detail::Read<int32>(object, "PhysicalAttack");
            row.MagicalAttack = Detail::Read<int32>(object, "MagicalAttack");

            auto key = row.ItemId;
            return pair{ std::move(key), std::move(row) };
        });
    }

    /** ItemConsumeEffect 표(ItemConsumeEffect.xlsx)의 한 행. */
    struct ItemConsumeEffectRow
    {
        /** 소모품 아이템 번호 */
        int32 ItemId = 0;
        /** 회복하는 HP의 비율. 최대 HP 기준이다 */
        float HpRestoreRatio = 0.f;
        /** 회복하는 MP의 비율. 최대 MP 기준이다 */
        float MpRestoreRatio = 0.f;
        /** 같은 아이템을 다시 쓰기까지의 대기(ms). 0이면 대기가 없다 */
        int32 CooldownMs = 0;
    };

    /** ItemId → 행. 키 순서로 돈다. */
    using ItemConsumeEffectTable = map<int32, ItemConsumeEffectRow>;

    /** ItemConsumeEffect.json을 읽는다. 실패하면 사유를 돌려주고, table에는 일부만 채워졌을 수 있다. */
    inline optional<string> ParseItemConsumeEffect(const Json& document, OUT ItemConsumeEffectTable& table)
    {
        return Detail::ParseTable(ITEM_CONSUME_EFFECT_FILE, document, OUT table, [](const Json& object)
        {
            ItemConsumeEffectRow row;
            row.ItemId = Detail::Read<int32>(object, "ItemId");
            row.HpRestoreRatio = Detail::Read<float>(object, "HpRestoreRatio");
            row.MpRestoreRatio = Detail::Read<float>(object, "MpRestoreRatio");
            row.CooldownMs = Detail::Read<int32>(object, "CooldownMs");

            auto key = row.ItemId;
            return pair{ std::move(key), std::move(row) };
        });
    }

    /** Level 표(Level.xlsx)의 한 행. */
    struct LevelRow
    {
        /** 레벨 */
        int32 Level = 0;
        /** 이 레벨에서 다음 레벨로 가는 데 필요한 경험치. 마지막 행이 최대 레벨이고 0이다 */
        int64 ExpRequirement = 0;
    };

    /** Level → 행. 키 순서로 돈다. */
    using LevelTable = map<int32, LevelRow>;

    /** Level.json을 읽는다. 실패하면 사유를 돌려주고, table에는 일부만 채워졌을 수 있다. */
    inline optional<string> ParseLevel(const Json& document, OUT LevelTable& table)
    {
        return Detail::ParseTable(LEVEL_FILE, document, OUT table, [](const Json& object)
        {
            LevelRow row;
            row.Level = Detail::Read<int32>(object, "Level");
            row.ExpRequirement = Detail::Read<int64>(object, "ExpRequirement");

            auto key = row.Level;
            return pair{ std::move(key), std::move(row) };
        });
    }

    /** ClassLevelStat 표(ClassLevelStat.xlsx)의 한 행. */
    struct ClassLevelStatRow
    {
        /** 직업 */
        Protocol::CharacterClass Class = {};
        /** 레벨 */
        int32 Level = 0;
        /** 최대 HP */
        int32 MaxHp = 0;
        /** 최대 MP */
        int32 MaxMp = 0;
        /** 물리 공격력 */
        int32 PhysicalAttack = 0;
        /** 마법 공격력 */
        int32 MagicalAttack = 0;
    };

    /** Class, Level → 행. 키 순서로 돈다. */
    using ClassLevelStatTable = map<tuple<Protocol::CharacterClass, int32>, ClassLevelStatRow>;

    /** ClassLevelStat.json을 읽는다. 실패하면 사유를 돌려주고, table에는 일부만 채워졌을 수 있다. */
    inline optional<string> ParseClassLevelStat(const Json& document, OUT ClassLevelStatTable& table)
    {
        return Detail::ParseTable(CLASS_LEVEL_STAT_FILE, document, OUT table, [](const Json& object)
        {
            ClassLevelStatRow row;
            row.Class = Detail::Read<Protocol::CharacterClass>(object, "Class");
            row.Level = Detail::Read<int32>(object, "Level");
            row.MaxHp = Detail::Read<int32>(object, "MaxHp");
            row.MaxMp = Detail::Read<int32>(object, "MaxMp");
            row.PhysicalAttack = Detail::Read<int32>(object, "PhysicalAttack");
            row.MagicalAttack = Detail::Read<int32>(object, "MagicalAttack");

            auto key = tuple{ row.Class, row.Level };
            return pair{ std::move(key), std::move(row) };
        });
    }

    /** Monster 표(Monster.xlsx)의 한 행. */
    struct MonsterRow
    {
        /** 몬스터 번호. 5000번대다 */
        int32 Id = 0;
        /** 최대 HP */
        int32 MaxHp = 0;
        /** 공격력 */
        int32 BaseAttack = 0;
        /** 이동 속도 */
        float MovementSpeed = 0.f;
        /** 처치하면 주는 경험치의 최솟값. 0 이상이고 MaxExp보다 크지 않다 */
        int64 MinExp = 0;
        /** 처치하면 주는 경험치의 최댓값 */
        int64 MaxExp = 0;
        /** 처치하면 주는 골드의 최솟값. 0 이상이고 MaxGold보다 크지 않다 */
        int64 MinGold = 0;
        /** 처치하면 주는 골드의 최댓값 */
        int64 MaxGold = 0;
        /** 공격 간격(초) */
        float AttackInterval = 0.f;
        /** 공격을 시도하는 거리 */
        float TryAttackRange = 0.f;
        /** 플레이어를 알아채는 거리 */
        float DetectionRange = 0.f;
        /** 스폰 위치에서 쫓아갈 수 있는 최대 거리 */
        float ChasingMaxRange = 0.f;
        /** 처치할 때 판정하는 드롭 그룹. ';'로 구분한다. 비어 있으면 드롭이 없다 */
        vector<int32> DropGroups;
    };

    /** Id → 행. 키 순서로 돈다. */
    using MonsterTable = map<int32, MonsterRow>;

    /** Monster.json을 읽는다. 실패하면 사유를 돌려주고, table에는 일부만 채워졌을 수 있다. */
    inline optional<string> ParseMonster(const Json& document, OUT MonsterTable& table)
    {
        return Detail::ParseTable(MONSTER_FILE, document, OUT table, [](const Json& object)
        {
            MonsterRow row;
            row.Id = Detail::Read<int32>(object, "Id");
            row.MaxHp = Detail::Read<int32>(object, "MaxHp");
            row.BaseAttack = Detail::Read<int32>(object, "BaseAttack");
            row.MovementSpeed = Detail::Read<float>(object, "MovementSpeed");
            row.MinExp = Detail::Read<int64>(object, "MinExp");
            row.MaxExp = Detail::Read<int64>(object, "MaxExp");
            row.MinGold = Detail::Read<int64>(object, "MinGold");
            row.MaxGold = Detail::Read<int64>(object, "MaxGold");
            row.AttackInterval = Detail::Read<float>(object, "AttackInterval");
            row.TryAttackRange = Detail::Read<float>(object, "TryAttackRange");
            row.DetectionRange = Detail::Read<float>(object, "DetectionRange");
            row.ChasingMaxRange = Detail::Read<float>(object, "ChasingMaxRange");
            row.DropGroups = Detail::Read<vector<int32>>(object, "DropGroups");

            auto key = row.Id;
            return pair{ std::move(key), std::move(row) };
        });
    }

    /** DropGroup 표(DropGroup.xlsx)의 한 행. */
    struct DropGroupRow
    {
        /** 드롭 그룹 번호. 몬스터의 DropGroups가 이 번호를 적는다 */
        int32 Id = 0;
        /** 판정 방식. INDEPENDENT는 항목마다 Rate로 따로 판정하고, WEIGHTED는 Weight에 비례해 PickCount개를 뽑는다 */
        DropMethod Method = {};
        /** 그룹이 판정에 들어갈 확률. 0 초과 1 이하다 */
        float Rate = 0.f;
        /** WEIGHTED 그룹이 뽑는 항목 수. 1 이상, 항목 수 이하다. INDEPENDENT 그룹은 비워 둔다 */
        optional<int32> PickCount;
        /** DropEntry 표에서 이 행에 속한 행. 키 순서다. */
        vector<DropEntryRow> Entries;
    };

    /** Id → 행. 키 순서로 돈다. */
    using DropGroupTable = map<int32, DropGroupRow>;

    /** DropGroup.json을 읽는다. 실패하면 사유를 돌려주고, table에는 일부만 채워졌을 수 있다. */
    inline optional<string> ParseDropGroup(const Json& document, OUT DropGroupTable& table)
    {
        return Detail::ParseTable(DROP_GROUP_FILE, document, OUT table, [](const Json& object)
        {
            DropGroupRow row;
            row.Id = Detail::Read<int32>(object, "Id");
            row.Method = Detail::Read<DropMethod>(object, "Method");
            row.Rate = Detail::Read<float>(object, "Rate");
            row.PickCount = Detail::Read<optional<int32>>(object, "PickCount");
            row.Entries = Detail::ReadRows<DropEntryRow>(object, "Entries", ReadDropEntryRow);

            auto key = row.Id;
            return pair{ std::move(key), std::move(row) };
        });
    }

    /** Map 표(Map.xlsx)의 한 행. */
    struct MapRow
    {
        /** 맵 번호. 클라이언트가 이 번호로 열 레벨을 고른다 */
        int32 Id = 0;
    };

    /** Id → 행. 키 순서로 돈다. */
    using MapTable = map<int32, MapRow>;

    /** Map.json을 읽는다. 실패하면 사유를 돌려주고, table에는 일부만 채워졌을 수 있다. */
    inline optional<string> ParseMap(const Json& document, OUT MapTable& table)
    {
        return Detail::ParseTable(MAP_FILE, document, OUT table, [](const Json& object)
        {
            MapRow row;
            row.Id = Detail::Read<int32>(object, "Id");

            auto key = row.Id;
            return pair{ std::move(key), std::move(row) };
        });
    }

    /** Area 표(Area.xlsx)의 한 행. */
    struct AreaRow
    {
        /** 지역 번호 */
        int32 Id = 0;
        /** 이 지역이 속한 맵 */
        int32 MapId = 0;
        /** 리스폰 지점이 있으면 TRUE. 리스폰 지점이 있는 지역(마을)은 하나뿐이다 */
        bool HasRespawnPoint = false;
        /** 나오는 몬스터. ';'로 구분한다. 비어 있으면 몬스터가 나오지 않는다 */
        vector<int32> Monsters;
        /** 동시에 있는 몬스터의 최대 수. 0 이상이다 */
        int32 MaxMonsterCount = 0;
        /** 몬스터가 다시 나오는 시간(초). 0 이상이다 */
        int32 MonsterRespawnSec = 0;
        /** 지역 중심의 X 좌표 */
        float CenterX = 0.f;
        /** 지역 중심의 Y 좌표 */
        float CenterY = 0.f;
        /** 지역 중심의 Z 좌표 */
        float CenterZ = 0.f;
        /** 중심에서 X 방향 반경. 0보다 크다 */
        float HalfExtentX = 0.f;
        /** 중심에서 Y 방향 반경. 0보다 크다 */
        float HalfExtentY = 0.f;
        /** 리스폰 지점의 X 좌표. 리스폰 지점이 없으면 비워 둔다 */
        optional<float> RespawnX;
        /** 리스폰 지점의 Y 좌표 */
        optional<float> RespawnY;
        /** 리스폰 지점의 Z 좌표 */
        optional<float> RespawnZ;
        /** Portal 표에서 이 행에 속한 행. 키 순서다. */
        vector<PortalRow> Portals;
    };

    /** Id → 행. 키 순서로 돈다. */
    using AreaTable = map<int32, AreaRow>;

    /** Area.json을 읽는다. 실패하면 사유를 돌려주고, table에는 일부만 채워졌을 수 있다. */
    inline optional<string> ParseArea(const Json& document, OUT AreaTable& table)
    {
        return Detail::ParseTable(AREA_FILE, document, OUT table, [](const Json& object)
        {
            AreaRow row;
            row.Id = Detail::Read<int32>(object, "Id");
            row.MapId = Detail::Read<int32>(object, "MapId");
            row.HasRespawnPoint = Detail::Read<bool>(object, "HasRespawnPoint");
            row.Monsters = Detail::Read<vector<int32>>(object, "Monsters");
            row.MaxMonsterCount = Detail::Read<int32>(object, "MaxMonsterCount");
            row.MonsterRespawnSec = Detail::Read<int32>(object, "MonsterRespawnSec");
            row.CenterX = Detail::Read<float>(object, "CenterX");
            row.CenterY = Detail::Read<float>(object, "CenterY");
            row.CenterZ = Detail::Read<float>(object, "CenterZ");
            row.HalfExtentX = Detail::Read<float>(object, "HalfExtentX");
            row.HalfExtentY = Detail::Read<float>(object, "HalfExtentY");
            row.RespawnX = Detail::Read<optional<float>>(object, "RespawnX");
            row.RespawnY = Detail::Read<optional<float>>(object, "RespawnY");
            row.RespawnZ = Detail::Read<optional<float>>(object, "RespawnZ");
            row.Portals = Detail::ReadRows<PortalRow>(object, "Portals", ReadPortalRow);

            auto key = row.Id;
            return pair{ std::move(key), std::move(row) };
        });
    }

    //~ 설정

    /** Progress 설정(Config.xlsx의 Progress 시트). */
    struct ProgressConfig
    {
        /** 새 캐릭터가 처음 서는 맵. 지역 번호가 아니라 맵 표의 맵 번호다 */
        int32 StartMapId = 0;
        /** 시작 위치의 X 좌표 */
        float StartX = 0.f;
        /** 시작 위치의 Y 좌표 */
        float StartY = 0.f;
        /** 시작 위치의 Z 좌표 */
        float StartZ = 0.f;
        /** 시작 방향(도) */
        float StartYaw = 0.f;
        /** 새 캐릭터가 들고 시작하는 골드. 0 이상이다 */
        int64 StartGold = 0;
    };

    /** Progress.json을 읽는다. 실패하면 사유를 돌려준다. */
    inline optional<string> ParseProgress(const Json& document, OUT ProgressConfig& config)
    {
        return Detail::ParseObject(PROGRESS_FILE, document, [&](const Json& object)
        {
            config.StartMapId = Detail::Read<int32>(object, "StartMapId");
            config.StartX = Detail::Read<float>(object, "StartX");
            config.StartY = Detail::Read<float>(object, "StartY");
            config.StartZ = Detail::Read<float>(object, "StartZ");
            config.StartYaw = Detail::Read<float>(object, "StartYaw");
            config.StartGold = Detail::Read<int64>(object, "StartGold");
        });
    }
}
