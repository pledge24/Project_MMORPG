# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 프로젝트 개요

반드시 함께 띄워야 하는 3티어 MMORPG. 포트폴리오용.

| 티어 | 경로 | 스택 | 포트 |
|---|---|---|---|
| 게임 클라이언트 | `P1/` | Unreal Engine 5.8 (**런처 설치본 고정**), 모듈명 `P1` | — |
| 게임 서버 | `Server/GameServer/` | C++20, 자체 IOCP 코어 | `127.0.0.1:7777` |
| 인증 서버 | `Server/AuthServer/` | Node.js / Express (ESM) | `.env`의 `PORT` (클라는 `5000`을 기대) |

저장소: SQL Server LocalDB — GameDB는 `(localdb)\ProjectModels`, UserDB는 `(localdb)\MSSQLLocalDB`.
**인스턴스가 서로 다름에 주의.** 두 서버가 공유하는 액세스 토큰 저장소로 Redis(`127.0.0.1:6379`).
Rider의 DB 연결은 읽기 전용 계정(`claude_ro`)을 사용한다. 상세: `docs/build.md`

## 문서 위치

- `docs/NEXT.md` — 다음에 무엇을 왜. **세션 시작 시 읽는다.** 미래를 말하는 유일한 문서다.
- `docs/harness.md` — 스킬 목록과 판단 기준
- `docs/ARCHITECTURE.md` — 깨면 안 되는 불변식
- `docs/tech-debt.md` — 알려진 부채 (D-NN)
- `docs/decisions/index.md` — ADR 표. 본문은 필요한 것만 연다
- `docs/work/` — 세션별 작업 기록
- `docs/{build,codegen,testing}.md` — 해당 영역을 건드릴 때만
- `docs/reports/` — 사람용 설명 자료

**아키텍처 결정(ADR)**은 코드에 드러나지 않는 제약이다. 추측하지 말고 읽는다.
프로토콜 변경·서버 동시성·클라-서버 경계 타입 추가·빌드 구성 변경·새 의존성 도입 전에는
`docs/decisions/index.md`를 읽고 해당 `scope`의 ADR만 연다. 전부 열지 않는다.
각 ADR의 `Confirmation` 절이 실제 준수 항목이며, 작업 후 그 항목으로 자체 점검한 결과를 보고한다.
`status: accepted`만 현행이다. `superseded`/`deprecated`/`rejected`는 이력이므로 따르지 않는다.
되돌리기 비싼 결정을 하게 되면 코드를 먼저 쓰지 말고 알린다. 초안은 `proposed`로 두고
승인 없이 `accepted`로 바꾸지 않는다.

## 도구 라우팅 — Rider MCP 전용 원칙

- 심볼·호출 관계 탐색: `analyze_calls`(INCOMING/OUTGOING, depth 2~3부터), `search_symbol`.
  **grep 금지** — UE RPC의 `_Implementation` 접미사에서 호출 사슬이 끊긴다.
- 빌드 검증: `build_solution_start`(+ `filesToRebuild`로 범위 축소) → `build_solution_state`로 폴링.
  **`rebuild=true` 금지. UBT/Build.bat/MSBuild를 터미널로 직접 돌리지 않는다** — 출력 절단으로
  에러가 유실된다.
- **Rider MCP 툴에는 `rootFolder`를 항상 명시한다** (파라미터 이름이 `projectPath`가 아니다).
  **함정: 솔루션이 하나만 열려 있으면 서버가 모호성을 못 느껴 거부하지 않고 그대로 실행한다.**
  Server를 빌드하려는데 P1만 열려 있으면 P1이 빌드된다. 인자 없이 `get_run_configurations`를
  부르면 열린 프로젝트 목록이 에러 메시지로 돌아온다.
- 린트·진단: `lint_files`, `get_file_problems`. 심볼 리네임: `rename_refactoring` (텍스트 치환 금지).
- 에디터·에셋·PIE 조작: UE MCP (도입 후. 도입 전에는 사람에게 요청).
- 서버 변경 검증은 Unreal을 띄우지 않고 `Server/DummyClient/`로 가능하다 (실 클라와 동일 프로토콜).
- **노출 ≠ 존재.** 판단 기준은 문서가 아니라 세션에 실제로 노출된 툴 목록이다. 근거와 예외:
  `docs/build.md`

