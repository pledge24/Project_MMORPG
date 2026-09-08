---
status: accepted
date: 2026-08-27
scope: [build, server]
supersedes: null
superseded-by: null
---

> 2026-09-09 MADR 전환 시 프론트매터와 Confirmation 절만 추가. 본문 미변경.

# ADR: L1 테스트 인프라 — GoogleTest 도입 방식과 실행 경로

- 상태: 확정
- 날짜: 2026-08-27 (세션 2)
- 관련: `docs/plans/completed/verification-infra.md` · `docs/decisions/2026-08-19-final-harness-verdict.md` §9 ·
  `docs/tech-debt.md` D-01/D-12/D-14/D-15/D-22/D-23

## 컨텍스트

자동 테스트가 0개였다. 그래서 세 가지가 동시에 막혀 있었다.

1. **완료 기준이 "빌드 통과"에서 멈춘다.** CLAUDE.md의 "자기신고를 믿지 않는다"는 검증 커맨드를
   요구하는데 서버에는 실행할 커맨드가 없었다.
2. 판정 ADR §9의 배치 실행 모드 **해동 조건 중 하나가 "AC 사다리 3단을 채울 테스트 인프라 존재"** 였다.
3. D-03(갓 클래스) 같은 리팩토링에 회귀 그물이 없었다.

---

## 결정 1 — GoogleTest는 소스 벤더링 (vcpkg 아님)

`Server/Libraries/googletest/`에 googletest **v1.18.0**(커밋 `063de7e`) 소스를 넣고,
`gtest-all.cc` + `gtest_main.cc`를 테스트 프로젝트가 **직접 컴파일**한다. gmock은 넣지 않았다.

세션 1은 vcpkg manifest를 권했다. 공식 문서로 재확인한 결과가 달라 **변경했다**(사용자 승인).

| 확인 사항 | 출처 | 결과 |
|---|---|---|
| vcpkg 존재 | 실측 (`VCPKG_ROOT` 없음, PATH 없음) | **없음.** VS2022 번들만 있고 프로젝트 툴셋은 VS18의 v145 |
| gtest의 MSBuild 자동 링크 | MS vcpkg 문서 | **불가.** gtest는 `main()` 재정의로 "수동 링크 필요" 목록에 명시 |
| manifest 기본 활성 | MS vcpkg 문서 | `VcpkgEnableManifest` 기본값 **false** |
| triplet 자동 추론 | MS vcpkg 문서 | 동적 CRT/동적 링크만. 정적 lib은 수동 지정 |
| v143 산출물 ↔ v145 링커 | MS 바이너리 호환 문서 | **합법**(링커가 입력보다 최신이면 OK) — vcpkg가 불가능한 게 아니라 비용이 큰 것 |
| MS NuGet gtest | nuget.org | GoogleTest **1.8.1(2018)** 고정 |
| 비-CMake 빌드 경로 | googletest 저장소 | `src/gtest-all.cc`(단일 TU) 현존 |
| 리포 관행 | 실측 | 서드파티 전부 이미 벤더링 중(`Libraries/include/{google,nlohmann,sw,hiredis}`) |

**근거.** 벤더링은 툴셋·CRT 완전 일치(v145로 같이 컴파일), 머신 선행조건 0, 네트워크 0,
의존성 획득 메커니즘 1개 유지를 동시에 만족한다. 대가는 리포 1.1MB다.
결정적이었던 것은 마지막 줄 — 이 리포는 **이미** 모든 서드파티를 벤더링한다. gtest만 다른
메커니즘을 들이면 새 클론에 "vcpkg를 설치하라"는 단계가 하나 늘고, 그 단계는 문서에만 존재하게 된다.

**기각안**

- *vcpkg manifest* — 위 표. 특히 gtest는 vcpkg가 자동 링크해 주지 못하는 예외 라이브러리라
  `.vcxproj`에 수동 설정이 세 군데 붙는다. 자동화의 이점이 그만큼 줄어든다.
