# 빌드 및 실행

빌드 구성을 건드릴 때만 읽는다.

---

## 전체 구성

전체를 한 번에 띄우는 스크립트는 없다. 티어별로 따로 빌드하고 실행한다.

| 티어 | 솔루션 / 명령 | 산출물 |
|---|---|---|
| 게임 서버 | `Server/Server.sln` (x64) | `Server/Binary/{Debug,Release}/` |
| 인증 서버 | `npm start` | — |
| 클라이언트 | `P1/P1.uproject` 또는 `P1/P1.sln` | UE 기본 경로 |

---

## 새로 클론했을 때

**git 훅을 켠다.** 저장소 루트에서 한 번 실행한다.

```
git config core.hooksPath .githooks
```

`.githooks/pre-commit`이 GitHub의 100MB 제한을 넘는 파일의 커밋을 막고, 50MB를 넘으면 경고한다. `core.hooksPath`는 로컬 git config에 들어가므로 커밋되지 않는다.

**클론마다 한 번씩 직접 설정한다.**
— 설정하지 않으면 훅이 없는 것과 같다. 그 상태에서는 경고도 실패도 남지 않아 누락을 알아챌 신호가 없다. 대용량 파일이 히스토리에 들어간 뒤 되돌리려면 히스토리를 다시 써야 한다.

---

## 게임 서버

`Server/Server.sln` (x64)에는 C++ 4개(`ServerCore` · `GameServer` · `DummyClient` · `GameServerTests`)와 파이썬 생성기 2개(`*.pyproj`)만 들어 있다. C++ 4개는 Debug와 Release 어느 구성에서도 빌드 대상이다. 파이썬 두 개는 `Build.0` 항목이 없어 빌드되지 않는다.

**AuthServer는 솔루션에 없다.** Node 프로젝트라 MSBuild가 `.NETCoreApp,v6.0` 참조를 요구하며 실패해서, 솔루션 빌드 신호를 상시 빨강으로 만들었다. `npm start`로만 다룬다.

`Server.slnLaunch.user`에 GameServer + DummyClient를 동시에 띄우는 다중 시작 프로필이 정의되어 있다.

**빌드 순서가 중요하다.** `ServerCore`는 정적 라이브러리이고 `GameServer`와 `DummyClient`가 이를 링크한다.

`GameServerTests`는 `GameServer`를 링크하지 않는다. `GameServer`가 exe라 링크할 수 없으므로, `GameServer.cpp`를 제외한 GameServer의 `.cpp`를 직접 컴파일한다. **이 프로젝트는 `.cpp`를 자동으로 모으지 않는다.** 테스트 파일을 추가하고 `.vcxproj`에 등록하지 않으면 그 테스트는 조용히 돌지 않는다. 자세한 것은 [테스트 계층](./testing.md)에 있다.

접속 정보와 부팅 설정은 `Server/GameServer/Core/Config.cpp`의 기본값을 쓰고, 아래 환경 변수가 있으면 그 값으로 덮어쓴다. 기본값이 이 저장소의 로컬 개발 환경이므로 새로 클론해도 따로 설정할 것이 없다. 테스트 DB를 가리킬 때는 환경 변수를 준다.

| 환경 변수 | 기본값 |
|---|---|
| `P1_GAME_DB_CONNECTION_STRING` | `(localdb)\P1Game`의 `GameDB`, 통합 인증 |
| `P1_REDIS_URI` | `tcp://127.0.0.1:6379` |
| `P1_GAME_SERVER_BIND_ADDRESS` | `127.0.0.1` |
| `P1_GAME_SERVER_PORT` | `7777` |
| `P1_GAME_SERVER_MAX_SESSIONS` | `30` |
| `P1_GAME_SERVER_WORKER_THREADS` | `5`. 메인 스레드가 하나 더 합류한다 |
| `P1_GAME_DB_THREADS` | `5` |
| `P1_GAME_DB_CONNECTIONS` | DB 스레드 수 |
| `P1_SERVER_NODE_ID` | Debug 빌드는 `0`. Release 빌드는 기본값이 없다 |
| `P1_WORLD_ID` | `1`. 이 서버가 맡은 월드의 번호다 |
| `P1_WORLD_NAME` | `로컬 월드`. 월드 목록에 보이는 이름이다 |
| `P1_WORLD_HOST` | `127.0.0.1`. 클라이언트가 이 월드에 접속할 IPv4 주소다. 포트는 `P1_GAME_SERVER_PORT`다 |
| `P1_WORLD_CAPACITY` | `100`. 혼잡도를 나누는 정원이고, 접속을 막지 않는다 |