## 완료 기준

- C++ 편집 후 반드시 빌드로 검증한다. 빌드 통과 없이는 완료가 아니다.
- **테스트가 존재하는 영역은 테스트 통과까지가 완료다.** 현재 적용 범위는 **게임 서버**와
  **인증 서버**. 빌드 초록은 그 영역에서 더 이상 완료 신호가 아니다. 실행 경로: `docs/testing.md`
- 테스트가 없는 영역(UE 클라 전체, 서버의 룸·DB 경로)은 서버 프로토콜/핸들러 변경을
  DummyClient로 스모크 확인, 인증 서버 기동은 `npm start`.
- 검증 없이 "완료했다"고 보고하지 않는다. 검증 불가한 부분은 불가하다고 명시한다.
- **자기신고를 믿지 않는다.** 검증 커맨드를 실제로 실행한 결과 없이 완료를 선언하지 않는다.
  "됐을 것이다"는 완료가 아니다 — 코드를 고친 사실과 그 코드가 도는 사실은 별개다.
- **존재 ≠ 가능.** 구성요소가 있는 것을 확인하고 "그러니 이 기능이 된다"고 쓰지 않는다.
  가장 싸게 실패하는 경로를 먼저 돌려보고 나서 쓴다. **부분 확인이 오히려 확신을 키우는 것**이
  이 실수의 공통 형태다(구멍 하나를 찾아 메우면 나머지를 검증했다고 느낀다).
  실제 사례: UE LLT — 설치본에 재료가 있는 것을 확인하고 "가능"으로 계획에 적었으나
  빌드 자체가 거부됐다(2026-08-27).

## 안전

- 에디터를 변경하는 UE MCP 작업 전에는 커밋 또는 셸브를 확인한다.
- 임의 코드를 실행하는 MCP 툴(`execute_tool`, `ue_execute_python`, `execute_terminal_command`)은
  **무엇을 실행할지 먼저 보고한다.**
- DB/Redis 파괴적 명령 금지: DROP, TRUNCATE, WHERE 없는 DELETE/UPDATE, FLUSHALL, FLUSHDB, `KEYS *`.
- 스키마 변경(ALTER/CREATE/DROP)은 사람 승인 후, 저장소의 SQL 스크립트 갱신과 함께만 실행한다.
- 위 두 줄은 `.claude/hooks/guard_dangerous_cmd.py`가 실제로 차단한다 (`Bash`·`PowerShell`·
  `execute_terminal_command`·`execute_sql_query` 4경로). **`ue_execute_python`은 훅이 못 덮으므로
  사람 판단에만 의존한다.** 훅 인터프리터는 `py -3` — 이 머신의 `python3`는 MS Store 별칭 스텁이라
  실행되지 않는다(exit 49).
- **훅은 명령 문자열 전체를 본다.** 위험 패턴을 *언급만* 하는 텍스트도 걸린다 — 대표적으로 그 패턴을
  설명하는 커밋 메시지. 해결책은 예외 추가가 아니라 텍스트를 명령에서 빼는 것:
  **`git commit -F <파일>`을 쓴다** (`-m "..."`이나 heredoc은 메시지가 명령 문자열에 들어간다).

## 작업 방식

- **세션 시작: `docs/NEXT.md`를 읽는다.**
- 복잡한 작업은 `new-work` 스킬로 시작한다. 코드베이스를 읽기 전에 「왜 이 작업인가」와
  「결정」을 먼저 쓰고, 그다음 플랜 모드에서 Phase를 짠다. 파일은
  `docs/work/YYYY-MM-DD-{작업명}.md`, 상태는 프론트매터로 표시한다.
- **막힌 항목은 지우지 않는다.** `[B] blocked — 무엇이/어디서` 로 표기하고 거기서 멈춘다.
  특히 에디터·사람 손이 필요한 작업은 우회로를 찾지 말고 기록 후 인계한다 — 우회는 계획에
  없던 상태를 만들고, 그 상태는 다음 세션이 모른다.
