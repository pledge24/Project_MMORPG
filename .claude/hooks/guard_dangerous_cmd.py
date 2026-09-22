#!/usr/bin/env python3
"""PreToolUse 훅 — CLAUDE.md '안전' 규칙을 실제로 강제한다.

왜 별도 파일인가:
  settings.json에 `python3 -c "..."` 한 줄로 넣었던 이전 버전은 이 환경에서 동작하지 않았다.
  Windows의 `python3`는 MS Store 앱 실행 별칭 스텁이라 스크립트를 실행하지 않고 exit 49로 죽는다.
  PreToolUse는 exit 2만 차단이고 그 외 non-zero는 non-blocking error라, 훅이 매번 조용히 실패하면서
  `rm -rf`가 그대로 실행됐다. 인터프리터를 `py -3`(C:\\Windows\\py.exe, 실제 exe)로 바꾸고
  로직을 파일로 빼서 JSON 이스케이프 문제까지 함께 없앤다.

동작:
  stdin으로 PreToolUse JSON을 받아 tool_name에 맞는 필드를 골라 검사한다.
  위반이면 permissionDecision=deny JSON을 stdout에 쓰고 exit 2로 끝낸다.
  (exit 2는 JSON 파싱 결과와 무관하게 차단되므로 이중 안전장치다.)
  차단 1건마다 block_counter.log 에 TSV 한 줄을 append 한다 (아래 COUNTER_ENV 주석 참조).
  언리얼 MCP 호출은 통과시킨 것도 ue_audit.log 에 인자 요약과 함께 남긴다 (AUDIT_ENV 주석 참조).

테스트: py -3 .claude/hooks/test_guard_dangerous_cmd.py
"""

import ast
import json
import os
import re
import subprocess
import sys
from datetime import datetime
from pathlib import Path

# 차단 카운터 — 차단이 일어난 그 자리에서 한 줄씩 append 한다.
#
# 왜 여기인가: 차단 건수는 소급 생성이 불가능한 데이터다. 지나간 차단은 다시 셀 수 없으므로
# 세션 리포트에 손으로 누적하는 방식은 부패한다(집계를 잊은 세션은 영구 결손).
# 발생 지점에서 자동으로 쌓아두고, 리포트는 이 파일을 조회만 한다.
#
# 경로를 환경변수로 덮어쓸 수 있게 둔 이유는 테스트다. 테스트가 실제 카운터를 오염시키면
# 숫자가 거짓이 된다.
COUNTER_ENV = "GUARD_BLOCK_COUNTER"
COUNTER_DEFAULT = Path(__file__).with_name("block_counter.log")

# 언리얼 MCP 감사 로그 — 통과시킨 호출도 한 줄씩 남긴다.
#
# 왜 필요한가: 위 카운터는 막힌 것만 센다. 그런데 사고는 막힌 호출이 아니라 통과한 호출에서
# 난다. 무엇이 통과했는지 기록이 없으면 "사고가 나면 그때 대응한다"는 방침 자체가 성립하지
# 않는다. 되짚을 근거가 없기 때문이다.
#
# 에디터 로그도 LogModelContextProtocol 항목으로 디스패치를 남기지만 둘이 다르다. 그쪽은
# 인자를 적지 않고, P1/Saved 에 있어 세션마다 새로 시작하며, 저장소에 남지 않는다.
# 여기서는 인자 요약까지 남기고 파일이 계속 누적된다.
AUDIT_ENV = "GUARD_UE_AUDIT"
AUDIT_DEFAULT = Path(__file__).with_name("ue_audit.log")
# 인자 요약 길이 상한. refPath 하나가 200자를 넘는 경우가 있어 넉넉히 잡되, 스크립트나
# 긴 배열이 통째로 들어와 로그가 부풀지 않도록 자른다.
AUDIT_ARGS_MAX = 400

# 저장소 루트 — 슬레이트 조작 전에 git status 를 읽을 때 쓴다.
# 환경변수로 덮어쓸 수 있게 둔 이유는 위 둘과 같다. 테스트가 실제 저장소를 보면
# 판정이 그날의 워킹 트리 상태에 따라 흔들린다.
REPO_ENV = "GUARD_REPO_ROOT"
REPO_DEFAULT = Path(__file__).resolve().parent.parent.parent

