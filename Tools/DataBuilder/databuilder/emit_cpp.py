"""서버 모듈마다 C++ 헤더 하나(`DesignData.h`)를 만든다.

헤더에는 그 모듈이 읽는 표의 행 구조체, 설정 구조체, `.proto`에 없는 열거형, 그리고 JSON을 그 타입으로 읽는 함수가 있다.
필드 이름은 열 이름 그대로다. 생성된 역직렬화는 형식(필드의 유무와 타입, 열거형 값 이름)만 확인한다.
기획 데이터의 규칙은 DataBuilder가 이미 검사했고, 코드가 정한 제약은 서버가 따로 검사한다.
"""

from __future__ import annotations

import re

from databuilder.column_types import ColumnType
from databuilder.emit_json import VERSION_FILE, nested_columns, server_columns
from databuilder.workbooks import Column, DesignData, Setting, Table, nested_children, value_type

HEADER_FILE = "DesignData.h"

SCALAR_CPP = {"int32": "int32", "int64": "int64", "float": "float", "string": "string", "bool": "bool"}
SCALAR_DEFAULT = {"int32": " = 0", "int64": " = 0", "float": " = 0.f", "bool": " = false", "enum": " = {}"}

DETAIL = r"""
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
"""


def screaming(name: str) -> str:
    """`ItemEquipStat` → `ITEM_EQUIP_STAT`."""
    return re.sub(r"(?<!^)(?=[A-Z])", "_", name).upper()


def cpp_value_type(column_type: ColumnType, data: DesignData) -> str:
    base = value_type(column_type, data)
    return data.enums[base.target].cpp_type if base.kind == "enum" else SCALAR_CPP[base.kind]


def cpp_field_type(column_type: ColumnType, data: DesignData) -> str:
    inner = cpp_value_type(column_type, data)
    if column_type.is_list:
        return f"vector<{inner}>"
    if column_type.is_optional:
        return f"optional<{inner}>"
    return inner


def cpp_field(column: Column, data: DesignData) -> list[str]:
    lines = []
    if column.description:
        lines.append(f"        /** {column.description} */")
    field_type = cpp_field_type(column.type, data)
    default = ""
    if not column.type.is_list and not column.type.is_optional:
        default = SCALAR_DEFAULT.get(value_type(column.type, data).kind, "")
    lines.append(f"        {field_type} {column.name}{default};")
    return lines


def used_enums(tables: list[Table], settings: list[Setting], data: DesignData) -> list[str]:
    names = set()
    for columns in [server_columns(t.columns) for t in tables] + [server_columns(s.fields) for s in settings]:
        for column in columns:
            base = value_type(column.type, data)
            if base.kind == "enum":
                names.add(base.target)
    return sorted(names)


def enum_lines(name: str, data: DesignData) -> list[str]:
    enum = data.enums[name]
    lines = []
    if enum.proto_file is None:
        lines += [f"    enum class {name} : int32", "    {"]
        lines += [f"        {value} = {number}," for value, number in enum.values.items()]
        lines += ["    };", ""]
    lines += [
        f"    /** 셀에 적는 값 이름({', '.join(enum.values)})을 {enum.cpp_type}로 읽는다. 모르는 이름이면 false. */",
        f"    inline bool ParseEnumName(string_view name, OUT {enum.cpp_type}& value)",
        "    {",
        f"        static const unordered_map<string_view, {enum.cpp_type}> VALUES = {{",
    ]
    lines += [f'            {{ "{value}", {enum.cpp_values[value]} }},' for value in enum.values]
    lines += [
        "        };",
        "",
        "        auto it = VALUES.find(name);",
        "        if (it == VALUES.end())",
        "            return false;",
        "",
        "        value = it->second;",
        "        return true;",
        "    }",
        "",
    ]
    return lines


def nested_table_lines(table: Table, data: DesignData) -> list[str]:
    """중첩한 표는 행 구조체와 원소 하나를 읽는 함수만 만든다. 파일과 키 → 행 맵은 부모 표가 갖는다."""
    columns = nested_columns(table)
    parent = table.parent_column.type.target
    lines = [f"    /** {table.name} 표({table.config.workbook})의 한 행. {parent} 행의 {table.config.nest} 아래에 키 순서로 실린다. */"]
    lines += [f"    struct {table.name}Row", "    {"]
    for column in columns:
        lines += cpp_field(column, data)
    lines += ["    };", ""]
    lines += [
        f"    /** {parent} 행의 {table.config.nest} 원소 하나를 읽는다. 틀리면 Detail::FieldError를 던진다. */",
        f"    inline {table.name}Row Read{table.name}Row(const Json& object)",
        "    {",
        f"        {table.name}Row row;",
    ]
    lines += [f'        row.{c.name} = Detail::Read<{cpp_field_type(c.type, data)}>(object, "{c.name}");' for c in columns]
    lines += ["        return row;", "    }", ""]
    return lines