- *MS NuGet(`Microsoft.googletest.v140.*`)* — GoogleTest 1.8.1은 C++20 코드베이스에 8년 낡았다.
  더해 이 솔루션은 NuGet restore 때문에 빌드가 상시 실패한 전력이 있다(D-13).
- *Catch2로 서버·UE 통일* — UE LLT가 Catch2라 어휘가 통일되는 이점은 실재한다. 다만 CLAUDE.md가
  서버=GoogleTest로 이미 확정했고, 결정 3의 결과로 UE LLT 자체가 보류됐으므로 통일할 대상이 없다.

**재검토 조건.** 서버에 벤더링 대상 의존성이 3개 이상 더 늘거나, gtest 버전 갱신이 실제로
번거롭게 느껴지는 시점. 그때 vcpkg를 다시 본다.

### Confirmation

* `Server/Libraries/googletest/`에 v1.18.0 소스가 있다 — `src/gtest-all.cc`와 `src/gtest_main.cc`가
  존재하고, `README.ko.md`가 버전 v1.18.0 / 커밋 `063de7e`를 명시한다.
* **gmock은 없다.** `Server/Libraries/googletest/include/gmock`이 존재하면 이 결정이 바뀐 것이다.
* 리포에 vcpkg 매니페스트가 없다: `find . -name vcpkg.json` 결과 0건.
* 서드파티 획득 메커니즘은 벤더링 하나다. `Server/Libraries/` 밖에서 서버 의존성을 끌어오는
  단계(패키지 매니저 restore, 머신 설치 선행조건)가 새로 생기면 위반이다.


---

## 결정 2 — 테스트 프로젝트가 GameServer의 `.cpp`를 직접 포함한다

`GameServer`는 exe라 링크할 수 없다. `Server/GameServerTests/GameServerTests.vcxproj`(콘솔 exe)가
**`Main/GameServer.cpp`를 제외한 GameServer `.cpp` 전부**를 `ClCompile`로 포함한다.
UE Low-Level Tests와 같은 패턴이다.

**왜 "필요한 것만"이 아니라 "main 빼고 전부"인가**

- `Inventory.cpp` → `Player.cpp` → `Room.cpp` → `DBRequestFunctions.cpp`로 전이 의존이 이어져 결국
  대부분을 넣게 된다. 링크 에러가 날 때마다 파일을 추가하는 루프는 비결정적이고 재현이 안 된다.
- `Main/GameServer.cpp`는 **gitignore된 `config.h`의 유일한 소비자**다(실측). 이것만 빼면
  테스트 타깃은 **비밀 없이 빌드된다** — D-12(CI)에서 그대로 쓰인다.

**부수 결정**

| 항목 | 선택 | 이유 |
|---|---|---|
| PCH | GameServer의 `Main/pch.cpp`를 `Create`로 포함 | 대상 코드와 같은 전제에서 컴파일해야 한다 |
| gtest 2파일 | `PrecompiledHeader=NotUsing` | 서버 pch가 `using namespace std;`를 전역에 푼다 |
| `main()` | `gtest_main.cc` 대신 `TestMain.cpp` | `SetConsoleOutputCP(CP_UTF8)` 필요 — 아래 참조 |
| `GenerateProtoPackets` 타깃 | **복제하지 않음** | 같은 파일에 XCOPY하는 타깃이 둘이면 병렬 빌드에서 경합한다. 재생성은 GameServer의 책임 |
| `ProjectReference` | ServerCore만(`LinkLibraryDependencies=false`) | 빌드 순서용. GameServer를 참조하면 `config.h` 의존이 되살아난다 |
| Release `WholeProgramOptimization` | `false` | `/GL` 산출물은 툴셋 마이너 버전 간에도 비호환(MS 문서) |