# 어떤 툴의 어느 필드를 볼 것인가.
# 셸 계열은 command, Rider의 SQL 실행은 queryText.
SHELL_TOOLS = {
    "Bash",
    "PowerShell",
    "mcp__rider__execute_terminal_command",
}
SQL_TOOLS = {
    "mcp__rider__execute_sql_query",
}

# rootFolder 없이 부르면 안 되는 Rider MCP 툴 — 상태를 바꾸는 것들만.
#
# 왜 필요한가: Rider 인스턴스 하나가 열린 솔루션 전부(P1, Server)를 한 엔드포인트로 서빙하고,
# 어느 쪽을 대상으로 할지는 rootFolder 가 정한다. 솔루션이 둘 이상 열려 있으면 서버가
# "Unable to determine the target project"로 거부해 주지만, **하나만 열려 있으면 거부하지 않는다.**
# Server를 빌드하려 했는데 P1만 열려 있으면 그대로 P1을 빌드한다. 그 구멍은 여기서만 막힌다.
#
# 읽기 툴은 넣지 않는다 — 탐색할 때마다 rootFolder를 요구하면 실익 없이 번거롭기만 하다.
ROOT_FOLDER_REQUIRED = {
    # 빌드 · 실행
    "mcp__rider__build_solution_start",
    "mcp__rider__execute_run_configuration",
    "mcp__rider__execute_terminal_command",
    # 파일 · 심볼 수정
    "mcp__rider__apply_patch",
    "mcp__rider__create_new_file",
    "mcp__rider__rename_refactoring",
    "mcp__rider__safe_delete",
    "mcp__rider__move_type_to_namespace",
    "mcp__rider__reformat_file",
    "mcp__rider__reorganize_namespaces",
    "mcp__rider__change_api_signature",
    "mcp__rider__extract_base_class",
    "mcp__rider__extract_interface",
    "mcp__rider__extract_method",
    # 언리얼 에디터 상태 변경
    "mcp__rider__ue_execute_python",
    "mcp__rider__ue_import_blueprint_nodes",
    "mcp__rider__ue_play",
    "mcp__rider__spawn_actor",
    # DB 연결 설정 변경
    "mcp__rider__create_database_connection",
    "mcp__rider__edit_database_connection",
}

# 언리얼 엔진 MCP 서버의 라우터 툴 — 이 이름 하나로 830 종이 전부 통과한다.
#
# 이 서버는 MCP 수준에 도구를 3종만 노출하고(list_toolsets, describe_toolset, call_tool)
# 실제 도구는 call_tool 의 인자로 실려 온다. settings.json 의 permissions 는 이름
# 하나만 보므로 전부 허용과 전부 차단 두 가지뿐이다. 판정이 가능한 지점은 인자를 읽는
# 이 훅뿐이다. 근거와 판정 층의 설계 의도는 ADR-0003.
UE_ROUTER_TOOLS = {
    "mcp__unreal__call_tool",
}

# 판정은 네 층이다. 위에서 걸리면 거기서 막는다.
#   1층 UE_ALLOWED_TOOLSETS  툴셋이 열려 있는가. 모르는 툴셋은 막는다
#   2층 UE_DENIED_TOOLS      그 안에서 개별 도구를 막는가
#   3층 UE_EXTERNAL_RE       조회가 아닌 도구가 참고용 보관소를 인자로 받았는가
#   4층                      슬레이트 조작과 execute_tool_script 의 전용 판정

# toolset_name 없이 부르는 최상위 도구. call_tool 자신은 재귀가 되므로 뺀다.
UE_TOP_LEVEL_TOOLS = {
    "list_toolsets",
    "describe_toolset",
}

