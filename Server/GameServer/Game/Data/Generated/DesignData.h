// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.
// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.
// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
#pragma once

/** DataBuilder가 생성한 기획 데이터의 타입과 역직렬화. */
namespace DesignData
{
    /** 원본 해시로 만든 데이터 버전을 담은 파일. */
    constexpr string_view VERSION_FILE = "DataVersion.json";
    constexpr string_view CURRENCY_FILE = "Currency.json";

    //~ 열거형

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
}