**`TestMain.cpp`를 따로 둔 이유.** gtest는 진단 메시지를 UTF-8 바이트로 stdout에 쓰는데 이 환경의
기본 콘솔 코드페이지는 cp949다. 한국어 실패 메시지가 `湲고? ?щ’ ??젣媛` 로 깨져 읽히지 않았다.
이 프로젝트는 주석과 로그를 한국어로 쓰기로 했으므로, 읽히지 않는 실패 진단은 테스트 하네스로서
제 역할을 못 한다. `main` 하나를 직접 두는 값싼 해결이다.

**정적 초기화 안전성(실측).** `CoreGlobal.cpp`의 `GCoreGlobal`은 ThreadManager/GlobalQueue/JobTimer/
DBConnectionPool/DBManager/RedisManager를 `new`만 하고 `SocketUtil::Init()`(WSAStartup)을 부른다.
`Global.cpp`의 `GGameServerGlobal`은 `new RoomManager()`뿐이고 그 생성자는 비어 있다.
**DB·Redis 접속은 일어나지 않는다** — 테스트 exe에서 안전하다.

### Confirmation

* `Server/GameServerTests/GameServerTests.vcxproj`의 `ClCompile` 항목에
  `..\GameServer\Main\GameServer.cpp`가 **없다** (주석에는 언급이 있으므로 `ClCompile` 라인으로
  한정해 확인한다).
* `config.h`의 소비자는 한 곳뿐이다:
  `grep -rn "config\.h" Server/GameServer Server/GameServerTests` 결과가
  `Server/GameServer/Main/GameServer.cpp` 한 줄. 두 줄 이상이면 테스트 타깃이 "비밀 없이 빌드된다"는
  성질을 잃고, D-12(CI)의 전제도 함께 무너진다.
* `ProjectReference`는 `ServerCore.vcxproj` 하나이며 `LinkLibraryDependencies=false`다.
  GameServer 참조가 추가되면 `config.h` 의존이 되살아난다.
* GameServer에 `.cpp`를 추가하면 테스트 vcxproj의 `<ItemGroup Label="GameServer 본체 ...">`에도
  등록한다 (이 프로젝트는 파일 자동 수집을 하지 않는다). 빠뜨리면 링크 에러로 드러난다.
* `GenerateProtoPackets` 타깃을 테스트 vcxproj에 복제하지 않는다 — 같은 파일에 XCOPY하는 타깃이
  둘이면 병렬 빌드에서 경합한다.


---

## 결정 3 — 실행 경로를 둘로 나눈다 (에이전트=셸, 사람=Rider)

| 용도 | 경로 |
|---|---|
| 빌드 (양쪽 공통) | `mcp__rider__build_solution_start` → `build_solution_state` |
| 에이전트·CI 실행 | `Server/Binary/Debug/GameServerTests.exe` 를 셸에서 직접 |
| 사람·IDE 실행 | Rider 실행 구성 `GameServerTests` (솔루션 등록 시 자동 생성) |

**왜 나눴나.** `execute_run_configuration`은 호출할 때마다 **Rider가 자체 확인 대화상자를 띄운다.**
TDD는 red→green으로 매 수정마다 실행하므로 루프가 사람 클릭에 묶인다. 대화상자의 체크박스는
**Brave 모드**인데, 이건 켜지 않기로 한 항목이다.

공식 문서 확인 결과 **Rider 쪽에 더 좁은 대안이 없다.** 확인을 끄는 설정은
`Settings | Tools | MCP Server | Command execution`의
"Run shell commands or run configurations without confirmation (brave mode)" 하나뿐이고,
셸 명령과 실행 구성을 함께 덮으며 IDE 전역·모든 MCP 클라이언트에 적용된다.
per-command allowlist는 없다. 그래서 해법이 실행 경로 쪽에 있었다.

**안전장치 회계 — 솔직하게**

