"""DataBuilder 설정 파일(`databuilder.toml`)을 읽는다.

어느 워크북이 어느 표가 되고, 어느 서버 모듈이 그 표를 읽는지는 이 파일이 정한다. 엑셀은 정하지 않는다.
"""

from __future__ import annotations

import re
import tomllib
from dataclasses import dataclass
from pathlib import Path

# 표, 설정, 열거형, 모듈 이름이 그대로 C++ 이름의 일부가 되므로 식별자로 제한한다.
TYPE_NAME = re.compile(r"^[A-Z][A-Za-z0-9]*$")
MODULE_NAME = re.compile(r"^[a-z][a-z0-9]*$")
ENUM_VALUE_NAME = re.compile(r"^[A-Z][A-Z0-9_]*$")
OUTPUT_DIR_NAME = "Generated"
# 모듈의 namespace를 적지 않으면 쓰는 C++ 네임스페이스.
DEFAULT_NAMESPACE = "DesignData"


class ConfigError(Exception):
    """설정 파일이 틀렸다. 메시지에 설정 파일의 어느 항목인지 들어 있다."""


@dataclass(frozen=True)
class ModuleConfig:
    """서버 모듈 하나. 산출물 폴더는 이 모듈만 읽는다."""

    name: str
    server_dir: Path
    # 생성되는 헤더의 C++ 네임스페이스. 한 번역 단위가 두 모듈의 헤더를 함께 include할 수 있어서 모듈마다 다르다.
    namespace: str = DEFAULT_NAMESPACE


@dataclass(frozen=True)
class ClientConfig:
    """클라이언트 산출물 폴더. 데이터 테이블에 채울 JSON과 그 행 구조체 헤더를 따로 둔다."""

    json_dir: Path
    source_dir: Path
    # 생성 헤더를 include할 때 쓰는 모듈 소스 루트 기준 경로(`Game/Data/Generated`).
    include_dir: str


@dataclass(frozen=True)
class TableConfig:
    """가로형 표 하나. 워크북의 시트를 모두 이어 붙인다."""

    name: str
    workbook: str
    modules: tuple[str, ...]
    # 부모 표 행의 이 필드 아래에 중첩한다. 중첩한 표는 파일과 키 → 행 맵을 따로 만들지 않는다.
    nest: str | None = None


@dataclass(frozen=True)
class SettingConfig:
    """세로형 설정 시트 하나. 시트 하나가 구조체 하나가 된다."""

    name: str
    workbook: str
    sheet: str
    modules: tuple[str, ...]


@dataclass(frozen=True)
class Config:
    source_dir: Path
    proto_dir: Path
    modules: dict[str, ModuleConfig]
    tables: dict[str, TableConfig]
    settings: dict[str, SettingConfig]
    # .proto에 없는 열거형. 열거형 이름 → 값 이름 → 값 번호.
    enums: dict[str, dict[str, int]]
    # 없으면 클라이언트 산출물을 만들지 않는다.
    client: ClientConfig | None = None