숫자가 아니거나 범위 밖인 값은 경고를 남기고 기본값을 쓴다. 바인드 주소나 월드 주소가 IPv4 주소가 아니거나, 연결 수가 DB 스레드 수보다
작으면 서버가 사유를 로그에 남기고 종료 코드 1로 멈춘다. 환경 변수는 UTF-8로 바꿔 읽으므로 월드 이름에 한국어를 쓸 수 있다.

**노드 번호와 월드 번호는 틀린 값을 기본값으로 바꾸지 않는다.** `P1_SERVER_NODE_ID`가 0~1023의 정수가 아니거나, Release 빌드에서
값이 없으면 서버가 종료 코드 1로 멈춘다. `P1_WORLD_ID`가 1 이상의 정수가 아니어도 멈춘다. 서버마다 겹치지 않는 번호를 준다.
— 두 서버가 같은 월드 번호로 뜨면 한 월드 항목을 서로 덮어쓰고, 그 월드의 인증 티켓을 둘 중 아무 서버나 소비한다.
— 노드 번호는 캐릭터 ID와 아이템 UID에 들어간다. 두 서버가 같은 번호로 뜨면 두 서버가 발급한 ID가 겹친다. 근거는
`docs/adr/0016-issue-ids-with-snowflake.md`에 있다.
— 연결 풀은 DB 스레드마다 하나가 있어야 한다. 모자라면 동시에 돈 DB 잡 하나가 연결을 빌리지 못하고 실패한다. 부팅 때
`main`은 DB 스레드를 띄우기 전에 연결을 하나 빌려 아이템 번호를 읽고 돌려준다.

**게임 서버는 DB를 모두 준비한 뒤에 접속을 받는다.** 부팅 순서는 기획 데이터 → 설정 검증 → 룸 → DB와 Redis 연결 →
아이템 번호 → DB 스레드 → 리슨 → 워커 스레드다.
— 리슨부터 열면 DB 큐가 생기기 전에 온 로그인이 없는 큐를 고르고, 다음 아이템 번호가 정해지기 전에 아이템이 만들어질 수 있다.

---

## 인증 서버

```bash
cd Server/AuthServer
npm install
npm start                # = tsx --env-file=.env src/main.ts
npm run typecheck        # = tsc (출력 파일을 만들지 않는다)
```

설정은 gitignore된 `.env`에 둔다. 처음에는 `.env.example`을 `.env`로 복사한다. `npm start`가 Node의 `--env-file`
옵션으로 읽어 `process.env`에 넣고, `src/common/config.ts`가 기동할 때 키와 형식을 검사한다. 검사, `AccountDB` 연결, Redis 연결 중
하나라도 실패하면 에러 원문을 남기고 종료 코드 1로 끝난다. 셸에 같은 이름의 환경 변수가 있으면 `.env`보다 그 값이 우선한다.
**Redis를 인증 서버보다 먼저 띄운다.**
— 인증 서버는 Redis 첫 연결에 실패하면 다시 시도하지 않고 종료 코드 1로 끝난다.

**키를 더하거나 바꾸면 `src/common/config.ts`의 `REQUIRED_ENV_KEYS`, `.env.example`, 아래 키 표를 함께 고친다.** 키 변경은 PR의
「머지 시 주의」에도 적는다.
— `common/tests/config.test.ts`가 앞의 두 목록이 같은지, 예시 값이 검사를 통과하는지 대조한다. 키 표는 대조하지 않는다.
각자의 `.env`는 저장소 밖이라 git으로 전달되지 않는다.

