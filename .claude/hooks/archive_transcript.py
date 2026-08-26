#!/usr/bin/env python3
"""SessionEnd 훅 — 세션 transcript(jsonl)를 레포 안 아카이브로 복사한다.

왜 필요한가:
  재현성의 원본은 Claude Code가 이미 전부 기록하고 있다(세션 전체 transcript jsonl).
  별도 로그 시스템을 만드는 건 이중 기록이라 한쪽이 반드시 부패한다. 다만 자체 transcript에는
  구멍이 둘 있다 — (1) cleanupPeriodDays가 지나면 삭제되고, (2) 위치가 레포 밖
  `~/.claude/projects/<프로젝트 슬러그>/` 라서 리포를 백업해도 안 따라온다.
  이 훅은 (2)를 메우고, settings.json 의 cleanupPeriodDays 연장이 (1)을 메운다.

역할 분리 (이걸 섞으면 둘 다 썩는다):
  원본(재현성)   = 여기서 만드는 .claude/transcripts-archive/*.jsonl — 기계용, 전량 기록
  색인(찾기)     = docs/reports/*.html — 사람용, 이해 병목 해소

한계:
  SessionEnd는 차단이 불가능한 이벤트라 실패해도 세션에 영향이 없다. 그래서 조용히 죽으면
  아무도 모른다. 실패는 반드시 stderr로 내고 non-zero로 끝낸다 (non-zero면 stderr가 사용자에게
  보인다). 프로세스가 강제 종료되는 경우(터미널 강제 종료 등)에는 애초에 훅이 안 돈다 —
  이 아카이브는 최선 노력이지 보장이 아니다.
"""

import json
import shutil
import sys
from datetime import datetime
from pathlib import Path

# .claude/hooks/ 기준으로 .claude/transcripts-archive/ 를 잡는다.
# cwd에 의존하지 않는다 — 훅이 어느 디렉토리에서 호출될지 보장이 없다.
ARCHIVE_DIR = Path(__file__).resolve().parent.parent / "transcripts-archive"


def destination(session_id):
    """저장 경로. 같은 세션이 이미 아카이브돼 있으면 그 이름을 그대로 재사용한다.

    SessionEnd는 한 세션에 여러 번 발동할 수 있다(clear 후 재개 등). 매번 오늘 날짜를
    붙이면 같은 세션의 사본이 날짜별로 흩어지므로, 기존 파일이 있으면 거기에 덮어쓴다
    (나중 사본이 항상 더 완전하다).
    """
    existing = sorted(ARCHIVE_DIR.glob("*-" + session_id + ".jsonl"))
    if existing:
        return existing[0]
    return ARCHIVE_DIR / (datetime.now().strftime("%Y-%m-%d") + "-" + session_id + ".jsonl")


def main():
    try:
        payload = json.load(sys.stdin)
    except Exception as e:
        sys.stderr.write("transcript 아카이브 실패: stdin을 읽지 못했다 (%s)\n" % e)
        return 1

    src = payload.get("transcript_path") or ""
    session_id = payload.get("session_id") or "unknown"

    if not src:
        sys.stderr.write("transcript 아카이브 실패: transcript_path가 비어 있다.\n")
        return 1

    src_path = Path(src)
    if not src_path.is_file():
        sys.stderr.write("transcript 아카이브 실패: 파일이 없다 — %s\n" % src)
        return 1

    try:
        ARCHIVE_DIR.mkdir(parents=True, exist_ok=True)
        dst = destination(session_id)
        shutil.copy2(src_path, dst)
    except Exception as e:
        sys.stderr.write("transcript 아카이브 실패: 복사 중 오류 — %s\n" % e)
        return 1

    sys.stdout.write("archived: %s (%.1f MB, reason=%s)\n" % (
        dst, dst.stat().st_size / (1024 * 1024), payload.get("reason", "?")))
    return 0


if __name__ == "__main__":
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(encoding="utf-8")
        except Exception:
            pass
    sys.exit(main())
