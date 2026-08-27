# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 프로젝트 개요

반드시 함께 띄워야 하는 3티어 MMORPG. 포트폴리오용.

| 티어 | 경로 | 스택 | 포트 |
|---|---|---|---|
| 게임 클라이언트 | `P1/` | Unreal Engine 5.8 (**런처 설치본 고정**), 모듈명 `P1` | — |
| 게임 서버 | `Server/GameServer/` | C++20, 자체 IOCP 코어 | `127.0.0.1:7777` |
| 인증 서버 | `Server/AuthServer/` | Node.js / Express (ESM) | `.env`의 `PORT` (클라는 `5000`을 기대) |

저장소: SQL Server LocalDB — GameDB는 `(localdb)\ProjectModels`(GameServer, config.h),
UserDB는 `(localdb)\MSSQLLocalDB`(AuthServer, .env). 인스턴스가 서로 다름에 주의.
그리고 두 서버가 공유하는 액세스 토큰 저장소로 Redis(`127.0.0.1:6379`).
Rider의 DB 연결은 읽기 전용 계정(`claude_ro`)을 사용한다.

## 도구 라우팅 — Rider MCP 전용 원칙

- 심볼·호출 관계 탐색: `analyze_calls`(INCOMING/OUTGOING, depth 2~3부터), `search_symbol`.
  grep 금지 — UE RPC의 `_Implementation` 접미사에서 호출 사슬이 끊긴다.
- 빌드 검증: `build_solution_start`(+ `filesToRebuild`로 범위 축소) → `build_solution_state`로 폴링.
  `rebuild=true` 금지. UBT/Build.bat/MSBuild를 터미널로 직접 돌리지 않는다 — 출력 절단으로 에러가 유실된다.
  `build_solution_state`가 진단 없이 실패만 돌려주면 Rider 빌드 로그를 직접 읽는다
  (`%LOCALAPPDATA%/JetBrains/Rider<버전>/log/SolutionBuilder/`).
  참고: Rider 공식 문서와 IDE의 `Settings > Tools > MCP Server > Exposed Tools`에는
  `build_project`도 있다. 다만 이 세션이 붙는 엔드포인트는 그걸 내놓지 않는다(실측).
  **판단 기준은 문서가 아니라 세션에 실제로 노출된 툴 목록이다** — 문서에 있다는 이유로
  위 이름을 `build_project`로 되돌리지 말 것. 반대로 노출 목록에 없다고 해서
  "그런 툴은 없다"고 단정하지도 말 것. 둘은 다른 얘기다.
- 클라(`P1`)와 서버(`Server`) 솔루션을 오가므로 Rider MCP 툴에는 **`rootFolder`를 항상 명시한다**
  (파라미터 이름이 `projectPath`가 아니다). Rider 인스턴스 하나가 열린 솔루션 전부를 한
  엔드포인트로 서빙하고, 대상을 고르는 건 `rootFolder`뿐이다.
  **함정: 솔루션이 하나만 열려 있으면 서버가 모호성을 못 느껴 거부하지 않고 그대로 실행한다.**
  Server를 빌드하려는데 P1만 열려 있으면 P1이 빌드된다. 그래서 빌드·실행·리팩토링 전에는
  의도한 솔루션이 실제로 열려 있는지도 확인한다 — 인자 없이 `get_run_configurations`를 부르면
  열린 프로젝트 목록이 에러 메시지로 돌아온다.
  상태를 바꾸는 Rider 툴은 `rootFolder`가 없으면 `.claude/hooks/guard_dangerous_cmd.py`가 차단한다.
- 린트·진단: `lint_files`, `get_file_problems`. 심볼 리네임: `rename_refactoring` (텍스트 치환 금지).
- 에디터·에셋·PIE 조작: UE MCP (도입 후. 도입 전에는 사람에게 요청).
- 서버 변경 검증은 Unreal을 띄우지 않고 `Server/DummyClient/`로 가능하다 (실 클라와 동일 프로토콜).