# 1층 — 열어 둔 툴셋. 이 안의 도구는 2층부터의 판정만 거친다.
UE_ALLOWED_TOOLSETS = {
    "editor_toolset.toolsets.asset.AssetTools",
    "editor_toolset.toolsets.object.ObjectTools",
    "editor_toolset.toolsets.texture.TextureTools",
    "editor_toolset.toolsets.blueprint.BlueprintTools",
    "editor_toolset.toolsets.material.MaterialTools",
    "editor_toolset.toolsets.material_instance.MaterialInstanceTools",
    "editor_toolset.toolsets.static_mesh.StaticMeshTools",
    "editor_toolset.toolsets.skeletal_mesh.SkeletalMeshTools",
    "editor_toolset.toolsets.data_table.DataTableTools",
    "editor_toolset.toolsets.data_asset.DataAssetTools",
    "editor_toolset.toolsets.curve_table.CurveTableTools",
    "editor_toolset.toolsets.string_table.StringTableTools",
    "editor_toolset.toolsets.scene.SceneTools",
    "editor_toolset.toolsets.actor.ActorTools",
    "editor_toolset.toolsets.primitive.PrimitiveTools",
    "editor_toolset.toolsets.programmatic.ProgrammaticToolset",
    "UMGToolSet.UMGToolSet",
    "PhysicsToolsets.PhysicsAssetToolset",
    "EditorToolset.EditorAppToolset",
    "EditorToolset.LogsToolset",
    "AutomationTestToolset.AutomationTestToolset",
    "SlateInspectorToolset.SlateInspectorToolset",
    "ToolsetRegistry.AgentSkillToolset",
    "SemanticSearchToolset.SemanticSearchToolset",
    "ConfigSettingsToolset.ConfigSettingsToolset",
    "PluginToolset.PluginToolset",
    "GameFeaturesToolset.GameFeaturesToolset",
}

# 2층 — 열린 툴셋 안에서 개별로 막을 도구. {툴셋: {도구, ...}}.
# 비어 있는 것이 정상이다. 사고가 나면 여기에 한 줄 적어 막는다.
UE_DENIED_TOOLS = {}

# 3층 — 조회로 볼 도구 이름의 접두사. 서버가 snake_case 와 PascalCase 를 섞어 쓰므로
# 소문자로 낮춰 비교한다.
#
# 이름 규칙을 안 따르는 조회 도구(Snapshot 등)는 쓰기로 분류되지만, 3층은 인자에 보관소
# 경로가 있을 때만 발동하므로 실제로 막히지 않는다.
UE_READONLY_PREFIXES = (
    "get", "list", "find", "search", "query", "read", "load", "describe", "discover",
    "is", "can", "has", "exists", "preview", "inspect", "snapshot", "screenshot",
    "capture", "observe", "unobserve", "waitfor",
)

# 참고용 보관소. 콘텐츠 경로와 디스크 경로 양쪽을 잡는다 (ADR-0006).
UE_EXTERNAL_RE = re.compile(r"/Game/External\b|[/\\]Content[/\\]External\b", re.IGNORECASE)

# 3층이 검사하지 않을 인자. 보관소를 **읽기만** 하는 출발지다.
#
# duplicate 는 원본을 남기고 import_file 은 디스크의 이미지를 읽기만 한다. 목적지
# 인자(new_path, folder_path)는 그대로 검사하므로 보관소를 목적지로 쓰는 것은 막힌다.
# move 는 출발지를 비우므로 여기 넣지 않는다.
#
# **이 예외는 직접 호출에만 적용된다.** execute_tool_script 안의 호출은 인자가 JSON
# 문자열이라 어느 쪽이 목적지인지 정적으로 가릴 수 없다. 그래서 스크립트로 묶지 않고
# 한 건씩 부른다.
UE_EXTERNAL_SOURCE_ARGS = {
    "duplicate": ("path",),
    "import_file": ("source_file",),
}

# 4층 — 슬레이트 조작. 막지 않는 대신 워킹 트리가 깨끗할 때만 통과시킨다.
# ref 는 익명이고 관찰자가 계속 재할당하므로 Click(ref="i1") 은 로그로 복원되지 않는다.
# 직전 상태가 커밋되어 있으면 결과를 diff 로 읽을 수 있다.
UE_SLATE_TOOLSET = "SlateInspectorToolset.SlateInspectorToolset"
UE_SLATE_MUTATORS = {
    "Click", "Type", "PressKey", "SelectOption", "FillForm", "Drag", "Hover", "Windows",
}

