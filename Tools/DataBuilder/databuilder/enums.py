"""열거형을 모은다. `.proto`에 있는 열거형은 읽고, 없는 것만 설정 파일에서 만든다.

엑셀 셀에는 `.proto` 값 이름에서 열거형 이름의 접두사를 뺀 값을 적는다.
`ItemType`의 `ITEM_TYPE_GEAR`는 `GEAR`다. 0번 값(`ITEM_TYPE_NONE`)은 「값 없음」이라 셀에 적을 수 없다.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

# 최상위 열거형만 읽는다. 메시지 안의 열거형은 들여쓰기가 있어서 줄 첫머리에서 시작하지 않는다.
PROTO_ENUM = re.compile(r"^enum\s+(\w+)\s*\{(.*?)\}", re.MULTILINE | re.DOTALL)
PROTO_ENUM_VALUE = re.compile(r"(\w+)\s*=\s*(-?\d+)\s*;")
LINE_COMMENT = re.compile(r"//[^\n]*")


class EnumError(Exception):
    """열거형 정의가 틀렸다."""


@dataclass(frozen=True)
class EnumDef:
    name: str
    # 셀에 적는 값 이름 → 값 번호. 선언 순서다.
    values: dict[str, int]
    # C++에서 쓰는 타입 이름과 값 표현.
    cpp_type: str
    cpp_values: dict[str, str]
    # `.proto`에서 읽었으면 그 파일 이름(Common.proto). 생성하는 열거형이면 None.
    proto_file: str | None


def screaming_prefix(enum_name: str) -> str:
    """`ItemType` → `ITEM_TYPE_`. `.proto` 값 이름의 접두사다."""
    return re.sub(r"(?<!^)(?=[A-Z])", "_", enum_name).upper() + "_"


def read_proto_enums(proto_dir: Path) -> dict[str, EnumDef]:
    """`.proto` 파일(UTF-8)의 최상위 열거형을 모두 읽는다. 접두사 규칙을 어긴 값이 있으면 EnumError."""
    enums: dict[str, EnumDef] = {}
    for proto_path in sorted(proto_dir.glob("*.proto")):
        text = LINE_COMMENT.sub("", proto_path.read_text(encoding="utf-8"))
        for match in PROTO_ENUM.finditer(text):
            name, body = match.group(1), match.group(2)
            prefix = screaming_prefix(name)
            values: dict[str, int] = {}
            cpp_values: dict[str, str] = {}
            for value_name, number in PROTO_ENUM_VALUE.findall(body):
                if not value_name.startswith(prefix):
                    raise EnumError(f"{proto_path.name}: 열거형 {name}의 값 {value_name}이 접두사 {prefix}로 시작하지 않는다")
                if int(number) == 0:
                    continue
                cell_name = value_name[len(prefix):]
                values[cell_name] = int(number)
                cpp_values[cell_name] = f"Protocol::{value_name}"
            if name in enums:
                raise EnumError(f"{proto_path.name}: 열거형 {name}이 {enums[name].proto_file}에도 있다")
            enums[name] = EnumDef(name, values, f"Protocol::{name}", cpp_values, proto_path.name)
    return enums


def collect_enums(proto_dir: Path, generated: dict[str, dict[str, int]]) -> dict[str, EnumDef]:
    """`.proto`의 열거형과 설정 파일의 열거형을 합친다. 두 곳에 같은 이름이 있으면 EnumError."""
    enums = read_proto_enums(proto_dir)
    for name, values in generated.items():
        if name in enums:
            raise EnumError(f"열거형 {name}은 {enums[name].proto_file}에 있으므로 설정 파일에서 만들지 않는다")
        enums[name] = EnumDef(name, dict(values), name, {value: f"{name}::{value}" for value in values}, None)
    return enums