Node 24가 필요하다. CI도 Node 24로 돈다.

| 키 |
|---|
| `PORT` |
| `DB_CONNECTION_STRING` |
| `DB_MAX_CONNECTION` |
| `DB_MIN_CONNECTION` |
| `DB_IDLE_TIMEOUT` |
| `REDIS_HOST` |
| `REDIS_PORT` |
| `TICKET_TTL_SECONDS` |

정적 검사는 ESLint 10이고 `npm run lint`로 돈다. 상세는 `docs/testing.md`에 있다.

**패키지는 아래 표에 있는 것만 둔다.** 새 패키지를 들이거나 버전 고정을 풀 때는 이 표를 함께 고친다.
「다시 볼 신호」가 나타나면 그 줄의 결정을 다시 검토한다.

| 패키지 | 쓰는 곳 | 이 선택을 유지하는 이유 | 다시 볼 신호 |
|---|---|---|---|
| `express` 5 | 라우터와 에러 핸들러 | 비동기 핸들러가 던진 에러를 에러 핸들러로 넘긴다. 핸들러에 `try`/`catch`가 없어도 된다 | - |
| `bcrypt` 6 | `common/password.ts` | 네이티브 패키지를 유지한다. 미리 빌드된 바이너리로 로드된다 | Node를 올렸을 때 그 버전용 바이너리가 없다 |
| `mssql` 12와 `msnodesqlv8` 5 | `common/db.ts`, `auth/auth.repository.ts` | LocalDB에 Windows 인증의 ODBC로 붙는다. `mssql`은 타입 정의를 싣지 않아, 쓰는 API만 `src/types/`의 선언 파일에 적는다 | `mssql`을 올린 뒤 선언 파일과 실제 API가 어긋난 것을 실행해서야 안다 |
| `redis` 5 | `common/ticket.ts` | 이 머신의 Redis는 3.0이라 node-redis 6의 기본 프로토콜 RESP3를 지원하지 않는다 | node-redis 5가 보안 패치를 끝내거나, 쓰려는 Node 버전을 지원하지 않는다 |
| `typescript` 6.0(`~`로 고정) | 타입 검사 | `typescript-eslint`가 지원하는 가장 높은 계열이다 | `typescript-eslint`가 7을 지원하기 시작한다 |
| `tsx` | `npm start` | 빌드 단계 없이 TypeScript를 실행한다 | 실행 시점의 변환 비용이나 `tsx`와 Node 버전의 호환 문제가 기동을 막는다 |
| `vitest`, `supertest` | 테스트 | 판정 테스트와 HTTP 계약 테스트를 한 러너로 돌린다 | - |
| `eslint`, `@eslint/js`, `typescript-eslint`, `globals` | 정적 검사 | `eslint.config.js`가 쓴다 | - |
| `@types/*` | 타입 검사 | `bcrypt`, `express`, `node`, `supertest`의 타입 정의다 | - |

`dotenv`는 두지 않고 Node의 `--env-file` 옵션으로 `.env`를 읽는다. 테스트나 스크립트가 `.env`를 따로 읽어야 하는
일이 반복되면 이 결정을 다시 본다. UUID는 `uuid` 패키지 대신 `node:crypto`의 `randomUUID`로 만든다.

---

## 클라이언트

`P1/P1.uproject`를 열거나(Unreal Engine 5.8), C++ 작업은 `P1/P1.sln`로 연다. 소스 파일을 추가한 뒤에는 `.uproject` 우클릭 메뉴로 프로젝트 파일을 재생성한다.

### 엔진 제약 — 무언가를 계획하기 전에 이것부터 본다

> **엔진은 런처 설치본(Installed Build)만 쓴다. 소스 빌드는 이 프로젝트의 선택지가 아니다.**
> 확정된 제약이며 재검토하지 않는다.

**불가능한 것 두 가지**