# 4층 — execute_tool_script 는 스크립트 안에서 다른 도구를 부르는 것이 기능이라
# 그대로 열면 위 세 층을 건너뛴다. 스크립트를 ast 로 파싱해 호출을 추출하고 같은
# 판정을 적용한다. 규약은 execute_tool(전체이름, JSON문자열).
UE_PROG_TOOLSET = "editor_toolset.toolsets.programmatic.ProgrammaticToolset"
UE_PROG_SCRIPT_TOOL = "execute_tool_script"

# 샌드박스가 import 를 여섯 모듈로 제한하지만 빌트인은 별개다. 이것들이 살아 있으면
# 정적 판정을 건너뛸 수 있으므로 여기서 막는다.
UE_SCRIPT_FORBIDDEN = {
    "eval", "exec", "compile", "__import__", "globals", "locals", "vars",
    "getattr", "setattr", "delattr", "open", "input", "breakpoint",
}


# 셸에서 막을 것 — 파괴적 파일/git 조작 + Redis 전체 삭제.
SHELL_PATTERNS = [
    (r"rm\s+-[a-zA-Z]*r[a-zA-Z]*f|rm\s+-[a-zA-Z]*f[a-zA-Z]*r", "rm -rf"),
    (r"\brd\s+/s\b|\brmdir\s+/s\b", "rd /s (재귀 삭제)"),
    (r"\bdel\s+/[a-zA-Z]*s\b", "del /s (재귀 삭제)"),
    (r"Remove-Item\b(?=[\s\S]*-Recurse)(?=[\s\S]*-Force)", "Remove-Item -Recurse -Force"),
    (r"git\s+push\s+.*(--force\b|(?<!-)-f\b)", "git push --force"),
    (r"git\s+reset\s+--hard", "git reset --hard"),
    (r"git\s+clean\s+.*-[a-zA-Z]*f", "git clean -f"),
    (r"git\s+checkout\s+\.(\s|$)", "git checkout ."),
    (r"git\s+restore\s+\.(\s|$)", "git restore ."),
    (r"git\s+branch\s+-D\b", "git branch -D"),
    (r"\bFLUSHALL\b", "Redis FLUSHALL"),
    (r"\bFLUSHDB\b", "Redis FLUSHDB"),
    (r"\bKEYS\s+['\"]?\*", "Redis KEYS *"),
]

# 스키마·데이터를 통째로 날리는 SQL. 셸(sqlcmd 등)과 SQL 툴 양쪽에 적용한다.
SQL_DDL_PATTERNS = [
    (r"\bDROP\s+(TABLE|DATABASE|SCHEMA|PROCEDURE|VIEW|INDEX)\b", "DROP"),
    (r"\bTRUNCATE\s+TABLE\b", "TRUNCATE TABLE"),
    (r"\bALTER\s+TABLE\b", "ALTER TABLE (스키마 변경은 사람 승인 후)"),
    (r"\bCREATE\s+(TABLE|DATABASE|SCHEMA)\b", "CREATE TABLE/DATABASE (스키마 변경은 사람 승인 후)"),
]


def _strip_sql_noise(text):
    """WHERE 유무를 판정하기 전에 주석과 문자열 리터럴을 지운다.

    주석 안의 WHERE를 진짜 WHERE로 오인해 통과시키는 일을 막기 위함이다.
    """
    text = re.sub(r"--[^\n]*", " ", text)
    text = re.sub(r"/\*[\s\S]*?\*/", " ", text)
    text = re.sub(r"'(?:[^']|'')*'", " '' ", text)
    return text


def check(text, is_sql):
    """위반 사유 목록을 돌려준다. 비어 있으면 통과."""
    hits = []

    if not is_sql:
        for pattern, label in SHELL_PATTERNS:
            if re.search(pattern, text, re.IGNORECASE):
                hits.append(label)

    for pattern, label in SQL_DDL_PATTERNS:
        if re.search(pattern, text, re.IGNORECASE):
            hits.append(label)

    # WHERE 없는 DELETE/UPDATE. 셸 문자열에서는 오탐이 잦아 SQL 툴에만 적용한다.
    if is_sql:
        cleaned = _strip_sql_noise(text)
        has_where = re.search(r"\bWHERE\b", cleaned, re.IGNORECASE) is not None
        if re.search(r"\bDELETE\s+FROM\b", cleaned, re.IGNORECASE) and not has_where:
            hits.append("WHERE 없는 DELETE")
        if re.search(r"\bUPDATE\s+[\w\[\].]+\s+SET\b", cleaned, re.IGNORECASE) and not has_where:
            hits.append("WHERE 없는 UPDATE")

    return hits


