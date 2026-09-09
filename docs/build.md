# 빌드 및 실행

`CLAUDE.md`에서 분리한 문서. 빌드 구성을 건드릴 때만 읽는다.

---

## 전체 구성

전체를 한 번에 띄우는 스크립트는 없다. 티어별로 따로 빌드하고 실행한다.

| 티어 | 솔루션 / 명령 | 산출물 |
|---|---|---|
| 게임 서버 | `Server/Server.sln` (x64) | `Server/Binary/{Debug,Release}/` |
| 인증 서버 | `npm start` | — |
| 클라이언트 | `P1/P1.uproject` 또는 `P1/P1.sln` | UE 기본 경로 |

---

## 게임 서버

`Server/Server.sln` (x64)에는 C++ 3개(`ServerCore` · `GameServer` · `DummyClient`)와 파이썬 생성기 2개(`*.pyproj`)만 들어 있다.

**AuthServer는 솔루션에 없다.** Node 프로젝트라 MSBuild가 `.NETCoreApp,v6.0` 참조를 요구하며 실패해서, 솔루션 빌드 신호를 상시 빨강으로 만들었다. `npm start`로만 다룬다.

`Server.slnLaunch.user`에 GameServer + DummyClient를 동시에 띄우는 다중 시작 프로필이 정의되어 있다.

**빌드 순서가 중요하다.** `ServerCore`는 정적 라이브러리이고 `GameServer`와 `DummyClient`가 이를 링크한다.

접속 문자열은 환경변수가 아니라 `Server/GameServer/config.h`에 컴파일 타임 상수로 박혀 있다. **gitignore됨 — 새로 클론하면 직접 만들어야 한다.**

---

## 인증 서버

```bash
cd Server/AuthServer
npm install
npm start                # = node src/app.js
```

설정은 gitignore된 `.env`를 `src/Config/configs.js`가 읽는다.

| 키 |
|---|
| `PORT` |
| `DB_CONNECTION_STRING` |
| `DB_DRIVER` |
| 커넥션 풀 크기 |
| `REDIS_HOST` |
| `REDIS_PORT` |
| `ACCESS_TOKEN_TTL` |

`eslint`가 devDependency로 설치돼 있지만 flat config도 lint 스크립트도 없어서 실제로 연결돼 있진 않다. (D-18)

---

## 클라이언트

`P1/P1.uproject`를 열거나(Unreal Engine 5.8), C++ 작업은 `P1/P1.sln`로 연다. 소스 파일을 추가한 뒤에는 `.uproject` 우클릭 메뉴로 프로젝트 파일을 재생성한다.

### 엔진 제약 — 무언가를 계획하기 전에 이것부터 본다

> **엔진은 런처 설치본(Installed Build)만 쓴다. 소스 빌드는 이 프로젝트의 선택지가 아니다.**
> 확정된 제약이며 재검토하지 않는다.

**불가능한 것 두 가지**

- **프로젝트 안의 `TargetType.Program` 타깃** — UBT가 이런 타깃을 무조건 고유 빌드 환경으로 잡고 설치본이 이를 거부한다. UE Low-Level Tests(Catch2)가 여기 걸려 채택하지 않았다 (ADR-0002 결정 6). 스탠드얼론 툴이 필요하면 `Server/DummyClient`처럼 UE 밖에서 만든다.
- **엔진 소스 패치** — 엔진 버그를 만나면 프로젝트 코드 안에서 우회하는 수밖에 없다.

**영향이 없는 것**: 게임 빌드·실행·패키징, 프로젝트 플러그인(플러그인 모듈은 프로젝트 쪽에서 컴파일된다), 엔진 코드 디버깅(설치본도 `Engine/Source`를 딸려 준다), 서버·인증 티어 전부.

---

## 클라이언트 모듈과 include

