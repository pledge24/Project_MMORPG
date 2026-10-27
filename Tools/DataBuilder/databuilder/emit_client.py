"""클라이언트가 데이터 테이블에 채울 JSON과 그 행 구조체(USTRUCT) 헤더를 만든다.

키 말고 클라이언트로 가는 열(both, client)이 하나라도 있는 표만 클라이언트 산출물을 만든다.
JSON의 각 행은 첫 필드 `RowName`이 데이터 테이블의 행 이름이고, 그 값은 기획 데이터의 키다(복합 키는 `_`로 잇는다).
에디터 스크립트가 데이터 테이블의 `ImportKeyField`를 `RowName`으로 두고 이 JSON으로 테이블을 채운다.

`.proto`의 열거형은 `UPROPERTY`가 될 수 없으므로 값 이름(`FString`)으로 싣고, 행 구조체에 프로토콜 열거형을 돌려주는 함수를 둔다.
설정 파일의 열거형은 `UENUM`으로 만든다. 이름은 클라이언트 규칙(`FP1<표>Row`, `EP1<열거형>`)을 따르고, 파일 하나에 타입 하나다.
"""

from __future__ import annotations

from databuilder.column_types import ColumnType
from databuilder.config import ClientConfig
from databuilder.emit_json import dump, fields_text
from databuilder.workbooks import Column, DesignData, Row, Table, locate, nested_children, sort_key, targets_client, value_type

ROW_NAME_FIELD = "RowName"
ENUM_NAMES_FILE = "P1DesignDataEnumNames.h"
VERSION_HEADER_FILE = "P1DesignDataVersion.h"
# 클라이언트의 프로토콜 생성물(`*.pb.h`)이 있는 폴더. 모듈 소스 루트 기준이다.
PROTO_INCLUDE_DIR = "Network"

NOTICE = [
    "// DataBuilder가 생성한 파일이다. 손으로 고치지 않는다. 다시 생성하면 덮어써진다.",
    "// 원본은 DesignData/의 워크북, 설정은 Tools/DataBuilder/databuilder.toml이다.",
    "// 다시 만들기: pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1",
]

SCALAR_UE = {"int32": "int32", "int64": "int64", "float": "float", "string": "FString", "bool": "bool"}
SCALAR_UE_DEFAULT = {"int32": " = 0", "int64": " = 0", "float": " = 0.f", "bool": " = false"}
# UENUM의 0번 값. 「값 없음」이라 셀에 적을 수 없다.
UENUM_NONE = "None"


def client_columns(columns: list[Column]) -> list[Column]:
    return [column for column in columns if targets_client(column.target)]


def has_client_output(table: Table) -> bool:
    """키 말고 클라이언트로 가는 열이 하나라도 있는 표. 중첩한 표는 부모 표 참조도 키처럼 보지 않는다."""
    return any(not column.type.is_key for column in client_columns(table.columns))


def nested_client_columns(table: Table) -> list[Column]:
    """중첩한 표가 싣는 열. 부모 표 참조는 부모 행이 이미 말해 주므로 뺀다."""
    return [column for column in client_columns(table.columns) if column.type.kind != "parent"]


def client_tables(data: DesignData) -> list[Table]:
    """클라이언트 산출물을 만드는 표. 설정 파일의 순서다."""
    return [table for table in data.tables.values() if has_client_output(table)]


def client_children(table: Table, data: DesignData) -> list[Table]:
    return [child for child in nested_children(table, data) if has_client_output(child)]