| 장치 | 변경 후 | 근거 |
|---|---|---|
| `guard_dangerous_cmd.py` 위험 패턴 검사 | **유지** | `SHELL_TOOLS`에 `Bash`·`PowerShell`이 이미 포함돼 있다 |
| `rootFolder` 누락 검사 | 해당 없음 | 절대 경로 exe에는 대상 솔루션 모호성이 애초에 없다 |
| Rider 확인 대화상자 | 사라짐 | 이 경로가 Rider를 거치지 않으므로 |
| Brave 모드 | **꺼진 채 유지** | |

사람 클릭 게이트 한 겹이 없어지는 것은 맞다. 대신 남는 게이트(훅)는 확률적이 아니라 결정적이고,
D-12(CI)에서 그대로 재사용된다. Brave 모드는 그 반대 방향 — 게이트를 IDE 전역에서 없앤다.

**빌드는 왜 셸로 옮기지 않았나.** CLAUDE.md의 "UBT/MSBuild를 터미널로 직접 돌리지 않는다"는
출력 절단으로 에러가 유실되기 때문이다. 그 근거는 빌드에만 성립한다 — gtest 실행 출력은 짧고
완결적이며 판정은 종료 코드 하나다. 규칙의 문구가 아니라 근거를 기준으로 갈랐다.

### Confirmation

* **판정은 종료 코드다.** `Server/Binary/Debug/GameServerTests.exe`를 셸에서 직접 실행해
  **종료 코드 0**이면 통과. (`Server/Binary/`는 gitignore이므로 실행 전 빌드가 선행돼야 한다.)
* 빌드는 `mcp__rider__build_solution_start` → `build_solution_state`로만 한다. 세션 기록에
  UBT·`Build.bat`·MSBuild 직접 호출이 있으면 위반이다.
* `mcp__rider__execute_run_configuration`을 호출하지 않는다. 이 툴은 `.claude/settings.json`의
  PreToolUse matcher에는 들어 있지만 `permissions.allow`에는 없다 — allow 목록에 이 이름이
  추가되면 결정을 벗어난 것이다 (현재 allow는 빌드 2종뿐).
* Brave 모드는 리포 밖 IDE 설정이라 파일로 확인할 수 없다. 사람이
  `Settings | Tools | MCP Server | Command execution`의
  "Run shell commands or run configurations without confirmation (brave mode)"가 꺼져 있는지 본다.


---

## 결정 4 — 회귀 그물은 목록이 아니라 리플렉션으로

`.proto` 3개는 원본이 하나인데 생성물이 5개 트리로 XCOPY된다. "생성기를 다시 돌렸더니 무언가
조용히 어긋났다"가 이 프로젝트 고유의 위험이라, 40개 메시지 전체를 덮는 그물이 필요했다.

**수작업 40개 대신** `Server/GameServerTests/ProtocolContractTests.cpp` 하나에 셋을 담았다.

1. `MessageSetMatchesDeclaredList` — 메시지 이름·선언 순서를 계약 목록과 대조
2. `PacketIdsAreContiguousAndUnique` — `PKT_*`가 1000부터 선언 순서대로, 겹침 없이
3. `EveryMessageRoundTrips` — `DescriptorPool`로 전 메시지를 순회하며 리플렉션으로 모든 필드를
   채운 뒤 왕복. **메시지가 늘어도 이 테스트는 고치지 않는다**

1과 2는 **하나의 `PROTOCOL_MESSAGES(X)` 매크로 목록**이 함께 구동한다. 이름 목록이 중복이 아니라
계약 선언인 이유가 여기 있다 — 실패는 ".proto를 바꿨으니 목록도 의식적으로 갱신하라"는 신호다.
40개 왕복 테스트를 손으로 쓰는 쪽은 같은 값을 40번 베끼는 것이라 반드시 부패한다.

### Confirmation

* `.proto`에 메시지를 추가·삭제했을 때 고치는 곳은 `Server/GameServerTests/ProtocolContractTests.cpp`의
  `PROTOCOL_MESSAGES(X)` 목록 **한 곳뿐**이다. 목록 갱신은 의도된 신호라 정상이고,
  같은 diff에 `EveryMessageRoundTrips` 수정이 함께 들어 있으면 그물이 목록으로 퇴화한 것이다.
