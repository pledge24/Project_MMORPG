"""DataBuilder 진입점. `Invoke-DataBuilder.ps1`이 가상 환경의 파이썬으로 부른다.

    pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1

검사가 하나라도 실패하면 산출물을 바꾸지 않고 종료 코드 1로 끝난다.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from databuilder.build import run

TOOL_DIR = Path(__file__).resolve().parent
REPO_ROOT = TOOL_DIR.parents[1]


def main() -> int:
    parser = argparse.ArgumentParser(description="기획 데이터 원본을 검사하고 서버가 읽는 JSON과 C++ 타입을 생성한다")
    parser.add_argument("--config", type=Path, default=TOOL_DIR / "databuilder.toml", help="설정 파일 경로")
    args = parser.parse_args()
    return run(args.config.resolve(), REPO_ROOT)


if __name__ == "__main__":
    # 콘솔 코드 페이지와 무관하게 한국어 메시지가 깨지지 않게 한다.
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    sys.exit(main())