def check_client(data: DesignData, errors: list[str]) -> None:
    """클라이언트 산출물로 담을 수 없는 원본을 막는다. 클라이언트 산출물을 만들 때만 검사한다."""
    for table in data.tables.values():
        workbook = table.config.workbook
        for column in client_columns(table.columns):
            if column.type.is_optional:
                errors.append(f"{locate(workbook, None, 2, column.letter, column.name)}: 선택 값(빈 셀)은 클라이언트로 보낼 수 없다. 데이터 테이블의 행 구조체가 빈 값을 담지 못한다")
        if table.config.nest and has_client_output(table):
            parent = data.tables.get(table.parent_column.type.target) if table.parent_column else None
            if parent is not None and not has_client_output(parent):
                errors.append(f"{workbook}: 부모 표 '{parent.name}'에 클라이언트로 가는 열이 없어서 이 표를 클라이언트 행에 중첩할 수 없다")

    for setting in data.settings.values():
        for column in client_columns(setting.fields):
            where = locate(setting.config.workbook, setting.config.sheet, setting.rows[column.name], "C", column.name)
            errors.append(f"{where}: 설정 시트는 클라이언트 산출물을 만들지 않는다. 대상을 server나 none으로 둔다")

    for name in used_client_enums(data):
        enum = data.enums[name]
        if enum.proto_file is None and UENUM_NONE.upper() in enum.values:
            errors.append(f"열거형 {name}: 값 이름 NONE은 클라이언트 열거형의 0번 값({UENUM_NONE})과 겹친다")
        if enum.proto_file is None and any(number > 255 for number in enum.values.values()):
            errors.append(f"열거형 {name}: 클라이언트 열거형은 uint8이라 값 번호가 255를 넘을 수 없다")


def used_client_enums(data: DesignData) -> list[str]:
    names = set()
    for table in client_tables(data):
        for column in client_columns(table.columns):
            base = value_type(column.type, data)
            if base.kind == "enum":
                names.add(base.target)
    return sorted(names)


def row_name(table: Table, row: Row) -> str:
    return "_".join(str(row.values[column.name]) for column in table.key_columns)


def client_table_json(table: Table, data: DesignData) -> str:
    """한 행 한 줄이고, 행은 키 순서, 필드는 `RowName` 다음에 시트의 열 순서다. 중첩한 표의 행은 부모 행 아래에 한 줄씩 싣는다."""
    columns = client_columns(table.columns)
    children = client_children(table, data)
    key = table.key_columns[0].name
    lines = []
    for row in sorted(table.rows, key=lambda row: sort_key(table, row, data)):
        text = "{" + f"{dump(ROW_NAME_FIELD)}:{dump(row_name(table, row))}," + fields_text(columns, row)
        for child in children:
            parent = child.parent_column.name
            child_rows = sorted((r for r in child.rows if r.values[parent] == row.values[key]), key=lambda r: sort_key(child, r, data))
            child_lines = ["{" + fields_text(nested_client_columns(child), child_row) + "}" for child_row in child_rows]
            text += f",{dump(child.config.nest)}:[" + ("\n" + ",\n".join(child_lines) + "\n" if child_lines else "") + "]"
        lines.append(text + "}")
    return "[\n" + ",\n".join(lines) + ("\n" if lines else "") + "]\n"


def struct_name(table: Table) -> str:
    return f"FP1{table.name}Row"


def row_header_name(table: Table) -> str:
    return f"P1{table.name}Row.h"


def enum_header_name(enum_name: str) -> str:
    return f"P1{enum_name}.h"


def is_proto_enum(column_type: ColumnType, data: DesignData) -> bool:
    base = value_type(column_type, data)
    return base.kind == "enum" and data.enums[base.target].proto_file is not None


def ue_value_type(column_type: ColumnType, data: DesignData) -> str:
    base = value_type(column_type, data)
    if base.kind == "enum":
        # .proto의 열거형은 UPROPERTY가 될 수 없어서 값 이름으로 담는다.
        return "FString" if data.enums[base.target].proto_file else f"EP1{base.target}"
    return SCALAR_UE[base.kind]


