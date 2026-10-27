"""서버 모듈이 읽는 JSON과 데이터 버전을 만든다.

표는 한 행 한 줄이고, 행은 키 순서, 필드는 시트의 열 순서다. 행 하나의 값을 바꾸면 diff가 그 줄 하나다.
중첩한 표의 행은 부모 행 아래에 한 행 한 줄로 싣고, 부모 행의 마지막 필드(설정의 `nest`)가 된다.
설정은 한 필드 한 줄이다.
"""

from __future__ import annotations

import hashlib
import json
from typing import Any

from databuilder.workbooks import Column, DesignData, Row, Setting, Table, nested_children, sort_key, targets_server

VERSION_FILE = "DataVersion.json"


def dump(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def server_columns(columns: list[Column]) -> list[Column]:
    return [column for column in columns if targets_server(column.target)]


def nested_columns(table: Table) -> list[Column]:
    """중첩한 표가 싣는 열. 부모 표 참조는 부모 행이 이미 말해 주므로 뺀다."""
    return [column for column in server_columns(table.columns) if column.type.kind != "parent"]


def fields_text(columns: list[Column], row: Row) -> str:
    return ",".join(f"{dump(column.name)}:{dump(row.values[column.name])}" for column in columns)


def table_json(table: Table, data: DesignData) -> str:
    columns = server_columns(table.columns)
    children = nested_children(table, data)
    key = table.key_columns[0].name
    lines = []
    for row in sorted(table.rows, key=lambda row: sort_key(table, row, data)):
        text = "{" + fields_text(columns, row)
        for child in children:
            parent = child.parent_column.name
            child_rows = sorted((r for r in child.rows if r.values[parent] == row.values[key]), key=lambda r: sort_key(child, r, data))
            child_lines = ["{" + fields_text(nested_columns(child), child_row) + "}" for child_row in child_rows]
            text += f",{dump(child.config.nest)}:[" + ("\n" + ",\n".join(child_lines) + "\n" if child_lines else "") + "]"
        lines.append(text + "}")
    return "[\n" + ",\n".join(lines) + ("\n" if lines else "") + "]\n"


def setting_json(setting: Setting) -> str:
    lines = [f"{dump(column.name)}:{dump(setting.values[column.name])}" for column in server_columns(setting.fields)]
    return "{\n" + ",\n".join(lines) + ("\n" if lines else "") + "}\n"


def data_version(data: DesignData) -> str:
    """원본의 값으로 만든 해시.

    설명(4행), 시트 이름, 행 순서, 어디에도 싣지 않는 열(none)은 산출물을 바꾸지 않으므로 넣지 않는다.
    """

    def carried(columns: list[Column]) -> list[Column]:
        return [column for column in columns if column.target != "none"]

    canonical = {
        "tables": {
            name: {
                "columns": [[c.name, repr(c.type), c.target] for c in carried(table.columns)],
                # 중첩은 원본의 값이 아니지만 산출물의 모양을 바꾼다.
                "nest": table.config.nest,
                "rows": [
                    [row.values[c.name] for c in carried(table.columns)]
                    for row in sorted(table.rows, key=lambda row: sort_key(table, row, data))
                ],
            }
            for name, table in sorted(data.tables.items())
        },
        "settings": {
            name: [[c.name, repr(c.type), c.target, setting.values[c.name]] for c in carried(setting.fields)]
            for name, setting in sorted(data.settings.items())
        },
        "enums": {name: enum.values for name, enum in sorted(data.enums.items())},
    }
    return hashlib.sha256(dump(canonical).encode("utf-8")).hexdigest()[:16]


def version_json(version: str) -> str:
    return "{" + f'"Version":{dump(version)}' + "}\n"