- **프로젝트 안의 `TargetType.Program` 타깃** — UBT가 이런 타깃을 무조건 고유 빌드 환경으로 잡고 설치본이 이를 거부한다. UE Low-Level Tests(Catch2)가 여기 걸려 채택하지 않았다 (`docs/testing.md` 「UE L1(Low-Level Tests)은 채택하지 않는다」). 스탠드얼론 툴이 필요하면 `Server/DummyClient`처럼 UE 밖에서 만든다.
- **엔진 소스 패치** — 엔진 버그를 만나면 프로젝트 코드 안에서 우회하는 수밖에 없다.

**영향이 없는 것**: 게임 빌드·실행·패키징, 프로젝트 플러그인(플러그인 모듈은 프로젝트 쪽에서 컴파일된다), 엔진 코드 디버깅(설치본도 `Engine/Source`를 딸려 준다), 서버·인증 티어 전부.

---

## 클라이언트 모듈과 include

**`P1/Source/`에는 모듈이 세 개다.** 게임 모듈 `P1/` 외에 protobuf를 벤더링한 `ProtobufCore/`가 있고(`ProtobufCore.Build.cs`, `Include/google/**`, `Lib/Win64/libprotobuf.lib` 16MB), `P1.Build.cs`가 이를 `PrivateDependencyModuleNames`로 링크한다. `.lib`은 `P1/.gitignore`의 `*.lib`에 걸리지만 클론 즉시 빌드되도록 강제 추적 중이다 — 실수가 아니다.

**에디터 모듈 `P1Editor/`는 에디터 타깃에만 들어간다.** `.uproject`에 `Type: Editor`로 등록했고 `P1Editor.Target.cs`의 `ExtraModuleNames`에 있다. 게임 타깃(`P1.Target.cs`)에는 넣지 않는다. 지금은 에셋 검증기(`Validation/`)와 그 테스트(`Tests/`)만 담는다. 엔진 플러그인 `DataValidation`에 의존하므로 `.uproject`에 그 플러그인을 에디터 전용으로 명시했다. 게임 모듈의 헤더는 `P1Editor.Build.cs`의 `PrivateIncludePaths`에 있는 `P1/`을 기준으로 `#include "Game/Data/P1GameDataSettings.h"`처럼 부른다. 게임 모듈의 타입을 에디터 모듈에서 쓰려면 그 타입에 `P1_API`가 있어야 한다. 없으면 `StaticStruct()` 같은 함수가 링크되지 않는다.

**클라이언트 include는 경로를 한정한다.** `P1/Source/P1/P1.Build.cs`의 `PrivateIncludePaths`에는 모듈 루트 `P1/`과 생성물 폴더 `P1/Network` 둘만 있다. 헤더는 파일명이 아니라 모듈 루트 기준 상대 경로로 include한다(`#include "Game/Entities/P1Creature.h"`). **새 도메인 폴더를 만들어도 여기에 등록하지 않는다.**

생성물 폴더만 예외로 남겼다. protobuf 생성 코드가 서로를 `#include "Common.pb.h"` 형태로 부르는데 생성물은 손으로 고치지 않으므로, 이 폴더를 빼면 생성기를 다시 돌릴 때마다 빌드가 깨진다. 이 결정의 근거는 `docs/adr/0005-drop-include-path-flattening.md`에 있고, 얻는 것과 얻지 못하는 것은 `docs/ARCHITECTURE.md`에 불변식으로 적혀 있다.

**서버 include도 경로를 한정한다.** `Server/`의 네 `.vcxproj`는 `IncludePath`에 프로젝트 루트와 솔루션 폴더(`$(SolutionDir)`), 생성물 폴더 `Protocol`, 서드파티 경로만 둔다. GameServerTests는 프로젝트 루트 대신 `$(SolutionDir)GameServer`와 `$(SolutionDir)GameServer\Protocol`을 쓴다. ServerCore 헤더는 `#include "ServerCore/Network/Session.h"`처럼 솔루션 폴더 기준으로 부른다. **새 폴더를 만들어도 `IncludePath`에 등록하지 않는다.**