* 이름·ID 두 검사가 같은 목록에서 유도되는지: 파일 안에 `PROTOCOL_MESSAGES(AS_STRING)`와
  `PROTOCOL_MESSAGES(AS_PACKET_ID)` 두 전개가 남아 있어야 한다. 어느 한쪽이 손으로 쓴 배열로
  바뀌면 두 표가 어긋날 수 있는 구조로 되돌아간다.
* `Server/Common/Protobuf/bin/GenPackets.bat` 재실행 후 `GameServerTests.exe` 종료 코드 0.
  이것이 "생성기를 다시 돌렸더니 조용히 어긋났다"를 잡는 지점이다.


---

## 결정 5 — 인증 서버는 Node 내장 러너

`node --test` + `node:assert/strict`. **새 의존성 0개**(Node v20.12.2에서 stable, ESM 지원,
`node_modules` 자동 제외, 실패 시 exit 1 — 공식 문서 확인). `package.json`이 이미
`"type": "module"`이라 Jest/Mocha를 들일 이유가 없다.

첫 테스트를 `configs.js`에 건 이유: `.env`는 gitignore돼 있어 새 클론에 없다. 키가 하나 빠져도
서버는 기동에 성공하고 DB·Redis 접속 시점에야 터진다. `parseInt(undefined)`는 예외가 아니라
NaN이라 커넥션 풀 설정이 조용히 무의미해진다. 그 사이의 시간이 이 티어에서 가장 비싼 디버깅이었다.

### Confirmation

* `Server/AuthServer/package.json`의 `scripts.test`가 `"node --test"`다.
* `devDependencies`에 테스트 러너(jest·mocha·vitest 등)가 없다 — 현재 `eslint` 하나뿐.
  러너가 추가되면 "새 의존성 0개"라는 이 결정의 근거가 사라진다.
* `cd Server/AuthServer && npm test` 종료 코드 0.
* 테스트 파일은 `*.test.js`로 대상 소스 옆에 둔다 (현재 `src/Config/configs.test.js`).


---

## 결정 6 — UE L1(Low-Level Tests)은 채택하지 않는다. UE의 경로는 L2다

- **재검토 조건: 없음.**

근거는 두 개이고, 순서가 중요하다.

1. **프로젝트 제약** — 이 프로젝트는 **런처 설치본 엔진만 쓴다. 소스 빌드는 선택지가 아니다.**
   사용자가 확정했다(2026-08-27). 이건 기술 판단이 아니라 프로젝트 결정이므로, 아래 2번이
   설령 뒤집히더라도 결론은 그대로다. CLAUDE.md 「빌드 및 실행」에 상시 제약으로 올렸다.
2. **기술적 불가** — 설치본에서 LLT 타깃은 빌드 자체가 거부되고 설정으로 우회할 수 없다(아래).

손실은 작다. L1으로 검증할 가치가 큰 순수 로직 — 전투 판정, 인벤토리, 레벨 테이블 — 은
아키텍처상 대부분 서버 소유이고(§ 아키텍처 규칙), 그 계층은 결정 1~2로 이미 gtest가 붙었다.
클라 쪽에 남는 L1 대상은 이동 보간 수식 정도로 얇다. L2/L3/L4는 설치본에서 그대로 간다.

### 왜 설치본에서 LLT가 안 되는가 (설정 문제가 아니다)

표면의 거부는 두 줄이다.

- `TargetRules.cs:2690-2693` — Program 타입 타깃이 `.uproject` 폴더 안에 있으면 **조건 없이** `Unique`
- `RulesAssembly.cs:677-680` — `Unique` + 설치본 엔진이면 예외를 던지고 중단

여기까지만 보면 "검사를 통과시키면 되지 않나"로 읽힌다. `TargetRules.cs:2678-2704`의 게터가
`BuildEnvironmentOverride`를 **가장 먼저** 확인하므로, 타깃에서 `BuildEnvironment = Shared`를
지정하면 실제로 저 분기를 건너뛴다. 커뮤니티에 도는 workaround도 그것이다.

