#!/usr/bin/env python3
"""guard_dangerous_cmd.py 단위 테스트.

실행: py -3 .claude/hooks/test_guard_dangerous_cmd.py

훅을 import 하지 않고 실제 프로세스로 띄운다. 검증 대상이 로직이 아니라 **종료 코드**이기
때문이다 — PreToolUse 는 exit 2 만 차단으로 취급하므로, 함수가 옳게 판단해도 프로세스가
0으로 끝나면 훅은 아무것도 막지 못한다. 이전에 인터프리터(`python3` 별칭 스텁)가 exit 49 로
죽으면서 훅이 조용히 무력화된 전례가 있다. 그래서 프로세스 경계까지 포함해 검사한다.

카운터 테스트는 GUARD_BLOCK_COUNTER 로 임시 경로를 주입해 실제 block_counter.log 를
건드리지 않는다. 테스트가 통계를 오염시키면 숫자가 거짓이 된다.
"""

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
HOOK = HERE / "guard_dangerous_cmd.py"
REPO = HERE.parent.parent
SRV = str(REPO / "Server")

RUN = ["py", "-3", str(HOOK)]


def run_hook(tool, tool_input, env=None):
    """훅을 한 번 실행하고 (returncode, stdout) 을 돌려준다."""
    merged = dict(os.environ)
    if env:
        merged.update(env)
    p = subprocess.run(
        RUN,
        input=json.dumps({"tool_name": tool, "tool_input": tool_input}),
        capture_output=True, text=True, encoding="utf-8", env=merged,
    )
    return p.returncode, p.stdout


# (설명, tool_name, tool_input, 차단되어야 하는가)
CASES = [
    # --- 셸 위험 패턴 ---
    ("rm -rf", "Bash", {"command": "rm -rf /tmp/x"}, True),
    ("rm -f 단독", "Bash", {"command": "rm -f notes.txt"}, False),
    ("git push --force", "Bash", {"command": "git push --force origin dev"}, True),
    ("git push -f", "Bash", {"command": "git push -f"}, True),
    ("git reset --hard", "Bash", {"command": "git reset --hard origin/dev"}, True),
    ("git clean -fd", "Bash", {"command": "git clean -fd"}, True),
    ("git checkout .", "Bash", {"command": "git checkout ."}, True),
    ("git checkout .gitignore", "Bash", {"command": "git checkout .gitignore"}, False),
    ("git branch -D", "Bash", {"command": "git branch -D feature"}, True),
    ("FLUSHALL", "Bash", {"command": "redis-cli FLUSHALL"}, True),
    ("KEYS *", "Bash", {"command": "redis-cli KEYS *"}, True),
    ("git status", "Bash", {"command": "git status"}, False),
    ("ls && git log", "Bash", {"command": "ls -la && git log --oneline -5"}, False),
    ("PS 재귀삭제", "PowerShell", {"command": "Remove-Item -Recurse -Force C:\\tmp"}, True),
    ("PS 단일삭제", "PowerShell", {"command": "Remove-Item C:\\tmp\\a.txt"}, False),
    ("PS 재귀조회", "PowerShell", {"command": "Get-ChildItem -Recurse"}, False),

    # --- SQL ---
    ("SELECT", "mcp__rider__execute_sql_query", {"queryText": "SELECT * FROM Users WHERE user_id=1"}, False),
    ("WHERE 없는 DELETE", "mcp__rider__execute_sql_query", {"queryText": "DELETE FROM Characters"}, True),
    ("WHERE 있는 DELETE", "mcp__rider__execute_sql_query", {"queryText": "DELETE FROM Characters WHERE character_id=5"}, False),
    ("WHERE 없는 UPDATE", "mcp__rider__execute_sql_query", {"queryText": "UPDATE Users SET last_login=GETDATE()"}, True),
    ("WHERE 있는 UPDATE", "mcp__rider__execute_sql_query", {"queryText": "UPDATE Users SET last_login=GETDATE() WHERE username='a'"}, False),
    ("DROP TABLE", "mcp__rider__execute_sql_query", {"queryText": "DROP TABLE Users"}, True),
    ("TRUNCATE", "mcp__rider__execute_sql_query", {"queryText": "TRUNCATE TABLE Characters"}, True),
    ("ALTER TABLE", "mcp__rider__execute_sql_query", {"queryText": "ALTER TABLE Characters ADD col INT"}, True),
    ("주석 속 WHERE", "mcp__rider__execute_sql_query", {"queryText": "DELETE FROM x -- WHERE y"}, True),

    # --- rootFolder 강제 ---
    ("빌드 rootFolder 없음", "mcp__rider__build_solution_start", {"rebuild": False}, True),
    ("빌드 rootFolder 빈문자", "mcp__rider__build_solution_start", {"rootFolder": ""}, True),
    ("빌드 rootFolder 공백만", "mcp__rider__build_solution_start", {"rootFolder": "   "}, True),
    ("빌드 rootFolder 있음", "mcp__rider__build_solution_start", {"rootFolder": SRV}, False),
    ("리네임 rootFolder 없음", "mcp__rider__rename_refactoring", {"symbolFqn": "Foo"}, True),
    ("리네임 rootFolder 있음", "mcp__rider__rename_refactoring", {"symbolFqn": "Foo", "rootFolder": SRV}, False),
    ("ue_python rootFolder 없음", "mcp__rider__ue_execute_python", {"script": "print(1)"}, True),
    ("실행구성 rootFolder 없음", "mcp__rider__execute_run_configuration", {"name": "GameServer"}, True),

    # --- 읽기 툴은 rootFolder 없이도 통과 ---
    ("읽기: 실행구성 조회", "mcp__rider__get_run_configurations", {}, False),
    ("읽기: read_file", "mcp__rider__read_file", {"filePath": "a.cpp"}, False),
    ("읽기: search_symbol", "mcp__rider__search_symbol", {"q": "Room"}, False),
    ("읽기: ue_health", "mcp__rider__ue_health", {}, False),

    # --- 터미널: 두 검사가 함께 걸린다 ---
    ("터미널 rootFolder 없음", "mcp__rider__execute_terminal_command", {"command": "ls"}, True),
    ("터미널 정상", "mcp__rider__execute_terminal_command", {"command": "ls", "rootFolder": SRV}, False),
    ("터미널 위험명령", "mcp__rider__execute_terminal_command", {"command": "rd /s /q build", "rootFolder": SRV}, True),

    # --- 관계없는 툴 ---
    ("Read 툴", "Read", {"file_path": "/etc/passwd"}, False),
]


