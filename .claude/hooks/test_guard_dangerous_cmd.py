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

# 언리얼 MCP 라우터 케이스에서 반복되는 긴 이름들.
UE = "mcp__unreal__call_tool"
OBJ = "editor_toolset.toolsets.object.ObjectTools"
BP = "editor_toolset.toolsets.blueprint.BlueprintTools"
AUTO = "AutomationTestToolset.AutomationTestToolset"
PROG = "editor_toolset.toolsets.programmatic.ProgrammaticToolset"
LOGS = "EditorToolset.LogsToolset"
APP = "EditorToolset.EditorAppToolset"
SLATE = "SlateInspectorToolset.SlateInspectorToolset"
ASSET = "editor_toolset.toolsets.asset.AssetTools"
SKILL = "ToolsetRegistry.AgentSkillToolset"
SEQ = "animation_toolset.toolsets.sequencer.SequencerTools"
NIA = "NiagaraToolsets.NiagaraToolset_System"
TEX = "editor_toolset.toolsets.texture.TextureTools"

# 참고용 보관소 경로. 콘텐츠 경로와 디스크 경로 양쪽을 검사한다.
EXT = "/Game/External/ClassicMMOUI/Textures/0_common/Btn1_normal"
EXT_DISK = "D:/Unreal/Projects/Project_MMORPG/P1/Content/External/ClassicMMOUI/a.png"
P1PATH = "/Game/P1/UI/Frontend/WBP_LoginMenu"

# execute_tool_script 판정용 스크립트들.
# 규약은 execute_tool(전체이름, JSON문자열) 이고 run() 을 정의해야 한다.
SCRIPT_OK = '''
import json
def dup(src, dst):
    return execute_tool("editor_toolset.toolsets.asset.AssetTools.duplicate",
                        json.dumps({"path": src, "new_path": dst}))
def run():
    dup("/Game/P1/a", "/Game/P1/b")
    return {"ok": 1}
'''

SCRIPT_EXT_READ = '''
import json
def run():
    return {"r": execute_tool("editor_toolset.toolsets.asset.AssetTools.get_referencers",
                              json.dumps({"asset_path": "/Game/External/ClassicMMOUI/x"}))}
'''

SCRIPT_EXT_WRITE = '''
import json
def run():
    execute_tool("editor_toolset.toolsets.asset.AssetTools.delete",
                 json.dumps({"path": "/Game/External/ClassicMMOUI"}))
    return {}
'''

SCRIPT_CLOSED = '''
def run():
    execute_tool("animation_toolset.toolsets.sequencer.SequencerTools.create_sequence", "{}")
    return {}
'''

SCRIPT_EVAL = '''
def run():
    eval("execute_tool('x', '{}')")
    return {}
'''

SCRIPT_DUNDER = '''
def run():
    execute_tool.__globals__["x"] = 1
    return {}
'''

SCRIPT_DYNAMIC = '''
def run():
    name = "editor_toolset.toolsets.asset.AssetTools.delete"
    execute_tool(name, "{}")
    return {}
'''