def ue_field(column: Column, data: DesignData) -> list[str]:
    value = ue_value_type(column.type, data)
    if column.type.is_list:
        field_type, default = f"TArray<{value}>", ""
    else:
        base = value_type(column.type, data)
        field_type = value
        if base.kind == "enum":
            default = "" if is_proto_enum(column.type, data) else f" = {value}::{UENUM_NONE}"
        else:
            default = SCALAR_UE_DEFAULT.get(base.kind, "")
    lines = [f"    /** {column.description} */"] if column.description else []
    lines += ["    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)", f"    {field_type} {column.name}{default};", ""]
    return lines


def proto_getter(column: Column, data: DesignData) -> list[str]:
    """값 이름으로 담은 .proto 열거형을 프로토콜 열거형으로 돌려주는 함수."""
    cpp_type = data.enums[value_type(column.type, data).target].cpp_type
    if column.type.is_list:
        return [
            f"    /** {column.name} 값을 프로토콜 열거형으로 읽는다. 값 이름이 아닌 원소는 0번 값이다. */",
            f"    TArray<{cpp_type}> Get{column.name}() const",
            "    {",
            f"        TArray<{cpp_type}> ParsedValues;",
            f"        for (const FString& ElementName : {column.name})",
            "        {",
            f"            {cpp_type} Parsed{{}};",
            "            P1DesignData::ParseEnumName(ElementName, Parsed);",
            "            ParsedValues.Add(Parsed);",
            "        }",
            "        return ParsedValues;",
            "    }",
            "",
        ]
    return [
        f"    /** {column.name} 값을 프로토콜 열거형으로 읽는다. 값 이름이 아니면 0번 값이다. */",
        f"    {cpp_type} Get{column.name}() const",
        "    {",
        f"        {cpp_type} Parsed{{}};",
        f"        P1DesignData::ParseEnumName({column.name}, Parsed);",
        "        return Parsed;",
        "    }",
        "",
    ]


def row_header(table: Table, data: DesignData, client: ClientConfig) -> str:
    nested = bool(table.config.nest)
    columns = nested_client_columns(table) if nested else client_columns(table.columns)
    children = [] if nested else client_children(table, data)

    includes = set()
    for column in columns:
        base = value_type(column.type, data)
        if base.kind != "enum":
            continue
        enum = data.enums[base.target]
        if enum.proto_file:
            includes.add(f"{client.include_dir}/{ENUM_NAMES_FILE}")
            includes.add(f"{PROTO_INCLUDE_DIR}/{enum.proto_file.removesuffix('.proto')}.pb.h")
        else:
            includes.add(f"{client.include_dir}/{enum_header_name(base.target)}")
    for child in children:
        includes.add(f"{client.include_dir}/{row_header_name(child)}")

    name = struct_name(table)
    lines = NOTICE + ["#pragma once", "", '#include "CoreMinimal.h"']
    if not nested:
        lines.append('#include "Engine/DataTable.h"')
    lines += [f'#include "{include}"' for include in sorted(includes)]
    lines += [f'#include "{row_header_name(table).removesuffix(".h")}.generated.h"', ""]

    if nested:
        parent = table.parent_column.type.target
        lines.append(f"/** {table.name} 표({table.config.workbook})의 한 행. {parent} 행의 {table.config.nest} 아래에 키 순서로 실린다. */")
        lines += ["USTRUCT(BlueprintType)", f"struct {name}", "{"]
    else:
        keys = ", ".join(column.name for column in table.key_columns)
        lines.append(f"/** {table.name} 표({table.config.workbook})의 한 행. 행 이름은 키({keys})다. */")
        lines += ["USTRUCT(BlueprintType)", f"struct {name} : public FTableRowBase", "{"]
    lines += ["    GENERATED_BODY()", ""]

    for column in columns:
        if is_proto_enum(column.type, data):
            lines += proto_getter(column, data)
    for column in columns:
        lines += ue_field(column, data)
    for child in children:
        lines += [
            f"    /** {child.name} 표에서 이 행에 속한 행. 키 순서다. */",
            "    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)",
            f"    TArray<{struct_name(child)}> {child.config.nest};",
            "",
        ]

    while lines[-1] == "":
        lines.pop()
    lines.append("};")
    return "\n".join(lines) + "\n"