## 완료 기준

- C++ 편집 후 반드시 빌드로 검증한다(`build_solution_start` → `build_solution_state`).
  빌드 통과 없이는 완료가 아니다.
- **테스트가 존재하는 영역은 테스트 통과까지가 완료다.** 현재 적용 범위는 **게임 서버**와
  **인증 서버**(아래 테스트 계층의 실행 경로). 빌드 초록은 그 영역에서 더 이상 완료 신호가 아니다.
- 테스트가 없는 영역(UE 클라 전체, 서버의 룸·DB 경로)은 그전과 같다:
  서버 프로토콜/핸들러 변경은 DummyClient로 스모크 확인, 인증 서버 기동은 `npm start`.
- 검증 없이 "완료했다"고 보고하지 않는다. 검증 불가한 부분은 불가하다고 명시한다.
- **자기신고를 믿지 않는다.** 검증 커맨드를 실제로 실행한 결과 없이 완료를 선언하지 않는다.
  "됐을 것이다"는 완료가 아니다 — 코드를 고친 사실과 그 코드가 도는 사실은 별개다.
- **존재 ≠ 가능.** 구성요소가 있는 것을 확인하고 "그러니 이 기능이 된다"고 쓰지 않는다.
  가장 싸게 실패하는 경로를 먼저 돌려보고 나서 쓴다. 도구 라우팅 절의 `노출 ≠ 존재`와 짝이며,
  **부분 확인이 오히려 확신을 키우는 것**이 이 실수의 공통 형태다(구멍 하나를 찾아 메우면
  나머지를 검증했다고 느낀다). 실제 사례: UE LLT — 설치본에 Catch2·LowLevelTestsRunner가
  있는 것을 확인하고 "가능"으로 계획에 적었으나 빌드 자체가 거부됐다(2026-08-27).

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
- **훅은 명령 문자열 전체를 본다.** 그래서 위험 패턴을 *언급만* 하는 텍스트도 걸린다 —
  대표적으로 그 패턴을 설명하는 커밋 메시지. 해결책은 예외 추가가 아니라 텍스트를 명령에서 빼는 것:
  `git commit -F <파일>`을 쓴다 (`-m "..."`이나 heredoc은 메시지가 명령 문자열에 들어간다).

## 작업 방식

- 복잡한 작업은 `docs/plans/active/{작업명}.md`에 계획을 먼저 쓰고, 진행하며 체크박스·결정사항을
  갱신한다. 완료 시 `docs/plans/completed/`로 이동.
- **막힌 항목은 지우지 않는다.** `[B] blocked — 무엇이/어디서` 로 표기하고 거기서 멈춘다.
  특히 에디터·사람 손이 필요한 작업은 우회로를 찾지 말고 기록 후 인계한다 — 우회는 계획에
  없던 상태를 만들고, 그 상태는 다음 세션이 모른다.
- **완료 항목 옆에는 summary 한 줄**을 남긴다 (생성·수정한 파일 경로 + 핵심 결정).
  체크박스만 남으면 무엇이 왜 그렇게 됐는지가 사라진다. 차기 세션 프롬프트의 원료다.
- 아키텍처 결정은 `docs/decisions/`에 ADR로 남긴다 (날짜-제목.md).
- **세션 기록은 원본과 색인을 나눈다.** 원본(재현성)은 `.claude/transcripts-archive/`의
  transcript jsonl — 세션 종료 시 `archive_transcript.py` 훅이 자동 복사한다(gitignore).
  색인(어느 세션이었는지 찾기)은 `docs/reports/`의 사람용 리포트. 둘의 역할을 섞지 않는다 —
  기록 시스템이 둘이면 한쪽이 부패하는데, 역할이 다르면 둘 다 필요하다.
- 발견한 부채는 즉시 고치지 말고 `docs/tech-debt.md`에 기록한다.
- 같은 실수가 두 번 반복되면 이 파일에 규칙 한 줄 추가를 제안하고, 세 번이면 훅/규칙 승격을 제안한다.
- 이 파일이 비대해지면 서술적 내용은 `docs/`로 옮기고 여기엔 규칙과 목차만 남긴다.

