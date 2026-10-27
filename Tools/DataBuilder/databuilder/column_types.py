"""열 타입(시트 2행)을 해석하고, 셀 값을 그 타입의 값으로 바꾼다.

| 표기 | 뜻 |
| --- | --- |
| `int32` · `int64` · `float` · `string` · `bool` | 값 하나 |
| `enum:<열거형>` · `enum[]:<열거형>` | 열거형 값 하나, 목록 |
| `ref:<표>` · `ref?:<표>` · `ref[]:<표>` | 다른 표의 키. 필수, 선택(빈 셀 허용), 목록 |
| `parent:<표>` | 이 행이 속한 부모 표의 키. 필수다 |

`key:`를 앞에 붙이면 키 열이다(`key:int32`, `key:parent:Item`). 키 열이 될 수 있는 것은 정수, 문자열, 열거형, 참조 하나다.
목록은 셀 하나에 `;`로 구분해 적는다. 빈 셀은 빈 목록이다.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Any

from databuilder.enums import EnumDef

SCALAR_KINDS = ("int32", "int64", "float", "string", "bool")
KEYABLE_KINDS = ("int32", "int64", "string", "enum", "ref", "parent")
INT_RANGES = {"int32": (-(2**31), 2**31 - 1), "int64": (-(2**63), 2**63 - 1)}
INTEGER_TEXT = re.compile(r"^-?\d+$")

TYPE_PATTERN = re.compile(r"^(?P<kind>enum|ref|parent)(?P<mod>\[\]|\?)?:(?P<target>[A-Za-z][A-Za-z0-9]*)$")


class CellError(Exception):
    """셀 값이 열 타입에 맞지 않는다. 위치는 부른 쪽이 붙인다."""


@dataclass(frozen=True)
class ColumnType:
    kind: str
    # 열거형 이름이나 참조하는 표 이름. 값 하나짜리 타입이면 None.
    target: str | None = None
    is_key: bool = False
    is_list: bool = False
    is_optional: bool = False

    @property
    def is_reference(self) -> bool:
        return self.kind in ("ref", "parent")


def parse_type(text: Any) -> ColumnType:
    """시트 2행의 타입 표기를 읽는다. 틀리면 CellError."""
    if not isinstance(text, str) or not text.strip():
        raise CellError("타입이 비어 있다")
    rest = text.strip()

    is_key = rest.startswith("key:")
    if is_key:
        rest = rest[len("key:"):]

    if rest in SCALAR_KINDS:
        column_type = ColumnType(rest, is_key=is_key)
    else:
        match = TYPE_PATTERN.match(rest)
        if match is None:
            raise CellError(f"모르는 타입 '{text}'")
        kind, modifier = match.group("kind"), match.group("mod")
        if modifier == "?" and kind != "ref":
            raise CellError(f"'?'는 ref에만 붙는다: '{text}'")
        if modifier == "[]" and kind == "parent":
            raise CellError(f"부모 표 참조는 목록이 될 수 없다: '{text}'")
        column_type = ColumnType(kind, match.group("target"), is_key, modifier == "[]", modifier == "?")

    if is_key and (column_type.kind not in KEYABLE_KINDS or column_type.is_list or column_type.is_optional):
        raise CellError(f"키 열이 될 수 없는 타입이다: '{text}'")
    return column_type


def is_blank(value: Any) -> bool:
    return value is None or (isinstance(value, str) and not value.strip())


def convert_cell(value: Any, column_type: ColumnType, enums: dict[str, EnumDef], ref_key_type: ColumnType | None) -> Any:
    """셀 값을 열 타입의 값으로 바꾼다. 참조 열은 ref_key_type(참조를 따라가 닿은 키의 타입)으로 바꾼다.

    열거형은 값 이름 문자열로, 선택 참조의 빈 셀은 None으로, 목록은 list로 돌려준다. 맞지 않으면 CellError.
    """
    if isinstance(value, str) and value.strip()[:1] in ("{", "["):
        raise CellError("JSON 셀은 쓰지 않는다. 목록은 ';'로 구분하고, 좌표와 범위는 열로 나눈다")

    if column_type.is_list:
        if is_blank(value):
            return []
        parts = value.split(";") if isinstance(value, str) else [value]
        items = []
        for part in parts:
            if is_blank(part):
                raise CellError(f"목록 '{value}'에 빈 항목이 있다")
            items.append(convert_one(part.strip() if isinstance(part, str) else part, column_type, enums, ref_key_type))
        return items

    if is_blank(value):
        if column_type.is_optional:
            return None
        raise CellError("값이 비어 있다")
    return convert_one(value, column_type, enums, ref_key_type)


def convert_one(value: Any, column_type: ColumnType, enums: dict[str, EnumDef], ref_key_type: ColumnType | None) -> Any:
    kind = column_type.kind
    if column_type.is_reference:
        assert ref_key_type is not None and not ref_key_type.is_reference
        return convert_one(value, ref_key_type, enums, None)

    if kind in INT_RANGES:
        if isinstance(value, bool):
            raise CellError(f"정수가 아니다: {value!r}")
        if isinstance(value, float) and value.is_integer():
            value = int(value)
        elif isinstance(value, str) and INTEGER_TEXT.match(value.strip()):
            value = int(value.strip())
        if not isinstance(value, int):
            raise CellError(f"정수가 아니다: {value!r}")
        low, high = INT_RANGES[kind]
        if not low <= value <= high:
            raise CellError(f"{kind} 범위를 벗어난다: {value}")
        return value

    if kind == "float":
        if isinstance(value, str):
            try:
                value = float(value.strip())
            except ValueError:
                raise CellError(f"수가 아니다: {value!r}") from None
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise CellError(f"수가 아니다: {value!r}")
        return float(value)

    if kind == "string":
        if isinstance(value, bool) or isinstance(value, float):
            raise CellError(f"문자열이 아니다: {value!r}")
        return str(value)

    if kind == "bool":
        if isinstance(value, bool):
            return value
        if isinstance(value, str) and value.strip().upper() in ("TRUE", "FALSE"):
            return value.strip().upper() == "TRUE"
        raise CellError(f"불이 아니다: {value!r}. TRUE나 FALSE를 적는다")

    if kind == "enum":
        enum = enums[column_type.target]
        name = value.strip() if isinstance(value, str) else value
        if name not in enum.values:
            raise CellError(f"'{value}'는 열거형 {enum.name}의 값이 아니다. 적을 수 있는 값: {', '.join(enum.values)}")
        return name

    raise AssertionError(f"처리하지 않은 타입 {kind}")
