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

게임 서버를 빌드하려면 `Server/GameServer/config.h`도 직접 만든다. 아래 「게임 서버」에 적혀 있다.

---

## 게임 서버

`Server/Server.sln` (x64)에는 C++ 4개(`ServerCore` · `GameServer` · `DummyClient` · `GameServerTests`)와 파이썬 생성기 2개(`*.pyproj`)만 들어 있다. C++ 4개는 Debug와 Release 어느 구성에서도 빌드 대상이다. 파이썬 두 개는 `Build.0` 항목이 없어 빌드되지 않는다.

**AuthServer는 솔루션에 없다.** Node 프로젝트라 MSBuild가 `.NETCoreApp,v6.0` 참조를 요구하며 실패해서, 솔루션 빌드 신호를 상시 빨강으로 만들었다. `npm start`로만 다룬다.

`Server.slnLaunch.user`에 GameServer + DummyClient를 동시에 띄우는 다중 시작 프로필이 정의되어 있다.

**빌드 순서가 중요하다.** `ServerCore`는 정적 라이브러리이고 `GameServer`와 `DummyClient`가 이를 링크한다.

`GameServerTests`는 `GameServer`를 링크하지 않는다. `GameServer`가 exe라 링크할 수 없으므로, `Main/GameServer.cpp`를 제외한 GameServer의 `.cpp`를 직접 컴파일한다. **이 프로젝트는 `.cpp`를 자동으로 모으지 않는다.** 테스트 파일을 추가하고 `.vcxproj`에 등록하지 않으면 그 테스트는 조용히 돌지 않는다. 자세한 것은 [테스트 계층](./testing.md)에 있다.

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

`eslint`가 devDependency로 설치돼 있지만 flat config도 lint 스크립트도 없어서 실제로 연결돼 있진 않다.

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

**`P1/Source/`에는 모듈이 두 개다.** 게임 모듈 `P1/` 외에 protobuf를 벤더링한 `ProtobufCore/`가 있고(`ProtobufCore.Build.cs`, `Include/google/**`, `Lib/Win64/libprotobuf.lib` 16MB), `P1.Build.cs`가 이를 `PrivateDependencyModuleNames`로 링크한다. `.lib`은 `P1/.gitignore`의 `*.lib`에 걸리지만 클론 즉시 빌드되도록 강제 추적 중이다 — 실수가 아니다.

**클라이언트 include는 경로를 한정한다.** `P1/Source/P1/P1.Build.cs`의 `PrivateIncludePaths`에는 모듈 루트 `P1/`과 생성물 폴더 `P1/Network` 둘만 있다. 헤더는 파일명이 아니라 모듈 루트 기준 상대 경로로 include한다(`#include "Characters/Creature.h"`). **새 도메인 폴더를 만들어도 여기에 등록하지 않는다.**

생성물 폴더만 예외로 남겼다. protobuf 생성 코드가 서로를 `#include "Enum.pb.h"` 형태로 부르는데 생성물은 손으로 고치지 않으므로, 이 폴더를 빼면 생성기를 다시 돌릴 때마다 빌드가 깨진다. 이 결정의 근거는 `docs/adr/0005-drop-include-path-flattening.md`에 있고, 얻는 것과 얻지 못하는 것은 `docs/ARCHITECTURE.md`에 불변식으로 적혀 있다.

---

## 데이터베이스

스키마는 SQL 스크립트로 수동 적용한다.

| DB | 인스턴스 | 스크립트 |
|---|---|---|
| UserDB | `(localdb)\MSSQLLocalDB` | `Server/AuthServer/Queries/UserDB_CreateUsersTable.sql` |
| GameDB | `(localdb)\ProjectModels` | `Server/GameServer/Queries/GameDB_CreateAllTables.sql` |

GameDB 폴더에 `AlterTable.sql`, `GameDB_InsertAdminAccount.sql`, `GameDB_GetMaxItemUid.sql`도 있다.

**인스턴스가 서로 다르다.** 접속 문자열의 출처도 다르다 — GameDB는 `config.h`(컴파일 타임 상수), UserDB는 `.env`.

Rider의 DB 연결은 읽기 전용 계정(`claude_ro`)을 사용한다.

두 서버가 공유하는 액세스 토큰 저장소로 Redis(`127.0.0.1:6379`).

---

## Rider MCP 상세

규칙은 `CLAUDE.md` 「도구 라우팅」에 있다. 여기엔 근거와 예외를 적는다.

### 빌드 명령

**빌드는 터미널에서 돌리고 종료 코드로 판정한다. Rider MCP로 빌드하지 않는다.** 통일한 이유는 `docs/adr/0001-unify-build-path.md`에 있다.