## 코드 생성 — 프로토콜·데이터 파일을 고치기 전에 반드시 읽을 것

생성 결과물을 클라이언트/서버 트리로 **복사**하는 파이프라인이 두 개 있다. 복사본이 git에 커밋되어
있어서 직접 수정해도 되는 파일처럼 보이지만 아니다. 생성기를 다시 돌리는 순간 덮어써진다.

### 와이어 프로토콜

원본: `Server/Common/Protobuf/bin/{Enum,Struct,Protocol}.proto`

재생성은 `Server/Common/Protobuf/bin/GenPackets.bat` 실행. 이 스크립트는 `protoc.exe`를 돌리고,
`PacketHandlerGenerator.exe`를 방향별로 두 번 실행한 뒤, 결과를 `XCOPY`로 아래에 뿌린다.

- `Server/GameServer/Protocol/` — `*.pb.{h,cc}`
- `Server/GameServer/Main/` — `ServerPacketHandler.h`
- `Server/DummyClient/Protocol/` — `*.pb.{h,cc}`
- `Server/DummyClient/Main/` — `ClientPacketHandler.h`
- `P1/Source/P1/Network/` — `*.pb.{h,cc}` 및 `.proto` 3개
- `P1/Source/P1/` — `ClientPacketHandler.h` (`Network/`가 아니라 **모듈 루트**)

생성되는 건 패킷 핸들러 **헤더뿐**이다. `.cpp`(`Server/GameServer/Main/ServerPacketHandler.cpp`,
`P1/Source/P1/ClientPacketHandler.cpp`)는 손으로 작성한 파일이고, 새 패킷의 실제 동작은 여기에 구현한다.

이름 규칙이 기능적으로 중요하다. 생성기가 접두사로 핸들러를 나누므로 `C_*`는 클라→서버,
`S_*`는 서버→클라다. 요청/응답 쌍은 이름을 공유한다 (`C_MOVE`/`S_MOVE`).

### 게임 데이터

원본: `Server/Common/GameDatasheet/Original_*.xlsx` (Item, Map, Monster, Quest, Warrior_Level_Data)

재생성은 `Server/Common/GameDatasheet/GenJsonFile.bat` 실행. 각 워크북을 JSON으로 변환한 뒤
`S_*.json`은 `Server/GameServer/Game/Data/Json/`으로, `C_*.json`은 `P1/Content/Gamedata/`로 **MOVE**한다.
서버는 부팅 시 `Gamedata::LoadAllGamedata()`(`Server/GameServer/Game/Data/Gamedata.h`)로 이 파일들을 읽어
템플릿 ID를 키로 하는 `nlohmann::json` 기반 `DataTable`에 올린다.

두 생성기 exe 자체는 `Server/Tools/`의 파이썬 소스(`PacketHandlerGenerator/`,
`ExcelToJsonConverter/`)를 각 툴의 `MakeExe.bat`으로 PyInstaller 번들링해서 만들며, 결과 exe는
호출하는 `.bat` 옆에 놓인다.

## 빌드 및 실행

전체를 한 번에 띄우는 스크립트는 없다. 티어별로 따로 빌드하고 실행한다.

**서버** — `Server/Server.sln` (x64). C++ 3개(`ServerCore` · `GameServer` · `DummyClient`)와
파이썬 생성기 2개(`*.pyproj`)만 들어 있다. **AuthServer는 솔루션에 없다** — Node 프로젝트라
MSBuild가 `.NETCoreApp,v6.0` 참조를 요구하며 실패해서, 솔루션 빌드 신호를 상시 빨강으로 만들었다.
`npm start`로만 다룬다.
`Server.slnLaunch.user`에 GameServer + DummyClient를 동시에 띄우는 다중 시작 프로필이 정의되어 있다.
빌드 순서가 중요하다. `ServerCore`는 정적 라이브러리이고 `GameServer`와 `DummyClient`가 이를 링크한다.
산출물은 `Server/Binary/{Debug,Release}/`.
접속 문자열은 환경변수가 아니라 `Server/GameServer/config.h`에 컴파일 타임 상수로 박혀 있다
(gitignore됨 — 새로 클론하면 직접 만들어야 한다).

