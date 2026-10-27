"""서버 모듈이 읽는 JSON과 데이터 버전을 만든다.

표는 한 행 한 줄이고, 행은 키 순서, 필드는 시트의 열 순서다. 행 하나의 값을 바꾸면 diff가 그 줄 하나다.
설정은 한 필드 한 줄이다.
"""

from __future__ import annotations

import hashlib
import json
from typing import Any

from databuilder.workbooks import Column, DesignData, Setting, Table, sort_key, targets_server

VERSION_FILE = "DataVersion.json"


def dump(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"))


def server_columns(columns: list[Column]) -> list[Column]:
    return [column for column in columns if targets_server(column.target)]


def table_json(table: Table, data: DesignData) -> str:
    columns = server_columns(table.columns)
    rows = sorted(table.rows, key=lambda row: sort_key(table, row, data))
    lines = ["{" + ",".join(f"{dump(column.name)}:{dump(row.values[column.name])}" for column in columns) + "}" for row in rows]
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