**클라이언트.** 에디터를 닫고 돌린다.

```powershell
& "D:\Unreal\Editor\Launcher\UE_5.8\Engine\Build\BatchFiles\Build.bat" `
    P1Editor Win64 Development `
    -Project="D:\Unreal\Projects\Project_MMORPG\P1\P1.uproject" -WaitMutex
```

- 엔진 설치 경로는 머신마다 다르다. 위 경로는 이 머신의 런처 설치본이다.
- 타깃은 `P1Editor`, 플랫폼은 `Win64`, 구성은 `Development`다. 셋 중 하나라도 틀리면 엉뚱한 타깃을 빌드하고도 종료 코드 `0`이 나온다.
- 에디터가 떠 있으면 `UnrealEditor-P1.dll`을 덮어쓸 수 없어 실패한다.

**서버.** 구성은 `Debug|x64`이고 산출물은 `Server/Binary/Debug/`에 떨어진다.

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" `
    "D:\Unreal\Projects\Project_MMORPG\Server\Server.sln" `
    /p:Configuration=Debug /p:Platform=x64 /m /nologo /v:minimal
```

- MSBuild 경로는 `vswhere.exe -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe"`로 찾는다. 머신마다 다르다.
- **`/v:minimal`을 빼지 않는다.** 기본 상세도는 출력이 훨씬 커진다.
- 이 빌드는 추적 중인 `Server/Libraries/Libs/`의 `.lib`와 `.pdb`를 갱신한다. 커밋 전에 의도한 변경인지 확인한다.

**출력 규모** (2026년 9월 16일 증분 빌드 실측)

| 대상 | 성공 종료 코드 | 실패 종료 코드 | 성공 출력 | 실패 출력 |
|---|---|---|---|---|
| 클라이언트 | `0` | `6` | 30줄 | 33줄, 2,355바이트 |
| 서버 | `0` | `1` | 117줄, 4,962바이트 | 20줄, 1,907바이트 |

`Rebuild.bat`과 최초 전체 빌드는 모듈 수만큼 출력이 늘어나므로 절단될 수 있고, 그때는 `%LOCALAPPDATA%\UnrealBuildTool\Log.txt`를 읽는다.

### 노출 ≠ 존재, 노출 ≠ 동작

Rider 공식 문서와 IDE의 `Settings > Tools > MCP Server > Exposed Tools`에는 `build_project`도 있다. 다만 이 세션이 붙는 엔드포인트는 그걸 내놓지 않는다(2026년 9월 16일 재확인).

**판단 기준은 문서가 아니라 세션에 실제로 노출된 툴 목록이다.** 문서에 있다는 이유로 없는 툴을 부르지 말 것. 반대로 노출 목록에 없다고 해서 "그런 툴은 없다"고 단정하지도 말 것. 둘은 다른 얘기다.

**IDE 화면의 체크 상태도 근거가 아니다.** IDE의 노출 툴 설정은 이 엔드포인트에 반영되지 않는다. 21종을 끄고 Rider를 완전 재시작해도 `tools/list`가 101종을 그대로 내놨다(2026년 9월 16일 실측). 실측 전문과 처분은 `docs/adr/0002-control-mcp-tools-via-permissions.md`에 있다.

**노출되어 있고 에러도 내지 않는데 결과가 틀린 툴이 있다.** 이쪽이 더 위험하다. 거부당하면 알아채지만, 조용히 빈 결과를 돌려주면 "문제가 없다"로 읽히기 때문이다. 그런 툴의 목록과 증상은 ADR-0002에 있고, 대체 경로가 없는 것을 빼고는 `permissions.deny`로 막아 두었다.

**빈 결과를 근거로 삼기 전에 반증을 한 번 만들어 본다.** 일부러 틀린 입력을 넣어 그 툴이 실제로 잡아내는지 보는 것이 가장 싸다.

### rootFolder

Rider 인스턴스 하나가 열린 솔루션 전부를 한 엔드포인트로 서빙하고, 대상을 고르는 건 `rootFolder`뿐이다. 파라미터 이름이 `projectPath`가 아니다.

**함정**: 솔루션이 하나만 열려 있으면 서버가 모호성을 못 느껴 거부하지 않고 그대로 실행한다. Server를 빌드하려는데 P1만 열려 있으면 P1이 빌드된다.

빌드·실행·리팩토링 전에 의도한 솔루션이 실제로 열려 있는지 확인한다. 인자 없이 `get_run_configurations`를 부르면 열린 프로젝트 목록이 에러 메시지로 돌아온다.

상태를 바꾸는 Rider 툴은 `rootFolder`가 없으면 `.claude/hooks/guard_dangerous_cmd.py`가 차단한다.