**인증 서버**

```bash
cd Server/AuthServer
npm install
npm start                # = node src/app.js
```

설정은 gitignore된 `.env`를 `src/Config/configs.js`가 읽는다 (`PORT`, `DB_CONNECTION_STRING`,
`DB_DRIVER`, 커넥션 풀 크기, `REDIS_HOST`, `REDIS_PORT`, `ACCESS_TOKEN_TTL`). `eslint`가
devDependency로 설치돼 있지만 flat config도 lint 스크립트도 없어서 실제로 연결돼 있진 않다.

**클라이언트** — `P1/P1.uproject`를 열거나(Unreal Engine 5.8), C++ 작업은 `P1/P1.sln`로 연다.
소스 파일을 추가한 뒤에는 `.uproject` 우클릭 메뉴로 프로젝트 파일을 재생성한다.

> **엔진은 런처 설치본(Installed Build)만 쓴다. 소스 빌드는 이 프로젝트의 선택지가 아니다.**
> 확정된 제약이며 재검토하지 않는다. 무언가를 계획하기 전에 이것부터 본다.
>
> 그래서 **불가능한 것 두 가지**:
> - **프로젝트 안의 `TargetType.Program` 타깃** — UBT가 이런 타깃을 무조건 고유 빌드 환경으로
>   잡고 설치본이 이를 거부한다. UE Low-Level Tests(Catch2)가 여기 걸려 채택하지 않았다
>   (`docs/decisions/2026-08-27-l1-test-infra.md` 결정 6). 스탠드얼론 툴이 필요하면
>   `Server/DummyClient`처럼 UE 밖에서 만든다.
> - **엔진 소스 패치** — 엔진 버그를 만나면 프로젝트 코드 안에서 우회하는 수밖에 없다.
>
> 영향이 **없는** 것: 게임 빌드·실행·패키징, 프로젝트 플러그인(플러그인 모듈은 프로젝트 쪽에서
> 컴파일된다), 엔진 코드 디버깅(설치본도 `Engine/Source`를 딸려 준다), 서버·인증 티어 전부.

**데이터베이스** — 스키마는 SQL 스크립트로 수동 적용한다. 인증은
`Server/Queries/UserDB_CreateUsersTable.sql`, 게임은 `Server/GameServer/Queries/GameDB_CreateAllTables.sql`
(및 같은 폴더의 `AlterTable.sql`, `GameDB_InsertAdminAccount.sql`, `GameDB_GetMaxItemUid.sql`).

## 아키텍처 핵심 규칙 (상세: docs/ARCHITECTURE.md)

- 로그인은 3티어를 모두 거친다: 클라 → 인증 서버(HTTP, bcrypt 검증, 토큰을 Redis에 TTL 저장) →
  게임 서버(`C_LOGIN`에서 토큰을 Redis로 재검증). **인증 티어와 게임 티어를 잇는 건 Redis뿐이다.
  게임 서버는 `UserDB`를 직접 건드리지 않는다.**
- **동시성: `Room`이 `JobQueue`를 상속한다.** 패킷 핸들러는 인라인으로 일하지 않고
  `room->DoAsync(...)`로 잡만 밀어넣고 리턴한다. 룸 소유 상태는 큐 위에서 직렬화되므로 락이 없다.
  게임 기능을 추가할 때 이 형태를 따르고, **다른 룸의 오브젝트에 직접 손대지 않는다.**
  DB 작업도 핸들러에서 직접 하지 않고 `DBQueue`에 push한다.
