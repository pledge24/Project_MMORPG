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

    # 2) 위험 명령 패턴 검사
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
