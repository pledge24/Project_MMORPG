# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 프로젝트 개요

반드시 함께 띄워야 하는 3티어 MMORPG.

| 티어 | 경로 | 스택 | 포트 |
|---|---|---|---|
| 게임 클라이언트 | `P1/` | Unreal Engine 5.8 (**런처 설치본 고정**), 모듈명 `P1` | — |
| 게임 서버 | `Server/GameServer/` | C++20, 자체 IOCP 코어 | `127.0.0.1:7777` |
| 인증 서버 | `Server/AuthServer/` | Node.js / Express (ESM) | `.env`의 `PORT` (클라는 `5000`을 기대) |

저장소: SQL Server LocalDB — GameDB는 `(localdb)\ProjectModels`, UserDB는 `(localdb)\MSSQLLocalDB`.
**인스턴스가 서로 다름에 주의.** 두 서버가 공유하는 액세스 토큰 저장소로 Redis(`127.0.0.1:6379`).
Rider의 DB 연결은 읽기 전용 계정(`claude_ro`)을 쓴다. 상세: `docs/build.md`

## 문서 위치

- `docs/ARCHITECTURE.md` — 깨면 안 되는 불변식
- `docs/adr/` — 되돌리기 어려운 결정의 기록. 판정 기준과 형식은 `.claude/skills/domain-modeling/ADR-FORMAT.md`
- `docs/tech-debt.md` — 지금 틀린 것
- `docs/backlog.md` — 아직 착수하지 않은 작업 후보와 하지 않기로 확인된 것
- `docs/{build,codegen,testing}.md` — 해당 영역을 건드릴 때만
- `docs/reports/` — 사람용 설명 자료
- `docs/references/` — 2026-09에 걷어낸 하네스 v1의 이력. **현행 제약이 아니다.** 현행 제약은 위 문서들의 본문에 있다

## Agent skills

### 이슈 트래커

이슈와 명세는 GitHub Issues에 있다 (`gh` CLI). 상세: `docs/agents/issue-tracker.md`

### 트리아지 라벨

다섯 개 표준 역할을 저장소 라벨 문자열에 그대로 매핑한다. 상세: `docs/agents/triage-labels.md`

### 도메인 문서

single-context — 루트 `CONTEXT.md`와 `docs/adr/`. 상세: `docs/agents/domain.md`

## 도구 라우팅 — MCP 서버 두 개, 통제 수단도 두 개

- **Rider MCP에서 쓸 수 있는 툴은 36종이다.** 나머지는 `.claude/settings.json`의
  `permissions.deny`가 막는다. 판정 근거는 `docs/adr/0002-control-mcp-tools-via-permissions.md`.
- **언리얼 MCP(`unreal`)는 `permissions`가 아니라 훅이 막는다.** 이 서버는 도구를
  `list_toolsets`, `describe_toolset`, `call_tool` 세 개만 노출하고, 52개 툴셋의 수백 개
  도구가 전부 `call_tool`의 인자로 들어온다. 이름이 하나뿐이라 `permissions`로는 구분되지
  않는다. 허용 명단은 `.claude/hooks/guard_dangerous_cmd.py`의 `UE_ALLOWED_TOOLS`이고,
  **명단에 없으면 막힌다.** 지금 열려 있는 것은 조회 계열과 자동화 테스트뿐이다. 쓰기가
  필요하면 사람 승인을 받고 명단에 먼저 추가한다. 근거는
  `docs/adr/0003-gate-unreal-mcp-by-hook-whitelist.md`.
- **언리얼 MCP는 에디터가 떠 있어야 붙는다.** 사람이 에디터 콘솔에서
  `ModelContextProtocol.StartServer`를 입력해야 `127.0.0.1:8000`이 열린다. 연결 확인은
  `netstat`로 8000 포트를 보거나 `list_toolsets`를 한 번 부른다.
- 심볼 탐색: `skill_search`의 `mode=symbol`. 텍스트 탐색: `search_text`. **grep 금지** — UE RPC의
  `_Implementation` 접미사에서 호출 사슬이 끊긴다. 검색어는 접미사가 붙은 이름과 안 붙은 이름
  양쪽으로 잡는다. **`mode=symbol`의 좌표는 `1행 1열`로 고정되므로 파일 경로만 쓴다.**
- 호출자 확인은 `search_text`로 한다. `analyze_calls`는 C++ 심볼을 색인하지 않아 막아 두었다.
- 빌드 검증: **터미널에서 돌리고 종료 코드로 판정한다. 빌드에 Rider MCP를 쓰지 않는다.**
  클라이언트는 `Build.bat`, 서버는 `MSBuild`다. 0이 아니면 같은 출력에 에러가 코드와 파일과 줄과
  함께 찍혀 있다. **`Rebuild.bat`과 최초 전체 빌드는 출력이 커서 절단될 수 있다.** 그때는
  `%LOCALAPPDATA%\UnrealBuildTool\Log.txt`를 읽는다. 명령 원문은 `docs/build.md`, 근거는
  `docs/adr/0001-unify-build-path.md`.
