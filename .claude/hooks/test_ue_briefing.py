#!/usr/bin/env python3
"""ue_briefing.py 테스트. 실행: py -3 .claude/hooks/test_ue_briefing.py

실제 표지 디렉터리와 실제 스킬 파일을 건드리지 않도록 환경변수로 임시 경로를 넘긴다.
"""

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

HOOK = Path(__file__).with_name("ue_briefing.py")

SKILL_TEXT = """---
name: ue-mcp
description: 머리말 설명
---

# 언리얼 MCP

- 본문 첫 줄
- 본문 둘째 줄
"""


def run(payload, env):
    proc = subprocess.run(
        [sys.executable, str(HOOK)],
        input=json.dumps(payload, ensure_ascii=False),
        capture_output=True, text=True, encoding="utf-8", env=env,
    )
    return proc.returncode, proc.stdout


def main():
    failures = []

    def expect(name, cond):
        print(("PASS " if cond else "FAIL ") + name)
        if not cond:
            failures.append(name)

    with tempfile.TemporaryDirectory() as tmp:
        skill = Path(tmp) / "SKILL.md"
        skill.write_text(SKILL_TEXT, encoding="utf-8")
        env = dict(os.environ, UE_BRIEFING_STATE_DIR=str(Path(tmp) / "state"), UE_BRIEFING_SKILL=str(skill))

        call = {"hook_event_name": "PreToolUse", "tool_name": "mcp__unreal__call_tool", "session_id": "s1"}

        code, out = run(call, env)
        expect("첫 호출은 exit 2로 막는다", code == 2)
        reason = json.loads(out)["hookSpecificOutput"]["permissionDecisionReason"] if out else ""
        expect("사유에 스킬 본문이 들어간다", "본문 첫 줄" in reason and "본문 둘째 줄" in reason)
        expect("사유에 머리말은 들어가지 않는다", "머리말 설명" not in reason and "name: ue-mcp" not in reason)

        code, _ = run(call, env)
        expect("같은 세션의 두 번째 호출은 통과한다", code == 0)

        code, _ = run(dict(call, session_id="s2"), env)
        expect("다른 세션의 첫 호출은 다시 막는다", code == 2)

        code, _ = run({"hook_event_name": "SessionStart", "source": "compact", "session_id": "s1"}, env)
        expect("압축 뒤 SessionStart는 통과한다", code == 0)
        code, _ = run(call, env)
        expect("압축 뒤 첫 호출은 다시 막는다", code == 2)

        code, _ = run({"hook_event_name": "PreToolUse", "tool_name": "Bash", "session_id": "s3"}, env)
        expect("다른 툴은 건드리지 않는다", code == 0)

        code, _ = run(dict(call, session_id="../../evil"), env)
        expect("세션 id의 경로 문자는 지운다", code == 2 and (Path(tmp) / "state" / "evil").exists())

        env_missing = dict(env, UE_BRIEFING_SKILL=str(Path(tmp) / "none.md"), UE_BRIEFING_STATE_DIR=str(Path(tmp) / "state2"))
        code, out = run(call, env_missing)
        expect("스킬 파일이 없어도 막고 경로를 알려 준다", code == 2 and "none.md" in out)

    print(f"\n{'실패 ' + str(len(failures)) + '건' if failures else '전부 통과'}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.exit(main())