- **클라: 네트워크 스레드는 UObject를 절대 만지지 않는다.** 수신 패킷은 게임 스레드의
  `UP1GameInstance::HandleRecvPackets()`가 비우고, 델리게이트로 액터/위젯에 전파한다.
- **클라/서버 클래스 계층이 대칭이다** (`Object → Creature → { Player, Monster }`).
  게임플레이 변경은 클라 + 서버 + 프로토콜, **3곳 수정을 기본으로 생각할 것.**

## 테스트 계층

설계 근거는 `docs/decisions/2026-08-27-l1-test-infra.md`. 여기엔 실행 경로만 적는다.

### 지금 도는 것

| 대상 | 빌드 | 실행 (에이전트·CI) | 실행 (사람·IDE) |
|---|---|---|---|
| 게임 서버 L1 (GoogleTest) | `build_solution_start(rootFolder=.../Server)` → `build_solution_state` | `Server/Binary/Debug/GameServerTests.exe` | Rider 실행 구성 `GameServerTests` |
| 인증 서버 | — | `cd Server/AuthServer && npm test` | Rider npm 구성 |

**판정은 종료 코드다.** 0이 아니면 실패다. 테스트는 `Server/GameServerTests/`,
gtest는 `Server/Libraries/googletest/`에 벤더링돼 있다(v1.18.0, gmock 없음).

**실행만 셸을 쓰는 이유** — `execute_run_configuration`은 호출마다 Rider가 확인 대화상자를 띄우고,
그걸 끄는 수단은 Brave 모드(IDE 전역으로 셸·실행구성 확인 해제)뿐이라 쓰지 않는다.
"UBT/MSBuild를 터미널로 직접 돌리지 않는다"는 규칙의 근거는 **출력 절단으로 에러가 유실되는 것**이고,
그건 빌드에만 성립한다 — gtest 출력은 짧고 완결적이다. 그래서 **빌드는 Rider, 실행은 셸**로 가른다.
`Server/Binary/`는 gitignore되어 있으므로 실행 전 빌드는 필수다.

**테스트를 추가할 때** — `.cpp`를 `Server/GameServerTests/GameServerTests.vcxproj`의
`<ItemGroup Label="테스트 소스">`에 등록해야 한다(이 프로젝트는 파일 자동 수집을 하지 않는다).

### 아직 없는 것 — 무엇을 확인했고 무엇을 안 했는지

**UE 클라의 기본 경로는 L2다.** 별도 빌드 타깃이 필요 없어 `P1` 모듈에 그대로 컴파일된다.

아래는 전부 **미착수**다. 「완료 기준」의 "존재 ≠ 가능"을 여기에도 적용해, 확인한 것과
확인하지 않은 것을 갈라 적는다. **착수할 때는 가장 싸게 실패하는 경로부터 돌린다** —
파일이 있는지 여러 번 확인하는 것보다 한 번 빌드해 보는 게 싸다.