- **완료 항목 옆에는 summary 한 줄**을 남긴다 (생성·수정한 파일 경로 + 핵심 결정).
- **세션 기록은 원본과 색인을 나눈다.** 원본(재현성)은 `.claude/transcripts-archive/`의
  transcript jsonl — 세션 종료 시 `archive_transcript.py` 훅이 자동 복사한다(gitignore).
  색인은 `docs/reports/`의 사람용 리포트. 둘의 역할을 섞지 않는다 — 기록 시스템이 둘이면
  한쪽이 부패하는데, 역할이 다르면 둘 다 필요하다.
- 발견한 부채는 즉시 고치지 말고 `docs/tech-debt.md`에 기록한다.
- **세션 종료: `end-session` 스킬을 돈다. `NEXT.md` 갱신이 마지막이다.**
  work 파일에 「다음에 할 것」을 적지 않는다.
- 같은 실수가 두 번 반복되면 이 파일에 규칙 한 줄 추가를 제안하고, 세 번이면 훅/규칙 승격을 제안한다.
- 이 파일이 비대해지면 서술적 내용은 `docs/`로 옮기고 여기엔 규칙과 목차만 남긴다.

## 아키텍처 핵심 규칙 (상세: `docs/ARCHITECTURE.md`)

- **인증 티어와 게임 티어를 잇는 건 Redis뿐이다. 게임 서버는 `UserDB`를 직접 건드리지 않는다.**
- **`Room`이 `JobQueue`를 상속한다.** 패킷 핸들러는 인라인으로 일하지 않고 `room->DoAsync(...)`로
  잡만 밀어넣고 리턴한다. 룸 소유 상태는 큐 위에서 직렬화되므로 락이 없다.
  **다른 룸의 오브젝트에 직접 손대지 않는다.** DB 작업도 `DBQueue`에 push한다.
- **클라: 네트워크 스레드는 UObject를 절대 만지지 않는다.**
- **클라/서버 클래스 계층이 대칭이다.** 게임플레이 변경은 클라 + 서버 + 프로토콜,
  **3곳 수정을 기본으로 생각할 것.**

## 컨벤션

**인코딩 — `.proto`와 `.bat`은 cp949, 나머지는 전부 UTF-8.** `.gitattributes`와 두 개의
`.editorconfig`가 이를 강제한다. 이 파일들의 한국어 주석을 UTF-8로 읽으면 깨져 보인다.
정상이며, UTF-8로 다시 저장해서 "고치면" 이 파일들을 소비하는 툴 쪽이 깨진다.

**문서 파일명은 소문자와 대시**를 쓴다(`tech-debt.md`). 예외는 루트의 관례 파일과 각 폴더의
진입점뿐이다 — `CLAUDE.md`, `README.md`, `CONTEXT.md`, `docs/ARCHITECTURE.md`, `docs/NEXT.md`.

**커밋**은 gitmoji + 한국어 설명: `<emoji> <type>: <설명>` — `🚧 wip:`, `✅ done:`, `📝 plan:`,
`♻️ refactor:`, `🧹 clean:`, `🎉 add:`. wip 커밋에는 빌드 상태를 덧붙이는 경우가 많다.
기능 브랜치는 `#<issue>-<Topic>` 이름을 쓰고 PR로 `dev`에 머지한다.

**git 추적 — 전역 커밋 규칙의 의도적 예외.** 이 리포는 `CLAUDE.md`·`.claude/`·`docs/`를
**의도적으로 추적한다**. 전역 규칙(`~/.claude/rules/commit.md`)은 이들을 추적 제외하라고 하지만
여기서는 따르지 않는다. 루트 `.gitignore`를 되돌리지 말 것.

**주석과 로그는 한국어로 작성한다.** 코드를 수정할 때 주변 언어에 맞출 것.

빌드 구성·모듈·include 규칙은 `docs/build.md`.

## 기존 코드베이스 방침

- 이 프로젝트는 하네스 도입 전에 작성된 손코딩 베이스다. 뒤죽박죽인 부분이 많음을 전제한다.
- 잘못된 설계 발견 시: 즉시 고치지 않는다 → `docs/tech-debt.md`에 기록 → 계획된 리팩토링에서만 수정.
- 목표 구조: 관심사별 레이어 분리, BP 위젯 로직의 C++ 이관, 캐릭터 클래스의 컴포넌트 분리.
- 신규 코드는 목표 구조를 따른다. 기존 코드는 `docs/work/`에 계획 없이 구조를 건드리지 않는다.