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

import json
import os
import re
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

# 언리얼 엔진 MCP 서버의 라우터 툴 — 이 이름 하나로 수백 종이 전부 통과한다.
#
# 왜 여기서만 막을 수 있는가: 이 서버는 MCP 수준에 도구를 3종만 노출한다
# (list_toolsets, describe_toolset, call_tool). 실제 도구는 call_tool 의 인자에
# toolset_name 과 tool_name 으로 실려서 들어온다. 그래서 ADR-0002 가 쓴 수단인
# settings.json 의 permissions 로는 도구를 하나도 구분해 낼 수 없다 — 이름이
# mcp__unreal__call_tool 하나뿐이라 전부 허용하거나 전부 막는 선택만 남는다.
# 세분화가 가능한 지점은 인자를 실제로 읽는 이 훅뿐이다. 근거는 ADR-0003.
UE_ROUTER_TOOLS = {
    "mcp__unreal__call_tool",
}

# 통과시킬 (toolset_name, tool_name) 조합. **명단에 없으면 막는다.**
#
# 왜 허용 명단인가: ADR-0002 의 차단 명단은 Rider 처럼 도구가 개별로 노출될 때만 성립한다.
# 여기서는 엔진이 업데이트되면 툴셋과 도구가 조용히 늘어나고, 그것들이 전부 같은 이름으로
# 들어오므로 차단 명단은 반드시 뒤처진다. 방향을 뒤집어 모르는 것은 막는다.
#
# 범위는 두 가지 기준으로 정한다.
#   1. 테스트 판정은 헤드리스 커맨드렛이 한다 (P1/Scripts/Run-UeTests.ps1). MCP 는 그게
#      깨졌을 때 로그와 화면을 보는 진단 경로다. 그래서 조회 계열은 넓게 연다.
#   2. 되돌릴 수 없는 것은 열지 않는다. P1/Content 는 디스크의 3,438 개 중 47 개만 git 이
#      추적하고 .uasset 은 diff 도 안 된다 (2026-09-17 실측). 그래서 에셋 쓰기, 특히
#      폴더를 통째로 받는 delete 와 move 는 뺀다. LFS 전환 후에 다시 판단한다.
#
# 아래 둘은 열면 이 명단 자체가 무의미해지므로 따로 못 박아 둔다.
#   - ProgrammaticToolset.execute_tool_script: 스크립트 안에서 다른 도구를 부르는 것이
#     이 도구의 기능이다. 훅에는 호출 1 건으로 보이고 무엇을 불렀는지는 보이지 않는다.
#   - SlateInspector 의 조작 8 종(Click, Type, PressKey, SelectOption, FillForm, Drag,
#     Hover, Windows): 에디터 UI 로 사람이 할 수 있는 전부가 가능해진다. 우회 범위가
#     ProgrammaticToolset 보다 넓다. Click(ref="w123") 은 기록에 남아도 의미를 복원할 수 없다.
#
# 빈 문자열 키는 toolset_name 을 생략하고 최상위 도구를 부르는 경우다.
UE_ALLOWED_TOOLS = {
    "": {
        "list_toolsets",
        "describe_toolset",
    },
    # 오브젝트 속성 조회 — Rider 의 get_asset_properties 가 빈 결과만 내던 자리를 메운다.
    "editor_toolset.toolsets.object.ObjectTools": {
        "list_properties",
        "get_properties",
        "get_class",
        "search_subclasses",
    },
    # 블루프린트 조회. 같은 툴셋의 쓰기 도구는 의도적으로 뺐다.
    "editor_toolset.toolsets.blueprint.BlueprintTools": {
        "get_parent",
        "get_default_object",
        "list_variables",
        "get_variable_category",
        "get_variable_replication",
        "list_event_dispatchers",
        "list_events",
        "list_functions",
        "list_graphs",
        "get_graph",
        "read_graph_dsl",
        "get_graph_dsl_docs",
        "find_nodes",
        "find_node_types",
        "find_node_categories",
        "get_node_infos",
        "get_node_type_pins",
        "get_connected_subgraph",
        "get_pin_value",
        "list_component_events",
        "list_compatible_event_functions",
        "get_create_event_function",
    },
    # 자동화 테스트. 실행 도구까지 넣은 것은 의도적이다 — UE 클라 테스트를 무인으로 돌리는
    # 것이 이 서버를 들인 목적 중 하나다(docs/backlog.md 의 L1 항목). 이 경로가 실제로 도는
    # 것은 2026-09-16 에 확인했다. RunTests 가 통과·실패 개수를 JSON 으로 돌려준다.
    "AutomationTestToolset.AutomationTestToolset": {
        "DiscoverTests",
        "ListTests",
        "GetTestStatus",
        "GetTestResults",
        "RunTests",
        "RunTestsByFilter",
        "StopTests",
    },
    # 에디터 로그. 테스트나 에디터가 깨졌을 때 원인을 사람 손을 거치지 않고 읽는다.
    # SetVerbosity 는 조회가 아니지만 메모리상의 로그 상세도만 바꾼다. 에셋이나 설정 파일에
    # 남지 않는다.
    "EditorToolset.LogsToolset": {
        "GetLogEntries",
        "GetLogCategories",
        "GetVerbosity",
        "SetVerbosity",
    },
    # 에디터 상태 조회와 화면 캡처, 그리고 PIE 제어.
    # StartPIE 와 StopPIE 는 조회가 아니다. 넣은 이유는 이것이 열려야 UE 클라가 게임 서버와
    # 인증 서버에 실제로 붙는 3 티어 통합 스모크가 무인으로 돌기 때문이다. 에셋은 바꾸지
    # 않는다. 사람이 보고 있는 화면을 말없이 바꾸는 UI 조작 6 종은 뺐다
    # (SetContentBrowserPath, SetCameraTransform, SelectAssets, SelectActors,
    #  OpenEditorForAsset, FocusOnActors).
    "EditorToolset.EditorAppToolset": {
        "CaptureViewport",
        "CaptureEditorImage",
        "CaptureAssetImage",
        "GetCameraTransform",
        "GetContentBrowserPath",
        "GetOpenAssets",
        "GetSelectedActors",
        "GetSelectedAssets",
        "GetVisibleActors",
        "IsPIERunning",
        "ScreenCoordsToWorld",
        "SearchCVars",
        "WorldPosToScreenCoords",
        "StartPIE",
        "StopPIE",
    },
    # 에디터 UI 를 읽는다. MCP 도구로 노출되지 않는 패널 내용이 여기서 보인다.
    # 조작 8 종은 위 주석의 이유로 뺐다. Observe 는 100 밀리초마다 서브트리를 걷는 관찰자를
    # 남기므로 쓴 뒤에는 Unobserve 로 정리한다.
    "SlateInspectorToolset.SlateInspectorToolset": {
        "Snapshot",
        "Screenshot",
        "Observe",
        "Unobserve",
        "ListObservers",
        "WaitFor",
    },
    # 에셋 조회. get_referencers 와 get_dependencies 가 "이걸 고치면 뭐가 깨지나"에 답한다.
    # 쓰기 3 종(create_folder, move, save_assets)을 2026-09-21 에 열었다. #52 의 Content
    # 도메인 트리 재배치를 에이전트가 직접 수행하기 위해서다. 나머지 쓰기 4 종
    # (write_file, update_metadata_tags, delete, duplicate)은 계속 막는다.
    # **move 는 에셋 하나와 폴더 통째를 같은 인자로 받는다.** 경로를 한 글자 틀리면
    # 수천 개가 한 번에 움직인다. 부르기 전에 exists 로 양쪽을 확인한다.
    "editor_toolset.toolsets.asset.AssetTools": {
        "can_edit_asset",
        "create_folder",
        "exists",
        "find_assets",
        "get_asset_class",
        "get_asset_tags",
        "get_dependencies",
        "get_metadata_tags",
        "get_plugin_content_paths",
        "get_referencers",
        "is_checked_out",
        "is_dirty",
        "list_folders",
        "load_asset",
        "move",
        "read_file",
        "save_assets",
    }, 
    # 프로젝트 스킬 에셋 조회. CreateSkill 과 UpdateSkill 은 뺐다.
    "ToolsetRegistry.AgentSkillToolset": {
        "ListSkills",
        "GetSkills",
    },
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

    # 2) 언리얼 MCP 라우터 — 허용 명단에 있는 조합만 통과시킨다
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
        allowed = UE_ALLOWED_TOOLS.get(toolset)
        if allowed is None or target not in allowed:
            shown = (toolset + "." + target) if toolset else target
            return deny(
                "BLOCKED: 언리얼 MCP 도구 '" + shown + "' 는 허용 명단에 없다. "
                "이 서버는 도구 수백 종이 call_tool 하나로 들어와서 permissions 로 구분되지 않는다. "
                "그래서 이 훅의 UE_ALLOWED_TOOLS 가 유일한 통제 지점이고, 모르는 것은 막는다 "
                "(ADR-0003). 쓰기 계열이 필요하면 사람 승인을 받고 명단에 먼저 추가할 것. "
                "허용된 조합은 describe_toolset 으로 확인할 수 있다.",
                tool_name,
                ["UE 허용 명단 밖: " + shown],
            )
        _record_ue_call(toolset, target, tool_input.get("arguments"))
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