**그런데 LLT에는 통하지 않는다.** `TestTargetRules.SetupCommonProperties`가 조건 없이 이렇게 잡는다:

```csharp
Type = TargetType.Program;   LinkType = TargetLinkType.Monolithic;
bCompileAgainstEngine = false;      bCompileAgainstEditor = false;
bBuildWithEditorOnlyData = false;   bCompileICU = false;   bBuildDeveloperTools = false;
bForceDisableAutomationTests = true;  bUseLoggingInShipping = true;  bForceEnableExceptions = true;
GlobalDefinitions.Add("STATS=0");  // 외 다수
```

이 목록이 곧 **"빌드 환경"의 정의**다. 설치본이 주는 프리빌트 바이너리는 정반대 설정으로
컴파일돼 있다. `Shared`로 강제한다는 것은 UBT에게 "그 바이너리를 재사용하라"고 시키는 것이라,
검사를 통과시켜도 정의가 어긋난 산출물이 나온다. 커뮤니티 workaround는 *굳이 `Unique`를 선언할
필요가 없던* 타깃(CookedEditor류)용이지, **환경이 다른 것 자체가 존재 이유인 LLT용이 아니다.**

즉 이건 플래그로 우회할 성질이 아니라 설치본의 정의상 한계다. 유일한 해제 수단은 소스 빌드
엔진인데, 그건 위 근거 1에 의해 이 프로젝트의 선택지가 아니다. **그래서 조건부 보류가 아니라 폐기다.**

### 채택: L2를 UE의 기본 경로로

`IMPLEMENT_SIMPLE_AUTOMATION_TEST`가 설치본
`Engine/Source/Runtime/Core/Public/Misc/AutomationTest.h:4297`에 있다(실측). 별도 타깃 없이
`P1` 모듈에 컴파일된다. P1은 아직 하나도 쓰지 않고 있다.

L4도 살아 있다 — 설치본에 `Engine/Plugins/Experimental/Gauntlet` 플러그인과 public
`GauntletTestController.h`, 컴파일된 `Gauntlet.Automation.dll`이 들어 있다. 플러그인 모듈은
프로젝트 쪽에서 컴파일되므로 소스 엔진이 필요 없다.

**단 위 둘 다 "존재 확인"까지이고 실행은 아직 안 해봤다.** 착수할 때 가장 싸게 실패하는 경로부터
돌린다 — 이 ADR의 아래 "판정 경위"가 그걸 안 해서 생긴 기록이다. CLAUDE.md 테스트 계층 절에
계층별 검증 상태를 갈라 적어 뒀다.

**대가**는 에디터 의존이다. L2 실행에는 에디터가 필요해(`Window > Test Automation` 또는
`-ExecCmds="Automation RunTests ..."`) 완전 무인 검증이 되지 않는다. CI(D-12)에 붙일 때 다시 본다.

### 작성했던 파일을 지운 이유

파이프라인 증명용으로 `P1Tests.Target.cs` / `P1Tests.Build.cs` / `SmokeTest.cpp`를 썼고
(엔진 예제 `Programs/LowLevelTests/{FoundationTests,MathCoreTests}` 구조를 따랐다),
한때 `docs/references/p1-lowlevel-tests/`에 "해동용"으로 보존했다. **지웠다.**

해동이 오지 않을 것이 확정된 이상 해동 절차는 살아 있는 부패다 — 다음 세션이 그걸 읽고
"소스 빌드를 하면 되겠네"로 가면 같은 조사를 반복하게 된다. 이 리포의 "기록 시스템이 둘이면
한쪽이 부패한다" 원칙대로, 기록은 이 ADR 하나로 모은다. 파일 자체는 git 이력에 남아 있다
(브랜치 `test-infra-l1`, 커밋 `4b34cae` 시점).