SCRIPT_RECURSE = '''
def run():
    execute_tool(
        "editor_toolset.toolsets.programmatic.ProgrammaticToolset.execute_tool_script", "{}")
    return {}
'''


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

    # --- 언리얼 MCP 1층: 툴셋 허용 명단 (ADR-0003) ---
    # 쓰기가 열려 있다는 것 자체가 판정 대상이다. 무엇이 열렸는지 여기서 고정한다.
    ("UE 조회 통과", UE, {"toolset_name": OBJ, "tool_name": "list_properties"}, False),
    ("UE 속성 쓰기 통과", UE, {"toolset_name": OBJ, "tool_name": "set_properties"}, False),
    ("UE BP 그래프 쓰기 통과", UE, {"toolset_name": BP, "tool_name": "write_graph_dsl"}, False),
    ("UE 에셋 삭제 통과", UE, {"toolset_name": ASSET, "tool_name": "delete"}, False),
    ("UE 에셋 복사 통과", UE, {"toolset_name": ASSET, "tool_name": "duplicate"}, False),
    ("UE 텍스처 임포트 통과", UE, {"toolset_name": TEX, "tool_name": "import_file"}, False),
    ("UE 스킬 생성 통과", UE, {"toolset_name": SKILL, "tool_name": "CreateSkill"}, False),
    ("UE 에디터 UI 조작 통과", UE, {"toolset_name": APP, "tool_name": "SelectActors"}, False),
    ("UE PIE 시작 통과", UE, {"toolset_name": APP, "tool_name": "StartPIE"}, False),
    ("UE 테스트 실행 통과", UE, {"toolset_name": AUTO, "tool_name": "RunTests"}, False),
    ("UE 로그 상세도 변경 통과", UE, {"toolset_name": LOGS, "tool_name": "SetVerbosity"}, False),
    ("UE 최상위 조회 통과", UE, {"tool_name": "list_toolsets"}, False),

    # 1층 밖은 막는다. 이 프로젝트가 쓰지 않는 툴셋이 열리지 않았는지 고정한다.
    ("UE 시퀀서 툴셋 차단", UE, {"toolset_name": SEQ, "tool_name": "create_sequence"}, True),
    ("UE 나이아가라 툴셋 차단", UE, {"toolset_name": NIA, "tool_name": "AddModule"}, True),
    ("UE 모르는 툴셋 차단", UE, {"toolset_name": "Foo.Bar", "tool_name": "list_properties"}, True),
    ("UE 최상위 재귀 차단", UE, {"tool_name": "call_tool"}, True),
    ("UE 인자 누락 차단", UE, {}, True),
    ("UE 인자 형식오류 차단", UE, {"toolset_name": OBJ, "tool_name": 7}, True),

    # --- 언리얼 MCP 3층: 참고용 보관소 경로 가드 (ADR-0006) ---
    # Content/External/ 은 사람이 관리하고 git 이 추적하지 않는다. 되돌릴 방법이
    # 마켓플레이스 재설치뿐이라 조회만 허용한다.
    ("UE External 조회 통과", UE, {"toolset_name": ASSET, "tool_name": "get_referencers",
                                   "arguments": {"asset_path": EXT}}, False),
    ("UE External 의존성 조회 통과", UE, {"toolset_name": ASSET, "tool_name": "get_dependencies",
                                          "arguments": {"asset_path": EXT}}, False),
    ("UE External 삭제 차단", UE, {"toolset_name": ASSET, "tool_name": "delete",
                                   "arguments": {"path": EXT}}, True),
    ("UE External 에서 이동 차단", UE, {"toolset_name": ASSET, "tool_name": "move",
                                        "arguments": {"path": EXT, "new_path": P1PATH}}, True),
    ("UE External 로 이동 차단", UE, {"toolset_name": ASSET, "tool_name": "move",
                                      "arguments": {"path": P1PATH, "new_path": EXT}}, True),
    ("UE External 속성 쓰기 차단", UE, {"toolset_name": OBJ, "tool_name": "set_properties",
                                        "arguments": {"object": {"refPath": EXT}}}, True),
    ("UE External 디스크경로 차단", UE, {"toolset_name": TEX, "tool_name": "import_file",
                                         "arguments": {"source_file": EXT_DISK}}, True),
    ("UE P1 삭제는 통과", UE, {"toolset_name": ASSET, "tool_name": "delete",
                               "arguments": {"path": P1PATH}}, False),

    # --- 언리얼 MCP 4층: execute_tool_script 의 정적 판정 ---
    # 스크립트 안의 호출에도 같은 판정이 걸리는지 본다.
    ("UE 환경 조회 통과", UE, {"toolset_name": PROG,
                               "tool_name": "get_execution_environment"}, False),
    ("UE 스크립트 정상 통과", UE, {"toolset_name": PROG, "tool_name": "execute_tool_script",
                                   "arguments": {"script": SCRIPT_OK}}, False),
    ("UE 스크립트 External 조회 통과", UE, {"toolset_name": PROG,
                                            "tool_name": "execute_tool_script",
                                            "arguments": {"script": SCRIPT_EXT_READ}}, False),
    ("UE 스크립트 External 쓰기 차단", UE, {"toolset_name": PROG,
                                            "tool_name": "execute_tool_script",
                                            "arguments": {"script": SCRIPT_EXT_WRITE}}, True),
    ("UE 스크립트 닫힌툴셋 차단", UE, {"toolset_name": PROG, "tool_name": "execute_tool_script",
                                       "arguments": {"script": SCRIPT_CLOSED}}, True),
    ("UE 스크립트 eval 차단", UE, {"toolset_name": PROG, "tool_name": "execute_tool_script",
                                   "arguments": {"script": SCRIPT_EVAL}}, True),
    ("UE 스크립트 던더 차단", UE, {"toolset_name": PROG, "tool_name": "execute_tool_script",
                                   "arguments": {"script": SCRIPT_DUNDER}}, True),
    ("UE 스크립트 동적이름 차단", UE, {"toolset_name": PROG, "tool_name": "execute_tool_script",
                                       "arguments": {"script": SCRIPT_DYNAMIC}}, True),
    ("UE 스크립트 재귀 차단", UE, {"toolset_name": PROG, "tool_name": "execute_tool_script",
                                   "arguments": {"script": SCRIPT_RECURSE}}, True),
    ("UE 스크립트 문법오류 차단", UE, {"toolset_name": PROG, "tool_name": "execute_tool_script",
                                       "arguments": {"script": "def run(:"}}, True),
    ("UE 스크립트 빈인자 차단", UE, {"toolset_name": PROG, "tool_name": "execute_tool_script",
                                     "arguments": {}}, True),

    # --- 관계없는 툴 ---
    ("Read 툴", "Read", {"file_path": "/etc/passwd"}, False),
]


