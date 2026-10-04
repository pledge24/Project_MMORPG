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

- 티어 경계, 스레드, 패킷 경로, DB 접근을 바꾸기 전에 `docs/ARCHITECTURE.md` — 깨면 안 되는 불변식
- 코드를 쓰거나 고치기 전에 `docs/conventions.md` — 이름, 주석, 멤버 배치, 타입 사용
- 새 파일이나 에셋을 만들기 전에 `docs/folder-structure.md` — 어디에 둘지, 무슨 이름을 붙일지
- 구조를 바꾸거나 ADR을 쓰기 전에 `docs/adr/` — 되돌리기 어려운 결정의 기록. 판정 기준과 형식은 `.claude/skills/domain-modeling/ADR-FORMAT.md`.
  기존 ADR에 덧붙일 때는 덧붙인 날짜를 그 자리에 적는다 — 본문이 측정 날짜에 묶여 있다
- 결함을 찾았거나 고치기 전에 `docs/tech-debt.md` — 지금 틀린 것
- 다음 작업을 고르거나 작업 후보를 적기 전에 `docs/backlog.md` — 아직 착수하지 않은 작업 후보와 하지 않기로 확인된 것
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

## 빌드와 도구

- 빌드는 터미널에서 돌리고 종료 코드로 판정한다. 빌드에 Rider MCP를 쓰지 않는다. 클라이언트는
  `pwsh P1/Scripts/Invoke-UeBuild.ps1`, 서버는 `MSBuild`다. 명령 원문은 `docs/build.md`에 있다.
- 클라이언트 빌드 스크립트는 에디터가 떠 있으면 닫지 않고 1로 끝난다. `-CloseEditor`는 저장하지 않은
  에셋 변경이 없다고 판단한 뒤에만 붙인다.
- Rider MCP 툴을 쓰기 전에 `rider-mcp` 스킬을, 언리얼 MCP(`unreal`) 툴을 쓰기 전에 `ue-mcp` 스킬을 읽는다.
  두 서버 모두 이 저장소에서만 통하는 함정이 있다.
- Rider MCP는 `.claude/settings.json`의 `permissions.deny`가, 언리얼 MCP는 훅의 툴셋 허용 명단이 막는다.
  둘 다 사람만 고친다. 막힌 툴은 우회하지 않고 사람에게 요청한다.

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
- 진행 상태는 work 파일의 진행 표로 판단한다. 통합 브랜치로 머지된 티켓의 이슈는 `close-merged-issues`
  워크플로가 닫으므로, 워크플로가 실패하면 머지된 티켓의 이슈도 열려 있다.

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
- 사람은 PR을 머지하면서 원격 브랜치도 지운다. 「머지완료」를 들으면 로컬을 이 순서로 정리한다.
  1. PR의 base 브랜치로 전환하고 `git pull --ff-only`
  2. `git branch -d <브랜치>` — 남아 있는 원격 추적 브랜치와 비교하므로 squash 머지여도 통과한다
  3. `git fetch --prune` — 원격 추적 브랜치가 사라지면 2번이 거부되므로 반드시 2번 뒤에 한다
- 강제 삭제(`-D`, `-d -f`)는 훅이 막는다. 2번이 거부되면 에러 원문과 함께 사람에게 삭제를 요청한다.

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