참고로 그 파일들을 `P1/Source/`에 두지 않았던 이유도 남겨 둔다: `ProjectFileGenerator.cs:3176`이
**프로젝트 파일 생성 시에도** 같은 검증을 수행하므로, 두면 P1의 프로젝트 파일 재생성이 깨진다.
앞으로 `P1/Source/`에 Program 타깃을 추가하려는 시도가 있으면 같은 함정을 밟는다.

### 판정 경위 — 이 결정에서 내가 틀렸던 것

계획 단계에서 설치본에 `ThirdParty/Catch2`, `Developer/LowLevelTestsRunner`,
`Programs/AutomationTool/LowLevelTests`가 있는 것을 확인하고 계획 파일에 **"LLT 가능"이라고 적었다.**
재료가 있는 것과 빌드가 허용되는 것은 다른 문제인데, 앞을 확인하고 뒤를 추론했다.

더 나쁜 부분은 그 다음이다. 같은 조사에서 `Engine/Build/LowLevelTests.xml`이 **없는 것**도
발견했고 "그럼 BuildGraph 대신 UBT 직접 빌드로 가면 된다"고 처리했다. **구멍 하나를 찾아 메웠다는
사실이 나머지에 대한 확신을 높였다.** 부분 확인이 전체 검증처럼 느껴지는 것 — 이게 이 실수의 형태다.

기존 메모리 `tool-exposure-is-not-existence`("노출 ≠ 존재")와 같은 계열의 두 번째 사례라,
CLAUDE.md 「완료 기준」에 **"존재 ≠ 가능"** 규칙으로 승격했다("두 번이면 문서, 세 번이면 규칙"
사다리 적용). 대가는 약 20분과 실패한 빌드 1회였고, 그 실패가 규칙 하나를 만들었다.

### Confirmation

* `P1/` 아래에 `TargetType.Program` 타깃이 없다: `grep -rn "TargetType.Program" P1/` 결과 0건.
* `P1/Source/`의 `*.Target.cs`는 `P1.Target.cs`·`P1Editor.Target.cs` 둘뿐이다.
  `P1Tests.Target.cs`가 다시 생기면 빌드 이전에 **프로젝트 파일 재생성**부터 깨진다
  (`ProjectFileGenerator.cs:3176`이 같은 검증을 수행한다).
* `P1/`의 `.Build.cs`·`.Target.cs` 어디에도 Catch2·`LowLevelTestsRunner` 참조가 없다.
* 새 클라 테스트는 L2로 쓴다 — `IMPLEMENT_SIMPLE_AUTOMATION_TEST`를 써서 별도 타깃 없이
  `P1` 모듈에 컴파일되는 형태인지 확인한다.
* **재검토 조건이 없는 결정이다.** 소스 빌드 엔진을 전제하는 제안(엔진 패치, 프로젝트 내 Program
  타깃)이 계획에 올라오면 그 자체가 위반이다.


---

## 결과

| 티어 | 상태 | 테스트 |
|---|---|---|
| 게임 서버 L1 | ✅ 동작 | 14 통과 + 1 `DISABLED_`(D-14) |
| 인증 서버 | ✅ 동작 | 2 통과 |
| UE 클라 L2 | 🔜 경로 확정, 미착수 | 매크로 **존재 확인**까지. 실행 미검증 |
| UE 클라 L1(LLT) | **채택 안 함** | 런처 엔진 고정이 프로젝트 제약. 재검토 조건 없음 |

이 인프라가 이 세션에 잡은 버그 **2건**: D-01(계획된 것), 그리고 `removeItem`의 슬롯 오염
(계획에 없던 것 — 코드를 눈으로 읽어서는 나오지 않았다). 상세는 tech-debt D-01·D-15.

판정 ADR §9의 해동 조건 "AC 사다리 3단을 채울 테스트 인프라 존재"가 **서버 한정으로 충족**됐다.
UE 클라 쪽은 아직이므로, 배치 모드 해동은 여전히 이르다.