**`P1/Source/`에는 모듈이 두 개다.** 게임 모듈 `P1/` 외에 protobuf를 벤더링한 `ProtobufCore/`가 있고(`ProtobufCore.Build.cs`, `Include/google/**`, `Lib/Win64/libprotobuf.lib` 16MB), `P1.Build.cs`가 이를 `PrivateDependencyModuleNames`로 링크한다. `.lib`은 `P1/.gitignore`의 `*.lib`에 걸리지만 클론 즉시 빌드되도록 강제 추적 중이다 — 실수가 아니다.

**클라이언트 include는 평탄하다.** `P1/Source/P1/P1.Build.cs`가 게임플레이 하위 디렉터리를 전부 `PrivateIncludePaths`에 등록해 두어서, 헤더는 상대 경로가 아니라 파일명만으로 include한다(`#include "Creature.h"`). **새 하위 디렉터리를 만들면 여기에도 등록해야 한다.**

이 평탄화의 대가는 `docs/ARCHITECTURE.md`에 불변식으로 적혀 있다 — 폴더를 옮겨도 빌드가 깨지지 않는 대신 경계 위반도 빌드가 잡아주지 않는다.

---

## 데이터베이스

스키마는 SQL 스크립트로 수동 적용한다.

| DB | 인스턴스 | 스크립트 |
|---|---|---|
| UserDB | `(localdb)\MSSQLLocalDB` | `Server/Queries/UserDB_CreateUsersTable.sql` |
| GameDB | `(localdb)\ProjectModels` | `Server/GameServer/Queries/GameDB_CreateAllTables.sql` |

GameDB 폴더에 `AlterTable.sql`, `GameDB_InsertAdminAccount.sql`, `GameDB_GetMaxItemUid.sql`도 있다.

**인스턴스가 서로 다르다.** 접속 문자열의 출처도 다르다 — GameDB는 `config.h`(컴파일 타임 상수), UserDB는 `.env`.

Rider의 DB 연결은 읽기 전용 계정(`claude_ro`)을 사용한다.

두 서버가 공유하는 액세스 토큰 저장소로 Redis(`127.0.0.1:6379`).

---

## Rider MCP 상세

규칙은 `CLAUDE.md` 「도구 라우팅」에 있다. 여기엔 근거와 예외를 적는다.

### 빌드를 터미널로 돌리지 않는 이유

출력 절단으로 에러가 유실된다. `build_solution_state`가 진단 없이 실패만 돌려주면 Rider 빌드 로그를 직접 읽는다.

```
%LOCALAPPDATA%/JetBrains/Rider<버전>/log/SolutionBuilder/
```

### 노출 ≠ 존재

Rider 공식 문서와 IDE의 `Settings > Tools > MCP Server > Exposed Tools`에는 `build_project`도 있다. 다만 이 세션이 붙는 엔드포인트는 그걸 내놓지 않는다(실측).

**판단 기준은 문서가 아니라 세션에 실제로 노출된 툴 목록이다.** 문서에 있다는 이유로 `build_solution_start`를 `build_project`로 되돌리지 말 것. 반대로 노출 목록에 없다고 해서 "그런 툴은 없다"고 단정하지도 말 것. 둘은 다른 얘기다.

### rootFolder

Rider 인스턴스 하나가 열린 솔루션 전부를 한 엔드포인트로 서빙하고, 대상을 고르는 건 `rootFolder`뿐이다. 파라미터 이름이 `projectPath`가 아니다.

**함정**: 솔루션이 하나만 열려 있으면 서버가 모호성을 못 느껴 거부하지 않고 그대로 실행한다. Server를 빌드하려는데 P1만 열려 있으면 P1이 빌드된다.

빌드·실행·리팩토링 전에 의도한 솔루션이 실제로 열려 있는지 확인한다. 인자 없이 `get_run_configurations`를 부르면 열린 프로젝트 목록이 에러 메시지로 돌아온다.

상태를 바꾸는 Rider 툴은 `rootFolder`가 없으면 `.claude/hooks/guard_dangerous_cmd.py`가 차단한다.