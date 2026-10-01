## 프로젝트 개요

반드시 함께 띄워야 하는 3티어 MMORPG.

| 티어 | 경로 | 스택 | 포트 |
|---|---|---|---|
| 게임 클라이언트 | `P1/` | Unreal Engine 5.8 (런처 설치본 고정), 모듈명 `P1` | — |
| 게임 서버 | `Server/GameServer/` | C++20, 자체 IOCP 코어 | `127.0.0.1:7777` |
| 인증 서버 | `Server/AuthServer/` | Node.js / Express (ESM) | `.env`의 `PORT` (클라는 `5000`을 기대) |

저장소: SQL Server LocalDB — GameDB는 `(localdb)\ProjectModels`, UserDB는 `(localdb)\MSSQLLocalDB`.
두 인스턴스는 서로 다르다. 두 서버가 공유하는 액세스 토큰 저장소로 Redis(`127.0.0.1:6379`).
**Claude는 `claude_ro` 연결(`GameDB`·`UserDB`)로만 SQL을 실행한다.** `(사용자 전용)` 연결과 셸의 DB
클라이언트(`sqlcmd` 등)는 훅이 막는다. DB에 쓸 일은 SQL 초안을 사람에게 넘긴다. 상세: `docs/build.md`

## 문서 위치

- `docs/ARCHITECTURE.md` — 깨면 안 되는 불변식
- 코드를 쓰거나 고치기 전에 `docs/conventions.md` — 이름, 주석, 멤버 배치, 타입 사용
- 새 파일이나 에셋을 만들기 전에 `docs/folder-structure.md` — 어디에 둘지, 무슨 이름을 붙일지
- `docs/adr/` — 되돌리기 어려운 결정의 기록. 판정 기준과 형식은 `.claude/skills/domain-modeling/ADR-FORMAT.md`.
  기존 ADR에 덧붙일 때는 덧붙인 날짜를 그 자리에 적는다 — 본문이 측정 날짜에 묶여 있다
- `docs/tech-debt.md` — 지금 틀린 것
- `docs/backlog.md` — 아직 착수하지 않은 작업 후보와 하지 않기로 확인된 것
- `docs/work/` — 여러 세션에 걸치는 작업의 진행 현황. 그 작업의 티켓을 집기 전에 해당 work 파일을 읽는다
- `.Build.cs`, `.Target.cs`, `.vcxproj`, 빌드 스크립트를 고치기 전에 `docs/build.md`
- `.proto`, `DesignData/`, 생성기를 고치기 전에 `docs/codegen.md`
- 테스트를 추가하거나 테스트 프로젝트를 고치기 전에 `docs/testing.md`
- `docs/reports/` — 세션 리포트. 사람용 설명 자료이고 git이 추적하지 않는다(루트 `.gitignore`).
  여기 적은 것 중 앞으로도 지켜야 할 결론은 규범 문서로 옮긴다. 리포트에만 적으면 저장소에 남지 않는다
- `docs/references/` — 2026-09에 걷어낸 하네스 v1의 이력. 현행 제약이 아니다. 현행 제약은 위 문서들의 본문에 있다

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
- **언리얼 MCP(`unreal`)는 `permissions`가 아니라 훅이 막는다.** `call_tool` 하나로 830개가
  들어오므로 `permissions`로는 구분되지 않는다. `.claude/hooks/guard_dangerous_cmd.py`가 툴셋
  허용 명단(`UE_ALLOWED_TOOLSETS`, 27개)을 1층으로, 보관소 경로와 슬레이트 조작과
  `execute_tool_script`를 아래 세 층으로 본다. **에셋 쓰기는 열려 있다.** **명단은 사람만
  고친다** — 훅 파일 편집은 auto mode classifier가 막는다. 층별 판정과 근거는
  `docs/adr/0003-gate-unreal-mcp-by-hook-whitelist.md`.
- **언리얼 MCP는 에디터가 떠 있어야 붙는다.** 에디터를 띄우면 `127.0.0.1:8000`이 자동으로
  열리므로 사람이 콘솔에 입력할 것은 없다. 연결 확인은 `netstat`로 8000 포트를 보거나
  `list_toolsets`를 한 번 부른다. **UE 자동화 테스트는 이것과 무관하다** —
  `P1/Scripts/Run-UeTests.ps1`이 에디터 없이 돌리고 종료 코드로 판정한다.
- 심볼 탐색: `skill_search`의 `mode=symbol`. 텍스트 탐색: `search_text`. **grep 금지** — UE RPC의
  `_Implementation` 접미사에서 호출 사슬이 끊긴다. 검색어는 접미사가 붙은 이름과 안 붙은 이름
  양쪽으로 잡는다. **`mode=symbol`의 좌표는 `1행 1열`로 고정되므로 파일 경로만 쓴다.**