def check_pattern_cases():
    """패턴·rootFolder 판정. 실패 개수를 돌려준다."""
    bad = 0
    # 카운터와 감사 로그가 실제 파일을 건드리지 않도록 통째로 임시 경로로 돌린다.
    with tempfile.TemporaryDirectory() as tmp:
        env = {
            "GUARD_BLOCK_COUNTER": str(Path(tmp) / "noise.log"),
            "GUARD_UE_AUDIT": str(Path(tmp) / "noise-audit.log"),
        }
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


def check_audit_cases():
    """언리얼 MCP 감사 로그 동작. 실패 개수를 돌려준다."""
    bad = 0

    def report(ok, desc, detail=""):
        nonlocal bad
        if not ok:
            bad += 1
        print(("PASS " if ok else "FAIL ") + "%-24s %s" % (desc, detail))

    with tempfile.TemporaryDirectory() as tmp:
        log = Path(tmp) / "ue_audit.log"
        noise = str(Path(tmp) / "noise.log")
        env = {"GUARD_BLOCK_COUNTER": noise, "GUARD_UE_AUDIT": str(log)}

        # 1) 통과한 UE 호출 1회 → 줄 1개, TSV 필드 4개, 인자 요약 포함
        args = {"instance": {"refPath": "/Game/X.X_C"}}
        code, _ = run_hook(
            UE, {"toolset_name": OBJ, "tool_name": "list_properties", "arguments": args}, env)
        lines = log.read_text(encoding="utf-8").splitlines() if log.exists() else []
        fields = lines[0].split("\t") if lines else []
        report(
            code == 0 and len(lines) == 1 and len(fields) == 4
            and fields[1] == OBJ and fields[2] == "list_properties"
            and "/Game/X.X_C" in fields[3],
            "UE 통과 → 감사 1줄",
            "줄=%d 필드=%d" % (len(lines), len(fields)),
        )

        # 2) 차단된 호출은 여기 남기지 않는다. 차단은 block_counter 가 맡는다.
        # set_properties 는 2026-09-22 에 열렸으므로 1층 밖 툴셋으로 막는다.
        run_hook(UE, {"toolset_name": SEQ, "tool_name": "create_sequence", "arguments": {}}, env)
        after = log.read_text(encoding="utf-8").splitlines()
        report(len(after) == 1, "차단은 감사에 안 남음", "줄=%d" % len(after))

        # 3) 언리얼 MCP 가 아닌 툴은 감사 대상이 아니다
        run_hook("Bash", {"command": "git status"}, env)
        after = log.read_text(encoding="utf-8").splitlines()
        report(len(after) == 1, "비 UE 툴은 감사 안 함", "줄=%d" % len(after))

        # 4) 긴 인자를 잘라서 한 줄을 유지한다. 안 자르면 로그가 통째로 부푼다.
        run_hook(UE, {"toolset_name": LOGS, "tool_name": "GetLogEntries",
                      "arguments": {"pattern": "가" * 2000}}, env)
        after = log.read_text(encoding="utf-8").splitlines()
        long_len = len(after[1]) if len(after) > 1 else 0
        report(
            len(after) == 2 and "(잘림)" in after[1] and long_len < 700,
            "긴 인자는 잘린다",
            "줄=%d 길이=%d" % (len(after), long_len),
        )

        # 5) 감사 로그를 못 쓰는 상황에서도 통과 판정은 그대로 (핵심 안전 요건)
        code, _ = run_hook(
            UE, {"toolset_name": OBJ, "tool_name": "list_properties"},
            {"GUARD_BLOCK_COUNTER": noise, "GUARD_UE_AUDIT": tmp})
        report(code == 0, "감사 실패해도 통과", "exit=%s" % code)

    return bad