`PrecompiledHeaderFile`도 경로다(`Core/pch.h`, `ServerCore/Core/pch.h`, `Main/pch.h`). `/Yu`는 `.cpp`의 include 문자열과 이 값이 글자 그대로 같아야 하므로, 한쪽만 바꾸면 C1010으로 컴파일이 멈춘다.

---

## 데이터베이스

스키마는 SQL 스크립트로 수동 적용한다.

| DB | 인스턴스 | 스크립트 |
|---|---|---|
| AccountDB | `(localdb)\P1Auth` | `Server/AuthServer/migrations/`의 파일을 번호 순서로 |
| GameDB · GameDB_Test | `(localdb)\P1Game` | `Server/GameServer/Migrations/`의 파일을 번호 순서로 두 DB에 |

**두 서버의 스키마는 각자의 마이그레이션 폴더에 번호 붙은 파일로 관리한다.** AccountDB는 `Server/AuthServer/migrations/`,
GameDB는 `Server/GameServer/Migrations/`다. 파일 이름은 `NNN_<이름>.sql`이고
첫 줄에 `-- NNN: 설명`을 적는다. 각 파일은 이미 적용된 DB에 다시 실행해도 결과가 같게 쓴다. 적용한 파일은 고치지 않고,
스키마를 바꿀 때는 다음 번호의 파일을 더한다. 적용 기록 테이블과 실행 도구는 두지 않고, 사람이 번호 순서로 실행한다.

**GameDB의 마이그레이션은 `GameDB`와 `GameDB_Test`에 함께 적용한다.** 한쪽에만 적용하면 실제 DB 테스트가 게임 서버와 다른
스키마를 검사한다. 게임 서버의 `.vcxproj`에는 마이그레이션 파일을 `None` 항목으로 등록한다. 파일을 더하면 `.vcxproj`와
`.filters`에도 더한다.

**인스턴스가 서로 다르다.** 접속 문자열의 출처도 다르다 — GameDB는 `Config`의 기본값과 환경 변수, AccountDB는 `.env`.

**새 머신에서 AccountDB를 만들 때는 사람이 아래 순서로 실행한다.** Claude는 인스턴스와 DB를 만들지 못한다(훅이 막는다).

1. `sqllocaldb create P1Auth -s`로 인스턴스를 만들고 시작한다
2. `SERVERPROPERTY('IsIntegratedSecurityOnly')`가 `0`인지 확인한다. `claude_ro`는 SQL 인증 로그인이므로 혼합 인증이 켜져 있어야 한다
3. `CREATE DATABASE AccountDB`를 실행하고, `Server/AuthServer/migrations/`의 파일을 번호 순서로 `-d AccountDB`로 실행한다. 각 파일은 이미 적용된 DB에서 다시 실행해도 결과가 같다
4. SQL 인증 로그인 `claude_ro`를 만들고, `AccountDB`의 사용자로 만들어 `db_datareader`에만 넣는다
5. Rider에 아래 표의 `AccountDB` 데이터 소스 두 개(`AccountDB`, `AccountDB (사용자 전용)`)를 만든다. 드라이버는 `Microsoft SQL Server LocalDB (JetBrains)`다. 일반 SQL Server 드라이버로 만들면 `localhost`의 TCP로 붙으려다 실패한다

**새 머신에서 GameDB를 만들 때도 사람이 아래 순서로 실행한다.** `claude_ro`와 `claude_rw`는 `P1Game`의 로그인이다.
`P1Auth`의 `claude_ro`와 이름은 같지만 인스턴스가 다르므로 따로 만든다.

1. `sqllocaldb create P1Game -s`로 인스턴스를 만들고 시작한다
2. `SERVERPROPERTY('IsIntegratedSecurityOnly')`가 `0`인지 확인한다. `claude_ro`와 `claude_rw`는 SQL 인증 로그인이다
3. `CREATE DATABASE GameDB`와 `CREATE DATABASE GameDB_Test`를 실행하고, `Server/GameServer/Migrations/`의 파일을 번호 순서로
   두 DB에 각각 실행한다