- **Rider MCP 툴에는 `rootFolder`를 항상 명시한다** (파라미터 이름이 `projectPath`가 아니다).
  **함정: 솔루션이 하나만 열려 있으면 서버가 모호성을 못 느껴 거부하지 않고 그대로 실행한다.**
  Server를 빌드하려는데 P1만 열려 있으면 P1이 빌드된다. 인자 없이 `get_run_configurations`를
  부르면 열린 프로젝트 목록이 에러 메시지로 돌아온다.
- 린트·진단: `lint_files`, `get_file_problems`. 심볼 리네임: `rename_refactoring` (텍스트 치환 금지).
- UE 에셋 조회: `get_class_hierarchy`와 `search_assets`. **`search_assets`는 `baseClass`만 쓴다** —
  `query`는 빈 결과만 돌려준다. Rider의 에디터 조작 툴은 막혀 있으므로 사람에게 요청한다.
- **에셋 속성은 Rider가 아니라 언리얼 MCP로 읽는다.** Rider의 `get_asset_properties`는 블루프린트
  CDO에 `properties: []`를 돌려준다(ADR-0002). 같은 에셋을 `unreal`의 `ObjectTools.list_properties`로
  읽으면 속성이 나온다 — `BP_MonsterBase`에서 115개를 실측했다.
- 서버 변경 검증은 Unreal을 띄우지 않고 `Server/DummyClient/`로 가능하다 (실 클라와 동일 프로토콜).
- **노출 ≠ 존재.** 판단 기준은 문서가 아니라 세션에 실제로 노출된 툴 목록이다. **IDE 화면의 체크
  상태도 근거가 아니다** — 이 엔드포인트에 반영되지 않는다. 근거와 예외: `docs/build.md`와 ADR-0002

## 완료 기준

- C++ 편집 후 반드시 빌드로 검증한다. 빌드 통과 없이는 완료가 아니다.
- **테스트가 존재하는 영역은 테스트 통과까지가 완료다.** 현재 적용 범위는 **게임 서버**와
  **인증 서버**. 빌드 초록은 그 영역에서 더 이상 완료 신호가 아니다. 실행 경로: `docs/testing.md`
- 테스트가 없는 영역(UE 클라 전체, 서버의 룸·DB 경로)은 서버 프로토콜·핸들러 변경을
  DummyClient로 스모크 확인하고, 인증 서버 기동은 `npm start`로 확인한다.
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
- DB와 Redis의 파괴적 명령은 금지다. DROP, TRUNCATE, WHERE 없는 DELETE/UPDATE, 그리고 Redis의
  전체 삭제·전체 키 조회 명령 셋이 여기 해당한다.
- 스키마 변경(ALTER/CREATE/DROP)은 사람 승인 후, 저장소의 SQL 스크립트 갱신과 함께만 실행한다.
- 위 두 줄은 `.claude/hooks/guard_dangerous_cmd.py`가 실제로 차단한다 (`Bash`·`PowerShell`·
  `execute_terminal_command`·`execute_sql_query` 4경로). 차단 패턴의 전체 목록은 그 파일에 있다.
  같은 훅이 `mcp__unreal__call_tool`도 받아서 허용 명단으로 판정한다 (ADR-0003).
  **`ue_execute_python`은 훅이 못 덮으므로 사람 판단에만 의존한다.** 훅 인터프리터는 `py -3` —
  이 머신의 `python3`는 MS Store 별칭 스텁이라 실행되지 않는다(exit 49).
- **훅은 명령 문자열 전체를 본다.** 위험 패턴을 *언급만* 하는 텍스트도 걸린다 — 대표적으로 그 패턴을
  설명하는 커밋 메시지, 그리고 이 절을 쓰는 문서 편집 명령. 해결책은 예외 추가가 아니라 텍스트를
  명령에서 빼는 것이다. 커밋에는 **`git commit -F <파일>`을 쓰고**(`-m "..."`이나 heredoc은 메시지가
  명령 문자열에 들어간다), 파일 편집에는 셸을 거치지 않는 편집 도구를 쓴다.

## 작업 방식

- **막힌 항목은 우회하지 않는다.** 무엇이 어디서 막혔는지 에러 원문과 `file:line`으로 적고 거기서
  멈춘다. 특히 에디터나 사람 손이 필요한 작업은 우회로를 찾지 말고 기록 후 인계한다 — 우회는 계획에
  없던 상태를 만들고, 그 상태는 다음 세션이 모른다.