def _record_block(tool_name, hits):
    """차단 1건을 카운터 파일에 append 한다. TSV 한 줄: 시각 / 툴 / 걸린 패턴.

    **실패해도 조용히 넘어간다.** 카운터는 통계일 뿐이고 차단이 본 임무다.
    로그를 못 쓴다는 이유로 위험 명령이 통과하면 주객이 전도된다.
    """
    try:
        override = os.environ.get(COUNTER_ENV)
        path = Path(override) if override else COUNTER_DEFAULT
        line = "\t".join((
            datetime.now().astimezone().isoformat(timespec="seconds"),
            tool_name or "(unknown)",
            ";".join(hits) if hits else "(unknown)",
        ))
        with open(path, "a", encoding="utf-8") as f:
            f.write(line + "\n")
    except Exception:
        pass


def _record_ue_call(toolset, tool, arguments):
    """통과시킨 언리얼 MCP 호출 1건을 감사 로그에 append 한다.

    TSV 한 줄: 시각 / 툴셋 / 도구 / 인자 요약.

    **실패해도 조용히 넘어간다.** 기록을 못 썼다는 이유로 허용 판정을 뒤집지 않는다.
    _record_block 과 같은 원칙이다.
    """
    try:
        override = os.environ.get(AUDIT_ENV)
        path = Path(override) if override else AUDIT_DEFAULT
        try:
            summary = json.dumps(arguments, ensure_ascii=False, sort_keys=True)
        except Exception:
            summary = repr(arguments)
        # TSV 한 줄을 깨뜨리는 문자만 치환한다.
        summary = summary.replace("\t", " ").replace("\r", " ").replace("\n", " ")
        if len(summary) > AUDIT_ARGS_MAX:
            summary = summary[:AUDIT_ARGS_MAX] + "...(잘림)"
        line = "\t".join((
            datetime.now().astimezone().isoformat(timespec="seconds"),
            toolset or "(top-level)",
            tool or "(unknown)",
            summary,
        ))
        with open(path, "a", encoding="utf-8") as f:
            f.write(line + "\n")
    except Exception:
        pass


def _ue_is_readonly(target):
    """도구 이름만 보고 조회인지 판정한다."""
    low = target.lower().lstrip("_")
    return any(low.startswith(p) for p in UE_READONLY_PREFIXES)


def _ue_touches_external(blob):
    """인자나 스크립트 본문이 참고용 보관소 경로를 담고 있는지 본다."""
    if blob is None:
        return False
    if not isinstance(blob, str):
        try:
            blob = json.dumps(blob, ensure_ascii=False)
        except Exception:
            blob = repr(blob)
    return bool(UE_EXTERNAL_RE.search(blob))


def _ue_guarded_args(target, arguments):
    """3층이 검사할 인자만 남긴다. 보관소를 읽기만 하는 출발지는 뺀다.

    인자가 dict 가 아니면(스크립트 본문 등) 그대로 돌려주므로 예외가 걸리지 않는다.
    """
    skip = UE_EXTERNAL_SOURCE_ARGS.get(target)
    if not skip or not isinstance(arguments, dict):
        return arguments
    return {k: v for k, v in arguments.items() if k not in skip}