def table_lines(table: Table, data: DesignData) -> list[str]:
    if table.config.nest:
        return nested_table_lines(table, data)

    columns = server_columns(table.columns)
    children = nested_children(table, data)
    keys = table.key_columns
    key_types = [cpp_value_type(key.type, data) for key in keys]
    key_type = key_types[0] if len(keys) == 1 else f"tuple<{', '.join(key_types)}>"
    key_expr = f"row.{keys[0].name}" if len(keys) == 1 else f"tuple{{ {', '.join(f'row.{key.name}' for key in keys)} }}"

    lines = [f"    /** {table.name} 표({table.config.workbook})의 한 행. */", f"    struct {table.name}Row", "    {"]
    for column in columns:
        lines += cpp_field(column, data)
    for child in children:
        lines += [f"        /** {child.name} 표에서 이 행에 속한 행. 키 순서다. */", f"        vector<{child.name}Row> {child.config.nest};"]
    lines += ["    };", ""]
    key_names = ", ".join(key.name for key in keys)
    lines += [f"    /** {key_names} → 행. 키 순서로 돈다. */", f"    using {table.name}Table = map<{key_type}, {table.name}Row>;", ""]
    lines += [
        f"    /** {table.name}.json을 읽는다. 실패하면 사유를 돌려주고, table에는 일부만 채워졌을 수 있다. */",
        f"    inline optional<string> Parse{table.name}(const Json& document, OUT {table.name}Table& table)",
        "    {",
        f"        return Detail::ParseTable({screaming(table.name)}_FILE, document, OUT table, [](const Json& object)",
        "        {",
        f"            {table.name}Row row;",
    ]
    lines += [f'            row.{c.name} = Detail::Read<{cpp_field_type(c.type, data)}>(object, "{c.name}");' for c in columns]
    lines += [
        f'            row.{child.config.nest} = Detail::ReadRows<{child.name}Row>(object, "{child.config.nest}", Read{child.name}Row);'
        for child in children
    ]
    lines += [
        "",
        f"            auto key = {key_expr};",
        "            return pair{ std::move(key), std::move(row) };",
        "        });",
        "    }",
        "",
    ]
    return lines


def setting_lines(setting: Setting, data: DesignData) -> list[str]:
    fields = server_columns(setting.fields)
    sheet = f"{setting.config.workbook}의 {setting.config.sheet} 시트"
    lines = [f"    /** {setting.name} 설정({sheet}). */", f"    struct {setting.name}Config", "    {"]
    for column in fields:
        lines += cpp_field(column, data)
    lines += ["    };", ""]
    lines += [
        f"    /** {setting.name}.json을 읽는다. 실패하면 사유를 돌려준다. */",
        f"    inline optional<string> Parse{setting.name}(const Json& document, OUT {setting.name}Config& config)",
        "    {",
        f"        return Detail::ParseObject({screaming(setting.name)}_FILE, document, [&](const Json& object)",
        "        {",
    ]
    lines += [f'            config.{c.name} = Detail::Read<{cpp_field_type(c.type, data)}>(object, "{c.name}");' for c in fields]
    lines += ["        });", "    }", ""]
    return lines


def module_header(namespace: str, tables: list[Table], settings: list[Setting], data: DesignData) -> str:
    enums = used_enums(tables, settings, data)
    proto_files = sorted({data.enums[name].proto_file for name in enums if data.enums[name].proto_file})

    lines = [
        "// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.",
        "// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.",
        "// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1",
        "#pragma once",
    ]
    lines += [f'#include "Protocol/{proto.removesuffix(".proto")}.pb.h"' for proto in proto_files]
    lines += [
        "",
        "/** DataBuilder가 생성한 기획 데이터의 타입과 역직렬화. */",
        f"namespace {namespace}",
        "{",
        "    /** 원본 해시로 만든 데이터 버전을 담은 파일. */",
        f'    constexpr string_view VERSION_FILE = "{VERSION_FILE}";',
    ]
    for table in tables:
        if not table.config.nest:
            lines.append(f'    constexpr string_view {screaming(table.name)}_FILE = "{table.name}.json";')
    for setting in settings:
        lines.append(f'    constexpr string_view {screaming(setting.name)}_FILE = "{setting.name}.json";')
    lines.append("")

    if enums:
        lines += ["    //~ 열거형", ""]
        for name in enums:
            lines += enum_lines(name, data)

    lines += DETAIL.strip("\n").split("\n")
    lines.append("")

    lines += [
        "    /** 데이터 버전 파일을 읽는다. 실패하면 사유를 돌려준다. */",
        "    inline optional<string> ParseDataVersion(const Json& document, OUT string& version)",
        "    {",
        "        return Detail::ParseObject(VERSION_FILE, document, [&](const Json& object)",
        "        {",
        '            version = Detail::Read<string>(object, "Version");',
        "        });",
        "    }",
        "",
    ]

    if tables:
        lines += ["    //~ 표", ""]
        # 부모 행 구조체가 중첩한 표의 행 구조체를 필드로 가지므로 중첩한 표를 먼저 선언한다.
        for table in sorted(tables, key=lambda table: not table.config.nest):
            lines += table_lines(table, data)
    if settings:
        lines += ["    //~ 설정", ""]
        for setting in settings:
            lines += setting_lines(setting, data)

    while lines[-1] == "":
        lines.pop()
    lines.append("}")
    return "\n".join(lines) + "\n"