def check_slate_cases():
    """슬레이트 조작이 커밋 상태에 따라 갈리는지. 실패 개수를 돌려준다.

    GUARD_REPO_ROOT 로 임시 저장소를 주입한다. 실제 저장소를 보면 판정이 그날의 워킹
    트리 상태에 따라 흔들려 테스트가 재현되지 않는다.
    """
    bad = 0

    def report(ok, desc, detail=""):
        nonlocal bad
        if not ok:
            bad += 1
        print(("PASS " if ok else "FAIL ") + "%-24s %s" % (desc, detail))

    git = ["git", "-c", "user.email=t@t", "-c", "user.name=t"]

    def make_repo(root, dirty):
        root.mkdir()
        subprocess.run(["git", "init", "-q", str(root)], capture_output=True)
        (root / "a.txt").write_text("x", encoding="utf-8")
        subprocess.run(git + ["add", "."], cwd=str(root), capture_output=True)
        subprocess.run(git + ["commit", "-qm", "init"], cwd=str(root), capture_output=True)
        if dirty:
            (root / "a.txt").write_text("changed", encoding="utf-8")

    with tempfile.TemporaryDirectory() as tmp:
        base = {
            "GUARD_BLOCK_COUNTER": str(Path(tmp) / "c.log"),
            "GUARD_UE_AUDIT": str(Path(tmp) / "a.log"),
        }
        clean = Path(tmp) / "clean"
        dirty = Path(tmp) / "dirty"
        norepo = Path(tmp) / "norepo"
        make_repo(clean, False)
        make_repo(dirty, True)
        norepo.mkdir()

        click = {"toolset_name": SLATE, "tool_name": "Click", "arguments": {"ref": "i1"}}
        snap = {"toolset_name": SLATE, "tool_name": "Snapshot", "arguments": {"ref": ""}}

        # git status 를 읽지 못하는 상황은 막는 쪽으로 기운다. 커밋 상태를 모르면
        # 조작 결과를 diff 로 읽을 수 있다는 전제가 깨지기 때문이다.
        states = (("깨끗", clean, False), ("미커밋", dirty, True), ("git아님", norepo, True))
        for label, root, expect_block in states:
            env = dict(base)
            env["GUARD_REPO_ROOT"] = str(root)
            code, _ = run_hook(UE, click, env)
            report(
                (code == 2) == expect_block,
                "슬레이트 조작 (%s)" % label,
                "exit=%s 기대=%s" % (code, "차단" if expect_block else "통과"),
            )
            code, _ = run_hook(UE, snap, env)
            report(code == 0, "슬레이트 관찰 (%s)" % label, "exit=%s" % code)

    return bad


def main():
    bad = check_pattern_cases()
    print("")
    bad += check_counter_cases()
    print("")
    bad += check_audit_cases()
    print("")
    bad += check_slate_cases()

    total = len(CASES) + 4 + 5 + 6
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