- 호출자 확인은 `search_text`로 한다. `analyze_calls`는 C++ 심볼을 색인하지 않아 막아 두었다.
- 빌드 검증: **터미널에서 돌리고 종료 코드로 판정한다. 빌드에 Rider MCP를 쓰지 않는다.**
  클라이언트는 `P1/Scripts/Invoke-UeBuild.ps1`, 서버는 `MSBuild`다. 0이 아니면 같은 출력에 에러가
  코드와 파일과 줄과 함께 찍혀 있다. **빌드 스크립트는 에디터가 떠 있으면 닫지 않고 1로 끝난다.**
  `-CloseEditor`를 붙이는 것은 저장하지 않은 에셋 변경이 없다고 판단한 뒤다. 빌드 출력이 커서
  절단되는 문제는 스크립트가 로그를 파일로 보내고 오류 줄만 추려서 푼다. 명령 원문은
  `docs/build.md`, 근거는 `docs/adr/0001-unify-build-path.md`.
- **Rider MCP 툴에는 `rootFolder`를 항상 명시한다**(파라미터 이름이 `projectPath`가 아니다).
  솔루션이 하나만 열려 있으면 서버가 모호성을 못 느끼고 그대로 실행하므로, 생략해도 에러가
  나지 않는다. 열린 프로젝트 목록은 인자 없이 `get_run_configurations`를 부르면 에러로 돌아온다.
- 린트·진단: `lint_files`, `get_file_problems`. 심볼 리네임: `rename_refactoring` (텍스트 치환 금지).
- **`rename_refactoring`의 `applied: true`는 반영을 뜻하지 않는다.** 한 건마다 디스크를 확인하고
  파일 묶음이 끝나면 빌드로 판정한다. 시작 전에 사람에게 에디터 탭을 닫아 달라고 요청한다 —
  열린 탭은 저장되지 않으면서 성공을 보고한다. `no_renamable_symbol`로 거부되면 사람에게
  넘긴다. 실패 형태 셋과 사례: ADR-0002의 「`rename_refactoring`의 실패 형태 셋」
- UE 에셋 조회: `get_class_hierarchy`와 `search_assets`. **`search_assets`는 `baseClass`만 쓴다** —
  `query`는 빈 결과만 돌려준다. Rider의 에디터 조작 툴은 막혀 있으므로 사람에게 요청한다.
- **에셋 속성은 Rider가 아니라 언리얼 MCP로 읽는다.** Rider의 `get_asset_properties`는 블루프린트
  CDO에 `properties: []`를 돌려준다(ADR-0002). 같은 에셋을 `unreal`의
  `ObjectTools.list_properties`로 읽으면 속성이 나온다.
- **노출 ≠ 존재.** 판단 기준은 문서가 아니라 세션에 실제로 노출된 툴 목록이다. **IDE 화면의 체크
  상태도 근거가 아니다** — 이 엔드포인트에 반영되지 않는다. 근거와 예외: `docs/build.md`와 ADR-0002

## 완료 기준

- C++를 고친 뒤에는 빌드가 통과해야 완료다.
- 테스트가 있는 영역은 그 테스트까지 통과해야 완료다. 영역과 실행 경로는 `docs/testing.md`에 있다.
- **UE 테스트는 `UnrealEditor-Cmd`의 종료 코드로 판정하지 않는다.** 테스트가 실패해도, 필터가
  아무것도 못 맞춰도 `0`을 돌려준다. `pwsh P1/Scripts/Run-UeTests.ps1`을 쓰고 그 스크립트의
  종료 코드를 본다. 빌드와 이 스크립트는 에디터를 닫고 돌린다.
- 인증 서버 기동은 `npm start`로 확인한다.
- 완료를 보고할 때는 실제로 실행한 검증 명령과 그 결과를 함께 적는다. 실행하지 못한 검증은
  실행하지 못했다고 적는다 — 구성요소가 있다는 확인만으로 「그러니 된다」고 쓰지 않는다.

## 안전

- 에디터를 변경하는 UE MCP 작업 전에는 커밋 또는 셸브를 확인한다.
- 임의 코드를 실행하는 MCP 툴(`execute_tool`, `ue_execute_python`, `execute_terminal_command`)은
  무엇을 실행할지 먼저 보고한다. `ue_execute_python`은 훅이 덮지 못한다.