- **계획 밖에서 발견한 부채는 즉석에서 고치지 않는다.** `docs/tech-debt.md`에 기록하고 넘어간다.
  즉석 수정은 계획과 실제 diff의 대조를 무의미하게 만들고 범위를 두 배로 키운다.
- 아직 착수하지 않은 작업 후보는 `docs/backlog.md`에 적는다. `tech-debt.md`에 순서를 적지 않는다.

## 아키텍처 핵심 규칙 (상세: `docs/ARCHITECTURE.md`)

- **인증 티어와 게임 티어를 잇는 건 Redis뿐이다. 게임 서버는 `UserDB`를 직접 건드리지 않는다.**
- **`Room`이 `JobQueue`를 상속한다.** 패킷 핸들러는 인라인으로 일하지 않고 `room->DoAsync(...)`로
  잡만 밀어넣고 리턴한다. 룸 소유 상태는 큐 위에서 직렬화되므로 락이 없다.
  **다른 룸의 오브젝트에 직접 손대지 않는다.** DB 작업도 `DBQueue`에 push한다.
- **클라: 네트워크 스레드는 UObject를 절대 만지지 않는다.**
- **클라/서버 클래스 계층이 대칭이다.** 게임플레이 변경은 클라 + 서버 + 프로토콜,
  **3곳 수정을 기본으로 생각할 것.**

## Git 작업 규칙

Conventional Commits와 gitmoji를 기준으로 하고, 아래에 적힌 차이만 따른다.

### 커밋

형식: `<gitmoji> <type>: <제목>`

예: `✨ feat: psmux 세션 자동 복구 옵션 추가`

- 제목을 한국어 명령형으로 쓴다. `추가했음`처럼 과거형을 쓰지 않는다.
- 이모지 매핑은 gitmoji 표준을 따른다. (참고: https://gitmoji.dev)
- type은 Conventional Commits의 11개만 쓴다.

### 브랜치

형식: `<카테고리>/<이슈번호>-<요약>`

예: `feature/APP-123-add-login-button`

- 카테고리는 `feature`, `bugfix`, `hotfix`, `refactor`, `release`, `docs`, `chore` 중 하나를 쓴다.
- `bugfix`는 개발 중 발견한 버그에, `hotfix`는 운영 환경 긴급 수정에 쓴다.
- 이슈 번호가 없으면 생략한다.

### PR

- 제목을 커밋 메시지와 같은 형식으로 쓴다.
— squash merge를 쓰므로 PR 제목이 커밋 메시지가 된다.
- 본문은 `.github/pull_request_template.md`의 절 구성을 채운다.
- `gh pr create`에 `--body`가 아니라 `--body-file`을 쓴다.
— `--body`에 여러 줄을 넣으면 줄바꿈과 백틱이 깨진다.
- 직접 실행해 확인한 것만 「검증」에 적는다. 실행하지 않았으면 `없음`이라고 적는다.
- 「확인하지 못한 것」과 「머지 시 주의」를 비워두지 않는다. 없으면 `해당 없음`이라고 적는다.
- 「먼저 볼 곳」에 확신이 낮은 판단을 함께 적는다.

## 컨벤션

**인코딩 — `.proto`와 `.bat`은 cp949, 나머지는 전부 UTF-8.** `.gitattributes`와 두 개의
`.editorconfig`가 이를 강제한다. 이 파일들의 한국어 주석을 UTF-8로 읽으면 깨져 보인다.
정상이며, UTF-8로 다시 저장해서 "고치면" 이 파일들을 소비하는 툴 쪽이 깨진다.

**문서 파일명은 소문자와 대시**를 쓴다(`tech-debt.md`). 예외는 루트의 관례 파일과 각 폴더의
진입점뿐이다 — `CLAUDE.md`, `README.md`, `docs/ARCHITECTURE.md`.

**주석과 로그는 한국어로 작성한다.** 코드를 수정할 때 주변 언어에 맞출 것.

빌드 구성·모듈·include 규칙은 `docs/build.md`.

## 기존 코드베이스 방침

- 이 프로젝트는 손코딩 베이스다. 뒤죽박죽인 부분이 많음을 전제한다.
- 잘못된 설계 발견 시: 즉시 고치지 않는다 → `docs/tech-debt.md`에 기록 → 계획된 리팩토링에서만 수정.
- 목표 구조: 관심사별 레이어 분리, BP 위젯 로직의 C++ 이관, 캐릭터 클래스의 컴포넌트 분리.
- 신규 코드는 목표 구조를 따른다. 기존 코드는 계획 없이 구조를 건드리지 않는다.