def check_pattern_cases():
    """패턴·rootFolder 판정. 실패 개수를 돌려준다."""
    bad = 0
    # 카운터가 실제 로그를 건드리지 않도록 통째로 임시 경로로 돌린다.
    with tempfile.TemporaryDirectory() as tmp:
        env = {"GUARD_BLOCK_COUNTER": str(Path(tmp) / "noise.log")}
        for desc, tool, tin, expect in CASES:
            code, _ = run_hook(tool, tin, env)
            blocked = (code == 2)
            ok = (blocked == expect)
            if not ok:
                bad += 1
            print(("PASS " if ok else "FAIL ") + "[exit=%s] %-24s %s" % (code, desc, tool))
    return bad


def check_counter_cases():
    """카운터 동작. 실패 개수를 돌려준다."""
    bad = 0

    def report(ok, desc, detail=""):
        nonlocal bad
        if not ok:
            bad += 1
        print(("PASS " if ok else "FAIL ") + "%-24s %s" % (desc, detail))

    with tempfile.TemporaryDirectory() as tmp:
        log = Path(tmp) / "block_counter.log"
        env = {"GUARD_BLOCK_COUNTER": str(log)}

        # 1) 차단 1회 → 줄 1개, TSV 필드 3개
        code, _ = run_hook("Bash", {"command": "rm -rf /tmp/x"}, env)
        lines = log.read_text(encoding="utf-8").splitlines() if log.exists() else []
        fields = lines[0].split("\t") if lines else []
        report(
            code == 2 and len(lines) == 1 and len(fields) == 3
            and fields[1] == "Bash" and "rm -rf" in fields[2],
            "차단 1회 → 1줄 기록",
            "줄=%d 필드=%d %s" % (len(lines), len(fields), fields[1:] if fields else ""),
        )

        # 2) 통과는 세지 않는다
        run_hook("Bash", {"command": "git status"}, env)
        after = log.read_text(encoding="utf-8").splitlines()
        report(len(after) == 1, "통과는 기록 안 함", "줄=%d" % len(after))

        # 3) append-only — 두 번째 차단이 첫 줄을 덮어쓰지 않는다
        run_hook("mcp__rider__build_solution_start", {"rebuild": False}, env)
        after = log.read_text(encoding="utf-8").splitlines()
        report(
            len(after) == 2 and after[0] == lines[0]
            and "rootFolder 누락" in after[1],
            "두 번째 차단 → append",
            "줄=%d" % len(after),
        )

        # 4) 카운터를 못 쓰는 상황에서도 차단은 그대로 (이게 핵심 안전 요건)
        code, out = run_hook("Bash", {"command": "rm -rf /tmp/x"}, {"GUARD_BLOCK_COUNTER": tmp})
        denied = '"permissionDecision": "deny"' in out.replace('"permissionDecision":"deny"', '"permissionDecision": "deny"')
        report(code == 2 and denied, "카운터 실패해도 차단", "exit=%s deny=%s" % (code, denied))

    return bad


def main():
    bad = check_pattern_cases()
    print("")
    bad += check_counter_cases()

    total = len(CASES) + 4
    print("")
    print("=== %d/%d ok ===" % (total - bad, total))
    return 1 if bad else 0


if __name__ == "__main__":
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(encoding="utf-8")
        except Exception:
            pass
    sys.exit(main())