def _ue_gate(toolset, target, arguments):
    """1~3층 판정. 막을 이유를 돌려주고, 통과면 빈 문자열을 돌려준다.

    스크립트 안의 호출도 이 함수를 쓴다. 판정이 두 곳으로 갈라지면 한쪽만 고쳐진다.
    """
    if not toolset:
        if target not in UE_TOP_LEVEL_TOOLS:
            return "최상위 도구 '" + target + "' 는 열려 있지 않다"
        return ""
    if toolset not in UE_ALLOWED_TOOLSETS:
        return "툴셋 '" + toolset + "' 는 1층 허용 명단에 없다"

    denied = UE_DENIED_TOOLS.get(toolset)
    if denied and target in denied:
        return "도구 '" + target + "' 는 2층 거부 명단에 있다"

    # execute_tool_script 는 3층을 건너뛴다. 인자를 통째로 훑는 이 검사는 스크립트
    # 본문에 보관소 경로가 한 번이라도 나오면 걸려서, 읽기만 하는 스크립트까지 막는다.
    # 4층이 호출을 하나씩 뜯어 조회와 쓰기를 가른다.
    if toolset == UE_PROG_TOOLSET and target == UE_PROG_SCRIPT_TOOL:
        return ""
    if not _ue_is_readonly(target) and _ue_touches_external(_ue_guarded_args(target, arguments)):
        return ("'" + target + "' 는 조회가 아닌데 인자가 Content/External 경로를 담고 "
                "있다. 참고용 보관소는 조회만 허용한다 (ADR-0006)")
    return ""


def _ue_git_is_clean():
    """워킹 트리가 깨끗한지 본다. 판정할 수 없으면 None."""
    root = os.environ.get(REPO_ENV) or str(REPO_DEFAULT)
    try:
        p = subprocess.run(
            ["git", "status", "--porcelain"], cwd=root,
            capture_output=True, text=True, encoding="utf-8", timeout=15,
        )
    except Exception:
        return None
    if p.returncode != 0:
        return None
    return not (p.stdout or "").strip()


def _ue_check_script(arguments):
    """execute_tool_script 의 스크립트를 정적 판정한다. 막을 이유를 돌려준다.

    런타임에 조합한 문자열은 잡지 못한다. 막으려는 것이 의도적 우회가 아니라 오판이다.
    """
    script = arguments.get("script") if isinstance(arguments, dict) else None
    if not isinstance(script, str) or not script.strip():
        return "script 인자가 비어 있거나 문자열이 아니다"
    try:
        tree = ast.parse(script)
    except SyntaxError as e:
        return "스크립트를 파싱할 수 없다: " + str(e)

    calls = []
    for node in ast.walk(tree):
        if isinstance(node, ast.Name) and node.id in UE_SCRIPT_FORBIDDEN:
            return "금지 식별자 '" + node.id + "' 를 쓴다"
        if isinstance(node, ast.Attribute) and node.attr.startswith("__"):
            return "던더 속성 '" + node.attr + "' 에 접근한다"
        if not isinstance(node, ast.Call):
            continue
        if not (isinstance(node.func, ast.Name) and node.func.id == "execute_tool"):
            continue
        if not node.args:
            return "execute_tool 을 인자 없이 부른다"
        first = node.args[0]
        if not (isinstance(first, ast.Constant) and isinstance(first.value, str)):
            return ("execute_tool 의 첫 인자가 문자열 리터럴이 아니다. 무엇을 부르는지 "
                    "판정할 수 없으므로 막는다")
        calls.append(first.value)

    # 조회에만 쓰인 경로인지 정적으로 가릴 수 없으므로, 보관소 경로가 본문에 있으면
    # 쓰기 호출 전체를 막는다.
    external = _ue_touches_external(script)
    for full in calls:
        sub_toolset, _, sub_target = full.rpartition(".")
        if not sub_toolset:
            sub_target = full
        if sub_toolset == UE_PROG_TOOLSET and sub_target == UE_PROG_SCRIPT_TOOL:
            return "스크립트가 execute_tool_script 를 다시 부른다"
        reason = _ue_gate(sub_toolset, sub_target, script if external else None)
        if reason:
            return "스크립트가 부르는 '" + full + "' 가 막힌다 — " + reason
    return ""


def deny(reason, tool_name="", hits=()):
    """차단 결정을 내보낸다.

    JSON(permissionDecision=deny)과 exit 2를 함께 쓴다. exit 2는 JSON 파싱 결과와
    무관하게 차단하므로, 스키마가 어긋나도 통과로 새지 않는다.

    카운터 기록을 먼저 시도하되 그 결과는 아래 흐름에 영향을 주지 않는다.
    """
    _record_block(tool_name, hits)

    sys.stdout.write(json.dumps({
        "hookSpecificOutput": {
            "hookEventName": "PreToolUse",
            "permissionDecision": "deny",
            "permissionDecisionReason": reason,
        }
    }, ensure_ascii=False))
    sys.stderr.write(reason)
    return 2