def uenum_header(enum_name: str, data: DesignData) -> str:
    enum = data.enums[enum_name]
    lines = NOTICE + [
        "#pragma once",
        "",
        '#include "CoreMinimal.h"',
        f'#include "{enum_header_name(enum_name).removesuffix(".h")}.generated.h"',
        "",
        f"/** 기획 데이터 전용 열거형 {enum_name}. 값 번호가 서버의 생성된 열거형과 같다. */",
        "UENUM(BlueprintType)",
        f"enum class EP1{enum_name} : uint8",
        "{",
        f"    {UENUM_NONE} = 0 UMETA(Hidden),",
    ]
    lines += [f"    {value} = {number}," for value, number in enum.values.items()]
    lines.append("};")
    return "\n".join(lines) + "\n"


def enum_names_header(enum_names: list[str], data: DesignData) -> str:
    protos = sorted({data.enums[name].proto_file for name in enum_names})
    lines = NOTICE + ["#pragma once", "", '#include "CoreMinimal.h"']
    lines += [f'#include "{PROTO_INCLUDE_DIR}/{proto.removesuffix(".proto")}.pb.h"' for proto in protos]
    lines += [
        "",
        "/**",
        " * 셀에 적는 값 이름을 프로토콜 열거형으로 읽는 함수.",
        " *",
        " * 데이터 테이블의 행 구조체가 값 이름으로 담은 .proto 열거형을 게임 코드가 쓰는 값으로 바꾼다.",
        " */",
        "namespace P1DesignData",
        "{",
    ]
    for name in enum_names:
        enum = data.enums[name]
        lines += [
            f"    /** 값 이름({', '.join(enum.values)})을 {enum.cpp_type}로 읽는다. 모르는 이름이면 OutValue를 두고 false. */",
            f"    inline bool ParseEnumName(const FString& Name, {enum.cpp_type}& OutValue)",
            "    {",
        ]
        # FString의 ==는 대소문자를 가리지 않는다. 셀의 값 이름은 대문자만 받으므로 구분해서 비교한다.
        lines += [
            f'        if (Name.Equals(TEXT("{value}"), ESearchCase::CaseSensitive)) {{ OutValue = {enum.cpp_values[value]}; return true; }}'
            for value in enum.values
        ]
        lines += ["        return false;", "    }", ""]
    lines[-1] = "}"
    return "\n".join(lines) + "\n"


def version_header(version: str) -> str:
    lines = NOTICE + [
        "#pragma once",
        "",
        '#include "CoreMinimal.h"',
        "",
        "/** DataBuilder가 생성한 기획 데이터의 버전. */",
        "namespace P1DesignData",
        "{",
        "    /** 원본 해시로 만든 데이터 버전. 같은 원본이면 서버가 기동할 때 남기는 버전과 같다. */",
        f'    inline constexpr const TCHAR* Version = TEXT("{version}");',
        "}",
    ]
    return "\n".join(lines) + "\n"


def client_outputs(client: ClientConfig, data: DesignData, version: str) -> dict:
    """클라이언트 산출물 폴더 → 파일 이름 → 내용."""
    tables = client_tables(data)
    json_files = {f"{table.name}.json": client_table_json(table, data) for table in tables if not table.config.nest}

    source_files = {VERSION_HEADER_FILE: version_header(version)}
    for table in tables:
        source_files[row_header_name(table)] = row_header(table, data, client)
    enums = used_client_enums(data)
    for name in enums:
        if data.enums[name].proto_file is None:
            source_files[enum_header_name(name)] = uenum_header(name, data)
    proto_enums = [name for name in enums if data.enums[name].proto_file]
    if proto_enums:
        source_files[ENUM_NAMES_FILE] = enum_names_header(proto_enums, data)
    return {client.json_dir: json_files, client.source_dir: source_files}