- 스키마 변경(ALTER/CREATE/DROP)은 사람 승인 후, 저장소의 SQL 스크립트 갱신과 함께만 실행한다.
  파괴적 DB·Redis 명령은 `.claude/hooks/guard_dangerous_cmd.py`가 막는다.
- Python은 `py -3`로 실행한다. 이 머신의 `python3`는 MS Store 별칭 스텁이라 실행되지 않는다(exit 49).
- 훅은 명령 문자열 전체를 보므로 위험 패턴을 언급만 하는 텍스트도 걸린다. 커밋 메시지는
  `git commit -F <파일>`로 넣고, 파일 편집에는 셸을 거치지 않는 편집 도구를 쓴다.

## 작업 방식

- 막힌 항목은 우회하지 않는다. 무엇이 어디서 막혔는지 에러 원문과 `file:line`으로 적고 거기서
  멈춘다. 특히 에디터나 사람 손이 필요한 작업은 우회로를 찾지 말고 기록 후 인계한다 — 우회는 계획에
  없던 상태를 만들고, 그 상태는 다음 세션이 모른다.
- 계획 밖에서 발견한 부채는 즉석에서 고치지 않는다. `docs/tech-debt.md`에 기록하고 넘어간다.
  즉석 수정은 계획과 실제 diff의 대조를 무의미하게 만들고 범위를 두 배로 키운다.
- 아직 착수하지 않은 작업 후보는 `docs/backlog.md`에 적는다. `tech-debt.md`에 순서를 적지 않는다.
- 진행 상태는 work 파일의 진행 표로 판단하고, 이슈의 열림·닫힘으로 판단하지 않는다 — 티켓 PR은 통합
  브랜치로 머지되므로 이슈가 자동으로 닫히지 않는다.

## 아키텍처 핵심 규칙 (상세: `docs/ARCHITECTURE.md`)

- 인증 티어와 게임 티어를 잇는 건 Redis뿐이다. 게임 서버는 `UserDB`를 직접 건드리지 않는다.
- `Room`이 `JobQueue`를 상속한다. 패킷 핸들러는 인라인으로 일하지 않고 `room->DoAsync(...)`로
  잡만 밀어넣고 리턴한다. 룸 소유 상태는 큐 위에서 직렬화되므로 락이 없다.
  다른 룸의 오브젝트에 직접 손대지 않는다. DB 작업도 `DBQueue`에 push한다.
- 클라이언트의 네트워크 스레드는 UObject를 만지지 않는다.
- 클라/서버 클래스 계층이 대칭이다. 게임플레이를 바꿀 때는 클라이언트, 서버, 프로토콜 세 곳을
  함께 고칠 대상으로 먼저 확인한다 — 한쪽만 고쳐도 빌드는 통과한다.

## Git 작업 규칙

Conventional Commits와 gitmoji를 기준으로 하고, 아래에 적힌 차이만 따른다.

### 커밋

형식: `<gitmoji> <type>: <제목>`

예: `✨ feat: psmux 세션 자동 복구 옵션 추가`

- 제목을 한국어 명령형으로 쓴다. `추가했음`처럼 과거형을 쓰지 않는다.
- type은 아래 11개만 쓰고, 이모지는 type마다 하나로 고정한다: `✨ feat` · `🐛 fix` · `📝 docs` · `🎨 style` · `♻️ refactor` · `⚡️ perf` · `✅ test` · `🏗️ build` · `👷 ci` · `🔧 chore` · `⏪️ revert`.

### 브랜치

형식: `<카테고리>/<이슈번호>-<요약>`

예: `feature/123-add-login-button`

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
- PR을 올린 뒤에는 PR 링크와 squash 제목, squash 본문을 사람에게 보고한다. 머지는 사람이 하고,
  그때 이 두 개를 그대로 붙여 넣는다. 본문은 커밋 메시지 형식으로 쓰고 PR 본문을 복사하지 않는다.

## 컨벤션

- `.proto`와 `.bat`은 cp949, 나머지는 전부 UTF-8이다. cp949 파일의 한국어 주석은 UTF-8로 읽으면
  깨져 보이는 것이 정상이다. UTF-8로 다시 저장하면 이 파일들을 소비하는 툴이 깨진다.
- 주석과 로그는 한국어로 작성한다. 코드를 수정할 때 주변 언어에 맞춘다.
- 문서 파일명은 소문자와 대시를 쓴다(`tech-debt.md`). 예외는 루트의 관례 파일과 각 폴더의
  진입점뿐이다 — `CLAUDE.md`, `README.md`, `docs/ARCHITECTURE.md`.
- 신규 코드는 `docs/folder-structure.md`의 목표 구조를 따른다. 기존 코드의 구조는 계획된
  리팩토링에서만 바꾼다.