4. SQL 인증 로그인 `claude_ro`를 만들고, `GameDB`의 사용자로 만들어 `db_datareader`에만 넣는다
5. SQL 인증 로그인 `claude_rw`를 만들고, `GameDB`와 `GameDB_Test`의 사용자로 만들어 두 DB 모두에서 `db_datareader`와
   `db_datawriter`에만 넣는다
6. Rider에 아래 표의 GameDB 데이터 소스 네 개(`GameDB(claude_ro)`, `GameDB(claude_rw)`, `GameDB_Test(claude_rw)`,
   `GameDB(사용자 전용)`)를 AccountDB와 같은 드라이버로 만든다
7. 새로 만든 Claude 연결의 ID를 `.claude/hooks/guard_dangerous_cmd.py`의 `SQL_ALLOWED_CONNECTIONS`에 넣는다

Rider의 DB 연결은 세 벌이다.

| 데이터 소스 | 계정 | 쓰는 쪽 |
|---|---|---|
| `GameDB(claude_ro)` · `AccountDB` | 읽기 전용 `claude_ro` | Claude(Rider MCP)의 조회 |
| `GameDB(claude_rw)` · `GameDB_Test(claude_rw)` | 행 쓰기 전용 `claude_rw` | Claude(Rider MCP)의 행 쓰기 |
| `GameDB(사용자 전용)` · `AccountDB (사용자 전용)` | Windows 인증 | 사람. 스키마 변경과 마이그레이션 적용은 여기서 한다 |

`claude_rw`는 `P1Game`의 `GameDB`와 `GameDB_Test`에서 `db_datareader`와 `db_datawriter`에만 속한다.
그래서 행을 읽고 쓰는 문장만 실행되고, `CREATE`·`ALTER`·`DROP`·`TRUNCATE`는 DB가 권한 오류로 거부한다.
**문장의 제한은 훅이 아니라 DB 권한이 맡는다.** 훅의 문자열 검사는 주석, 동적 SQL, 문장 구분을 다 해석하지
못하지만 DB 권한은 우회할 수 없다. 처음에는 훅이 세션마다 처음 한 번 묻고 승인 기록을 남기는 설계였으나,
승인된 세션을 허용하는 훅 분기를 Claude Code 자동 모드의 분류기가 우회로 판정해 막았다(2026년 10월 21일).
같은 이유로 `claude_rw`에 `db_owner`나 `db_ddladmin`을 주지 않는다. 두 역할에는 `DROP`과 `ALTER`가 함께 들어 있다.

Claude는 위 표의 Claude 연결로만 SQL을 실행한다. `.claude/hooks/guard_dangerous_cmd.py`의
`SQL_ALLOWED_CONNECTIONS`가 `execute_sql_query`의 `connectionId`를 허용 목록과 대조해 나머지를 막는다.
Windows 인증 연결은 관리자 권한이고 `permissions`는 툴의 인자를 보지 못하므로, 이 연결을 가르는 판정은 훅에만 있다.
같은 훅이 WHERE 없는 `UPDATE`와 `DELETE`를 연결과 무관하게 막는다. DB 권한으로는 이 둘을 막을 수 없다.
LocalDB는 Windows 인증이라 셸에서 붙으면 관리자가 되므로, 같은 훅이 `sqlcmd`·`osql`·`Invoke-Sqlcmd`·
`sqllocaldb`·`SqlClient` 직접 접속도 막는다(2026년 9월 30일 차단 확인).
**`claude_ro`나 `claude_rw` 데이터 소스를 다시 만들면 ID가 바뀌어 Claude의 SQL이 전부 막힌다.** 그때는 새 ID를
허용 목록에 넣는다. 파이썬 등 임의 코드로 여는 ODBC 연결은 문자열 검사로 완전히 막을 수 없다.