| 계층 | 확인한 것 | 확인 안 한 것 |
|---|---|---|
| L2 게임 로직+입력 (Simple Automation Test + `InjectInputForAction`) | 설치본 `Core/Public/Misc/AutomationTest.h`에 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` 존재 | **실행 전체.** 컴파일·에디터 실행 다 안 해봤다 |
| L3 UI 입력 (Automation Spec + Automation Driver) | 없음 — **아직 안 봤다** | 전부 |
| L4 E2E (Gauntlet TestController) | 설치본에 `Engine/Plugins/Experimental/Gauntlet` 플러그인 + public `GauntletTestController.h` + 컴파일된 `Gauntlet.Automation.dll` | **실행 전체** |

- L2·L3 실행에는 **에디터가 필요하다** (`Window > Test Automation` 또는
  `-ExecCmds="Automation RunTests ..."`). 서버처럼 무인 루프가 되지 않는다.
- L3는 Live Coding 비호환 — TDD 루프 금지, 배치 전용.
- L4는 DummyClient 자산 재사용 검토. 병렬 실행 시 포트 파라미터화.
- CI: D-12. 서버 쪽 전제는 갖춰졌다 — `GameServerTests`는 gitignore된 `config.h` 없이 빌드된다.
  UE 쪽은 에디터 의존 때문에 별도 검토가 필요하다.

**UE L1(Low-Level Tests)은 채택하지 않는다.** 런처 설치본에서 빌드가 거부되고
(프로젝트 내 `TargetType.Program` 타깃 문제 — 위 「빌드 및 실행」의 엔진 제약 참조),
이 프로젝트는 소스 빌드 엔진을 쓰지 않기로 확정했다. 손실은 작다 — L1으로 검증할 순수 로직은
전투 판정·인벤토리·레벨 테이블처럼 대부분 서버 소유이고 그쪽은 GoogleTest가 덮는다.
근거는 `docs/decisions/2026-08-27-l1-test-infra.md` 결정 6.

## 컨벤션

**인코딩 — `.proto`와 `.bat`은 cp949, 나머지는 전부 UTF-8.** `.gitattributes`와 두 개의
`.editorconfig`가 이를 강제한다. 그래서 이 파일들의 한국어 주석을 UTF-8로 읽으면 깨져 보인다
(`�Ϸ�Ǿ����ϴ�`). 정상이며, UTF-8로 다시 저장해서 "고치면" 이 파일들을 소비하는 툴 쪽이 깨진다.

**클라이언트 include는 평탄하다.** `P1/Source/P1/P1.Build.cs`가 게임플레이 하위 디렉터리를 전부
`PrivateIncludePaths`에 등록해 두어서, 헤더는 상대 경로가 아니라 파일명만으로 include한다
(`#include "Creature.h"`). 새 하위 디렉터리를 만들면 여기에도 등록해야 한다.

**`P1/Source/`에는 모듈이 두 개다.** 게임 모듈 `P1/` 외에 protobuf를 벤더링한 `ProtobufCore/`가 있고
(`ProtobufCore.Build.cs`, `Include/google/**`, `Lib/Win64/libprotobuf.lib` 16MB), `P1.Build.cs`가
이를 `PrivateDependencyModuleNames`로 링크한다. `.lib`은 `P1/.gitignore`의 `*.lib`에 걸리지만
클론 즉시 빌드되도록 강제 추적 중이다 — 실수가 아니다.

**git 추적 — 전역 커밋 규칙의 의도적 예외.** 이 리포는 `CLAUDE.md`·`.claude/`·`docs/`·`prompts/`를
**의도적으로 추적한다**(커밋 `01bd5a6`). 전역 규칙(`~/.claude/rules/commit.md`)은 이들을 추적 제외하라고
하지만 여기서는 따르지 않는다. 루트 `.gitignore`를 되돌리지 말 것.

**커밋**은 gitmoji + 한국어 설명: `<emoji> <type>: <설명>` — `🚧 wip:`, `✅ done:`, `♻️ refactor:`,
`🧹 clean:`, `🎉 add:`. wip 커밋에는 빌드 상태를 덧붙이는 경우가 많다 (예: `(코드 실행 안되는 상태)`).
기능 브랜치는 `#<issue>-<Topic>` 이름을 쓰고 PR로 `dev`에 머지한다.

**주석과 로그는 한국어로 작성한다.** 코드를 수정할 때 주변 언어에 맞출 것.

## 기존 코드베이스 방침

- 이 프로젝트는 하네스 도입 전에 작성된 손코딩 베이스다. 뒤죽박죽인 부분이 많음을 전제한다.
- 잘못된 설계 발견 시: 즉시 고치지 않는다 → `docs/tech-debt.md`에 기록 → 계획된 리팩토링에서만 수정.
- 목표 구조: 관심사별 레이어 분리, BP 위젯 로직의 C++ 이관, 캐릭터 클래스의 컴포넌트 분리.
- 신규 코드는 목표 구조를 따른다. 기존 코드는 `docs/plans/`에 계획 없이 구조를 건드리지 않는다.
