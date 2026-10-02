#!/usr/bin/env python3
"""언리얼 MCP 브리핑 훅 — 세션의 첫 언리얼 MCP 호출을 한 번 막고 이 저장소의 함정을 보여 준다.

왜 필요한가:
  CLAUDE.md는 언리얼 MCP를 쓰기 전에 ue-mcp 스킬을 읽으라고 하지만, 실제 세션에서는
  거의 지켜지지 않았다(#131). 「먼저 읽어라」는 문서는 읽은 뒤에야 보이므로 읽지 않는
  상황을 고치지 못한다. 호출 시점에 기계적으로 끼어들어 내용을 직접 컨텍스트에 넣는다.

동작:
  PreToolUse(mcp__unreal__call_tool)
    이 세션의 표지 파일이 없으면 표지를 만들고, ue-mcp 스킬의 본문(머리말 제외)을 사유로 담아
    deny(exit 2)한다. 같은 호출을 다시 보내면 표지가 있으므로 통과한다.
  SessionStart(matcher: compact)
    컨텍스트가 압축되면 브리핑도 함께 사라지므로 이 세션의 표지를 지운다. 압축 뒤 첫 호출에서
    다시 보여 준다.

  내용의 원천은 ue-mcp 스킬 하나다. 이 파일에 문구를 복사하지 않는다.

테스트: py -3 .claude/hooks/test_ue_briefing.py
"""

import json
import os
import re
import sys
from pathlib import Path

# 표지 디렉터리. 테스트가 실제 표지를 건드리지 않도록 환경변수로 덮어쓴다.
STATE_ENV = "UE_BRIEFING_STATE_DIR"
STATE_DEFAULT = Path(__file__).with_name(".ue_briefed")

SKILL_ENV = "UE_BRIEFING_SKILL"
SKILL_DEFAULT = Path(__file__).resolve().parent.parent / "skills" / "ue-mcp" / "SKILL.md"

# 스킬 파일 맨 앞의 머리말(--- 로 둘러싼 블록).
FRONT_MATTER = re.compile(r"\A---\r?\n.*?\r?\n---\r?\n", re.S)


def _path_from_env(name, default):
    value = os.environ.get(name)
    return Path(value) if value else default


def _marker(session_id):
    # 세션 id는 파일 이름으로 쓴다. 경로 구분자 같은 문자가 들어오면 지운다.
    safe = re.sub(r"[^A-Za-z0-9_-]", "", session_id) or "unknown"
    return _path_from_env(STATE_ENV, STATE_DEFAULT) / safe


def _briefing():
    skill = _path_from_env(SKILL_ENV, SKILL_DEFAULT)
    try:
        text = skill.read_text(encoding="utf-8")
    except OSError:
        return f"{skill}를 읽지 못했다. 언리얼 MCP를 쓰기 전에 이 파일을 직접 읽는다."

    body = FRONT_MATTER.sub("", text, count=1)
    return (
        "이 세션의 첫 언리얼 MCP 호출이라 한 번 막았다. 아래는 .claude/skills/ue-mcp/SKILL.md의 본문이다. "
        "읽고 따른 뒤 같은 호출을 다시 보내면 통과한다.\n\n" + body.strip()
    )


def _deny(reason):
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
        payload = json.loads(sys.stdin.read() or "{}")
    except json.JSONDecodeError:
        return 0  # 입력을 못 읽으면 막지 않는다. 안전 판정은 guard_dangerous_cmd.py가 맡는다.

    event = payload.get("hook_event_name", "")
    marker = _marker(str(payload.get("session_id", "")))

    if event == "SessionStart":
        try:
            marker.unlink()
        except FileNotFoundError:
            pass
        return 0

    if event == "PreToolUse" and payload.get("tool_name") == "mcp__unreal__call_tool":
        if marker.exists():
            return 0
        marker.parent.mkdir(parents=True, exist_ok=True)
        marker.touch()
        return _deny(_briefing())

    return 0


if __name__ == "__main__":
    # Windows 콘솔 기본 인코딩(cp949)으로 쓰면 한국어 사유가 깨진다. guard_dangerous_cmd.py와 같게 맞춘다.
    for stream in (sys.stdin, sys.stdout, sys.stderr):
        try:
            stream.reconfigure(encoding="utf-8")
        except Exception:
            pass
    sys.exit(main())