두 서버가 공유하는 인증 티켓 저장소로 Redis(`127.0.0.1:6379`).

---

## Rider MCP 상세

규칙은 `CLAUDE.md` 「빌드와 도구」에 있다. 여기엔 근거와 예외를 적는다.

### 빌드 명령

**빌드는 터미널에서 돌리고 종료 코드로 판정한다. Rider MCP로 빌드하지 않는다.** 통일한 이유는 `docs/adr/0001-unify-build-path.md`에 있다.

**클라이언트.** `P1/Scripts/Invoke-UeBuild.ps1`을 쓴다.

```powershell
pwsh P1/Scripts/Invoke-UeBuild.ps1 -CloseEditor
```

- **에디터가 떠 있으면 `UnrealEditor-P1.dll`을 덮어쓸 수 없어 실패한다.** 이 스크립트가 빌드 전에 확인한다.
- **`-CloseEditor` 없이 부르면 에디터를 닫지 않고 `1`로 끝난다.** 강제 종료가 저장하지 않은 에셋 변경을 없애기 때문이다. 닫아도 되는 상태인지 사람이 판단한 뒤에 스위치를 붙인다.
- `-Relaunch`를 함께 주면 빌드가 끝난 뒤 에디터를 다시 띄운다.
- 실패하면 빌드 출력과 `%LOCALAPPDATA%\UnrealBuildTool\Log.txt`에서 오류 줄만 추려서 보여준다. 전체 출력은 `P1/Saved/UeBuild/build.log`에 있다.

스크립트가 부르는 명령은 아래와 같다. 엔진 경로는 `UE_ENGINE_ROOT`로 덮을 수 있다.

```powershell
& "D:\Unreal\Editor\Launcher\UE_5.8\Engine\Build\BatchFiles\Build.bat" `
    P1Editor Win64 Development `
    -Project="D:\Unreal\Projects\Project_MMORPG\P1\P1.uproject" -WaitMutex
```

- 엔진 설치 경로는 머신마다 다르다. 위 경로는 이 머신의 런처 설치본이다.
- 타깃은 `P1Editor`, 플랫폼은 `Win64`, 구성은 `Development`다. 셋 중 하나라도 틀리면 엉뚱한 타깃을 빌드하고도 종료 코드 `0`이 나온다.

**서버.** 구성은 `Debug|x64`이고 산출물은 `Server/Binary/Debug/`에 떨어진다.

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" `
    "D:\Unreal\Projects\Project_MMORPG\Server\Server.sln" `
    /p:Configuration=Debug /p:Platform=x64 /m /nologo /v:minimal
```

- MSBuild 경로는 `vswhere.exe -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe"`로 찾는다. 머신마다 다르다.
- **`/v:minimal`을 빼지 않는다.** 기본 상세도는 출력이 훨씬 커진다.
- 이 빌드는 `Server/Libraries/Libs/ServerCore/`에 `ServerCore.lib`를 만든다. 이 폴더는 git이 추적하지 않으므로, 새로 클론한 뒤에는 솔루션을 한 번 빌드해야 게임 서버가 링크된다. GameServer·GameServerTests·DummyClient가 ServerCore를 `ProjectReference`로 참조해서 솔루션 빌드는 ServerCore를 먼저 빌드한다.

**출력 규모** (2026년 9월 16일 증분 빌드 실측)

| 대상 | 성공 종료 코드 | 실패 종료 코드 | 성공 출력 | 실패 출력 |
|---|---|---|---|---|
| 클라이언트 | `0` | `6` | 30줄 | 33줄, 2,355바이트 |
| 서버 | `0` | `1` | 117줄, 4,962바이트 | 20줄, 1,907바이트 |

`Rebuild.bat`과 최초 전체 빌드는 모듈 수만큼 출력이 늘어나므로 절단될 수 있고, 그때는 `%LOCALAPPDATA%\UnrealBuildTool\Log.txt`를 읽는다.

Rider MCP 툴의 사용법과 함정은 `.claude/skills/rider-mcp/SKILL.md`에 있다.