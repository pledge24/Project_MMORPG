# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 프로젝트 개요

반드시 함께 띄워야 하는 3티어 MMORPG. 포트폴리오용.

| 티어 | 경로 | 스택 | 포트 |
|---|---|---|---|
| 게임 클라이언트 | `P1/` | Unreal Engine 5.8, 모듈명 `P1` | — |
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
- 클라(`P1`)와 서버(`Server`) 솔루션을 오가므로 Rider MCP 툴에는 projectPath를 항상 명시한다.
- 린트·진단: `lint_files`, `get_file_problems`. 심볼 리네임: `rename_refactoring` (텍스트 치환 금지).
- 에디터·에셋·PIE 조작: UE MCP (도입 후. 도입 전에는 사람에게 요청).
- 서버 변경 검증은 Unreal을 띄우지 않고 `Server/DummyClient/`로 가능하다 (실 클라와 동일 프로토콜).

## 완료 기준

- C++ 편집 후 반드시 빌드로 검증한다(`build_solution_start` → `build_solution_state`).
  빌드 통과 없이는 완료가 아니다.
- 자동 테스트는 아직 없다 (구축 예정 — 아래 테스트 계층 참조). 그전까지:
  서버 프로토콜/핸들러 변경은 DummyClient로 스모크 확인, 인증 서버 변경은 `npm start` 기동 확인.
- 검증 없이 "완료했다"고 보고하지 않는다. 검증 불가한 부분은 불가하다고 명시한다.

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
- 아키텍처 결정은 `docs/decisions/`에 ADR로 남긴다 (날짜-제목.md).
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

## 테스트 계층 (구축 예정 — 현재 자동 테스트·CI 없음)

- L1 순수 로직: 서버 GoogleTest / UE Low-Level Tests. 입력 없음. 재화·트랜잭션·직렬화는 여기서.
  서버는 VS 솔루션 기반이므로 통합 방식(NuGet vs 프로젝트 추가)은 세션 2에서 결정한다.
- L2 게임 로직+입력: Simple Automation Test + `InjectInputForAction`.
- L3 UI 입력: Automation Spec + Automation Driver. Live Coding 비호환 — TDD 루프 금지, 배치 전용.
- L4 E2E: Gauntlet TestController. DummyClient 자산 재사용 검토. 병렬 실행 시 포트 파라미터화.
- L1 UE(LLT) 실행은 Rider Run Configuration 호출로만 — Rider Unit Tests 창은 L2 전용이라 LLT를 인식 못 한다.

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