def main():
    try:
        payload = json.load(sys.stdin)
    except Exception:
        # 입력을 못 읽으면 판단할 근거가 없다. 차단하지 않고 통과시킨다
        # (여기서 exit 2를 내면 모든 툴 호출이 막힌다).
        return 0

    tool_name = payload.get("tool_name", "")
    tool_input = payload.get("tool_input", {}) or {}

    # 1) 대상 솔루션이 명시됐는가 (Rider MCP 상태 변경 툴 한정)
    if tool_name in ROOT_FOLDER_REQUIRED:
        root = tool_input.get("rootFolder")
        if not isinstance(root, str) or not root.strip():
            return deny(
                "BLOCKED: " + tool_name + " 를 rootFolder 없이 호출했다. "
                "Rider 하나가 열린 솔루션 전부를 서빙하므로 대상을 명시해야 한다. "
                "솔루션이 하나만 열려 있으면 서버가 거부하지 않고 그대로 실행하니 "
                "의도한 솔루션이 실제로 열려 있는지도 확인할 것 "
                "(인자 없이 get_run_configurations 를 부르면 열린 프로젝트 목록이 나온다).",
                tool_name,
                ["rootFolder 누락"],
            )

    # 2) 언리얼 MCP 라우터 — 네 층으로 판정한다 (ADR-0003)
    if tool_name in UE_ROUTER_TOOLS:
        toolset = tool_input.get("toolset_name") or ""
        target = tool_input.get("tool_name") or ""
        if not isinstance(toolset, str) or not isinstance(target, str):
            return deny(
                "BLOCKED: call_tool 의 toolset_name/tool_name 이 문자열이 아니다. "
                "판정할 근거가 없으므로 막는다.",
                tool_name,
                ["인자 형식 오류"],
            )
        toolset = toolset.strip()
        target = target.strip()
        arguments = tool_input.get("arguments")
        shown = (toolset + "." + target) if toolset else target

        reason = _ue_gate(toolset, target, arguments)
        if not reason and toolset == UE_SLATE_TOOLSET and target in UE_SLATE_MUTATORS:
            # 판정할 수 없으면 막는 쪽으로 기운다. 커밋 상태를 모르면 조작 결과를
            # diff 로 읽을 수 있다는 전제가 깨진다.
            clean = _ue_git_is_clean()
            if clean is False:
                reason = "워킹 트리에 커밋되지 않은 변경이 있다. 조작 전에 커밋할 것"
            elif clean is None:
                reason = "git status 를 읽지 못해 커밋 상태를 알 수 없다"
        if not reason and toolset == UE_PROG_TOOLSET and target == UE_PROG_SCRIPT_TOOL:
            reason = _ue_check_script(arguments)

        if reason:
            return deny(
                "BLOCKED: 언리얼 MCP 도구 '" + shown + "' — " + reason + ". "
                "이 서버는 도구 830 종이 call_tool 하나로 들어와 permissions 로는 "
                "구분되지 않으므로 이 훅이 유일한 통제 지점이다 (ADR-0003). "
                "열려 있는 툴셋은 list_toolsets 로 확인할 수 있다.",
                tool_name,
                ["UE 판정: " + shown],
            )

        _record_ue_call(toolset, target, arguments)
        return 0

    # 3) 위험 명령 패턴 검사
    if tool_name in SQL_TOOLS:
        target = tool_input.get("queryText", "")
        is_sql = True
    elif tool_name in SHELL_TOOLS:
        target = tool_input.get("command", "")
        is_sql = False
    else:
        return 0

    if not isinstance(target, str) or not target.strip():
        return 0

    hits = check(target, is_sql)
    if not hits:
        return 0

    unique = list(dict.fromkeys(hits))
    return deny(
        "BLOCKED: 위험 명령 감지 (" + ", ".join(unique) + "). "
        "CLAUDE.md '안전' 규칙에 걸린다. 정말 필요하면 사람에게 직접 실행을 요청할 것.",
        tool_name,
        unique,
    )


if __name__ == "__main__":
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(encoding="utf-8")
        except Exception:
            pass
    sys.exit(main())
