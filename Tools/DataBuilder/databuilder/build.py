"""DataBuilder의 한 번 실행. 원본을 모두 검사하고, 임시 폴더에 생성한 뒤, 모두 성공해야 산출물 폴더에 배포한다.

검사가 하나라도 실패하면 산출물 폴더를 하나도 건드리지 않는다. 배포할 때는 모듈의 산출물 폴더를 비우고 다시 쓴다.
"""

from __future__ import annotations

import shutil
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

from databuilder.config import Config, ConfigError, load_config
from databuilder.emit_client import check_client, client_outputs
from databuilder.emit_cpp import HEADER_FILE, module_header
from databuilder.emit_json import VERSION_FILE, data_version, setting_json, table_json, version_json
from databuilder.enums import EnumError, collect_enums
from databuilder.rules import check_rules
from databuilder.workbooks import read_design_data


@dataclass
class BuildResult:
    errors: list[str] = field(default_factory=list)
    # 산출물을 막지 않는 규칙 위반. 오류가 없으면 경고가 있어도 배포한다.
    warnings: list[str] = field(default_factory=list)
    # 산출물 폴더 → 파일 이름 → 내용. 오류가 있으면 비어 있다.
    outputs: dict[Path, dict[str, str]] = field(default_factory=dict)
    version: str = ""


def generate(config: Config) -> BuildResult:
    """원본을 읽어 검사하고 산출물의 내용을 만든다. 파일은 쓰지 않는다."""
    result = BuildResult()
    try:
        enums = collect_enums(config.proto_dir, config.enums)
    except (EnumError, OSError) as error:
        result.errors.append(str(error))
        return result

    data = read_design_data(config, enums, result.errors)
    if result.errors:
        return result
    check_rules(data, result.errors, result.warnings)
    if config.client is not None:
        check_client(data, result.errors)
    if result.errors:
        return result

    result.version = data_version(data)
    for module in config.modules.values():
        tables = [data.tables[name] for name, table in config.tables.items() if module.name in table.modules]
        settings = [data.settings[name] for name, setting in config.settings.items() if module.name in setting.modules]
        files = {VERSION_FILE: version_json(result.version), HEADER_FILE: module_header(module.namespace, tables, settings, data)}
        for table in tables:
            if not table.config.nest:  # 중첩한 표는 부모 표의 파일에 실린다.
                files[f"{table.name}.json"] = table_json(table, data)
        for setting in settings:
            files[f"{setting.name}.json"] = setting_json(setting)
        result.outputs[module.server_dir] = files
    if config.client is not None:
        result.outputs.update(client_outputs(config.client, data, result.version))
    return result


def deploy(outputs: dict[Path, dict[str, str]]) -> None:
    """임시 폴더에 모두 쓴 뒤, 산출물 폴더를 비우고 임시 폴더의 내용으로 채운다."""
    with tempfile.TemporaryDirectory(prefix="databuilder-") as staging_root:
        staged: list[tuple[Path, Path]] = []
        for index, (out_dir, files) in enumerate(outputs.items()):
            staging = Path(staging_root) / str(index)
            staging.mkdir()
            for name, text in files.items():
                # 줄바꿈을 LF로 고정한다. 실행한 머신에 따라 산출물의 바이트가 달라지지 않게 한다.
                (staging / name).write_bytes(text.encode("utf-8"))
            staged.append((staging, out_dir))

        for staging, out_dir in staged:
            if out_dir.exists():
                shutil.rmtree(out_dir)
            shutil.copytree(staging, out_dir)


def run(config_path: Path, root: Path) -> int:
    try:
        config = load_config(config_path, root)
    except ConfigError as error:
        print(f"[DataBuilder] {error}", file=sys.stderr)
        return 1

    result = generate(config)
    for warning in result.warnings:
        print(f"[DataBuilder] 경고: {warning}", file=sys.stderr)
    if result.errors:
        for error in result.errors:
            print(f"[DataBuilder] {error}", file=sys.stderr)
        print(f"[DataBuilder] 오류 {len(result.errors)}건. 산출물을 바꾸지 않았다.", file=sys.stderr)
        return 1

    deploy(result.outputs)
    for out_dir, files in result.outputs.items():
        print(f"[DataBuilder] {out_dir.relative_to(root).as_posix()}/ ← {', '.join(sorted(files))}")
    print(f"[DataBuilder] 데이터 버전 {result.version}")
    return 0