def load_config(path: Path, root: Path) -> Config:
    """설정 파일을 읽는다. 상대 경로는 root 기준이다. 틀리면 ConfigError."""
    try:
        raw = tomllib.loads(path.read_text(encoding="utf-8"))
    except (OSError, tomllib.TOMLDecodeError) as error:
        raise ConfigError(f"{path}: 설정 파일을 읽지 못했다: {error}") from error

    def require_str(section: dict, key: str, where: str) -> str:
        value = section.get(key)
        if not isinstance(value, str) or not value:
            raise ConfigError(f"{path}: {where}의 '{key}'가 없거나 문자열이 아니다")
        return value

    modules: dict[str, ModuleConfig] = {}
    for name, section in raw.get("modules", {}).items():
        if not MODULE_NAME.match(name):
            raise ConfigError(f"{path}: 모듈 이름 '{name}'은 소문자 식별자여야 한다")
        server_dir = root / require_str(section, "server_dir", f"modules.{name}")
        # 배포가 이 폴더를 통째로 지운다. 설정을 잘못 적어 사람이 쓴 폴더를 지우지 않도록 이름을 고정한다.
        if server_dir.name != OUTPUT_DIR_NAME:
            raise ConfigError(f"{path}: modules.{name}의 server_dir은 '{OUTPUT_DIR_NAME}' 폴더여야 한다")
        namespace = section.get("namespace", DEFAULT_NAMESPACE)
        if not isinstance(namespace, str) or not TYPE_NAME.match(namespace):
            raise ConfigError(f"{path}: modules.{name}의 'namespace'는 PascalCase 식별자여야 한다")
        # 게임과 로비의 헤더를 함께 include하는 코드(기동할 때의 대조)가 있어서, 같은 네임스페이스면 같은 이름이 두 번 정의된다.
        for other in modules.values():
            if other.namespace == namespace:
                raise ConfigError(f"{path}: modules.{name}과 modules.{other.name}이 같은 namespace '{namespace}'를 쓴다")
        modules[name] = ModuleConfig(name, server_dir, namespace)

    client = None
    if "client" in raw:
        section = raw["client"]
        json_dir = root / require_str(section, "json_dir", "client")
        source_dir = root / require_str(section, "source_dir", "client")
        for key, folder in (("json_dir", json_dir), ("source_dir", source_dir)):
            if folder.name != OUTPUT_DIR_NAME:
                raise ConfigError(f"{path}: client의 {key}는 '{OUTPUT_DIR_NAME}' 폴더여야 한다")
        # 클라이언트의 include는 모듈 소스 루트(Source/<모듈>/) 기준이다.
        parts = source_dir.relative_to(root).parts
        if "Source" not in parts or len(parts) < parts.index("Source") + 3:
            raise ConfigError(f"{path}: client의 source_dir은 Source/<모듈>/ 아래여야 한다")
        include_dir = "/".join(parts[parts.index("Source") + 2:])
        client = ClientConfig(json_dir, source_dir, include_dir)

    def require_modules(section: dict, where: str) -> tuple[str, ...]:
        names = section.get("modules")
        if not isinstance(names, list) or not names:
            raise ConfigError(f"{path}: {where}의 'modules'가 비어 있거나 목록이 아니다")
        for module in names:
            if module not in modules:
                raise ConfigError(f"{path}: {where}의 모듈 '{module}'이 [modules]에 없다")
        return tuple(names)

    tables: dict[str, TableConfig] = {}
    for name, section in raw.get("tables", {}).items():
        where = f"tables.{name}"
        if not TYPE_NAME.match(name):
            raise ConfigError(f"{path}: 표 이름 '{name}'은 PascalCase 식별자여야 한다")
        nest = section.get("nest")
        if nest is not None and (not isinstance(nest, str) or not TYPE_NAME.match(nest)):
            raise ConfigError(f"{path}: {where}의 'nest'는 PascalCase 식별자여야 한다")
        tables[name] = TableConfig(name, require_str(section, "workbook", where), require_modules(section, where), nest)

    settings: dict[str, SettingConfig] = {}
    for name, section in raw.get("settings", {}).items():
        where = f"settings.{name}"
        if not TYPE_NAME.match(name):
            raise ConfigError(f"{path}: 설정 이름 '{name}'은 PascalCase 식별자여야 한다")
        if name in tables:
            raise ConfigError(f"{path}: '{name}'이 표와 설정에 함께 있다")
        settings[name] = SettingConfig(
            name,
            require_str(section, "workbook", where),
            require_str(section, "sheet", where),
            require_modules(section, where),
        )

    enums: dict[str, dict[str, int]] = {}
    for name, values in raw.get("enums", {}).items():
        if not TYPE_NAME.match(name):
            raise ConfigError(f"{path}: 열거형 이름 '{name}'은 PascalCase 식별자여야 한다")
        if not isinstance(values, dict) or not values:
            raise ConfigError(f"{path}: 열거형 '{name}'에 값이 없다")
        for value_name, number in values.items():
            if not ENUM_VALUE_NAME.match(value_name):
                raise ConfigError(f"{path}: 열거형 '{name}'의 값 이름 '{value_name}'은 대문자와 밑줄로 쓴다")
            if not isinstance(number, int) or isinstance(number, bool) or number <= 0:
                raise ConfigError(f"{path}: 열거형 '{name}'의 '{value_name}' 번호는 1 이상의 정수다")
        if len(set(values.values())) != len(values):
            raise ConfigError(f"{path}: 열거형 '{name}'의 값 번호가 겹친다")
        enums[name] = dict(values)

    return Config(
        source_dir=root / require_str(raw, "source_dir", "최상위"),
        proto_dir=root / require_str(raw, "proto_dir", "최상위"),
        modules=modules,
        tables=tables,
        settings=settings,
        enums=enums,
        client=client,
    )
