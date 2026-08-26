# ADR: L1 테스트 인프라 — GoogleTest 도입 방식과 실행 경로

- 상태: 확정
- 날짜: 2026-08-27 (세션 2)
- 관련: `docs/plans/completed/verification-infra.md` · `docs/decisions/2026-08-19-final-harness-verdict.md` §9 ·
  `docs/tech-debt.md` D-01/D-12/D-14/D-15/D-22/D-23 · `docs/references/p1-lowlevel-tests/README.md`

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

---

## 결정 5 — 인증 서버는 Node 내장 러너

`node --test` + `node:assert/strict`. **새 의존성 0개**(Node v20.12.2에서 stable, ESM 지원,
`node_modules` 자동 제외, 실패 시 exit 1 — 공식 문서 확인). `package.json`이 이미
`"type": "module"`이라 Jest/Mocha를 들일 이유가 없다.

첫 테스트를 `configs.js`에 건 이유: `.env`는 gitignore돼 있어 새 클론에 없다. 키가 하나 빠져도
서버는 기동에 성공하고 DB·Redis 접속 시점에야 터진다. `parseInt(undefined)`는 예외가 아니라
NaN이라 커넥션 풀 설정이 조용히 무의미해진다. 그 사이의 시간이 이 티어에서 가장 비싼 디버깅이었다.

---

## 결정 6 — UE Low-Level Tests는 보류 (환경 제약, 설계 실패 아님)

`[B] blocked` — 상세는 `docs/references/p1-lowlevel-tests/README.md`.

작성한 모듈·타깃은 엔진 예제 구조를 그대로 따랐고 파일 자체에는 문제가 없다. 막은 것은
**엔진 설치 형태**다. UBT 소스에서 확인한 두 줄이 전부다.

- `TargetRules.cs:2690-2693` — Program 타입 타깃이 `.uproject` 폴더 안에 있으면 **조건 없이** `Unique`
- `RulesAssembly.cs:677-680` — `Unique` + 설치본 엔진이면 예외를 던지고 중단

플래그가 아니라 검증 예외라 설정으로 우회할 수 없다. **해제 조건은 UE 5.8 소스 빌드 설치.**

파일을 `P1/Source/`에 남기지 않은 이유: `ProjectFileGenerator.cs:3176`이 **프로젝트 파일 생성 시에도**
같은 검증을 수행하므로, 두면 P1의 프로젝트 파일 재생성이 깨질 수 있다. 원인을 이 세션과 연결하기
어려운 형태의 고장이라 지뢰를 남기는 대신 `docs/references/`로 옮겨 보존했다.

**그동안의 대안** — UE 쪽 테스트가 통째로 막힌 것은 아니다. **L2(Simple Automation Test)는 별도
타깃이 필요 없다.** 게임/에디터 모듈 안에 컴파일되어 에디터 안에서 돌고, 설치본 엔진에서 동작한다.
소스 빌드를 들이기 전까지 UE 쪽은 L2부터 시작하는 편이 비용 대비 효과가 낫다.

---

## 결과

| 티어 | 상태 | 테스트 |
|---|---|---|
| 게임 서버 L1 | ✅ 동작 | 14 통과 + 1 `DISABLED_`(D-14) |
| 인증 서버 | ✅ 동작 | 2 통과 |
| UE 클라 L1 | `[B] blocked` | 소스 빌드 엔진 필요 |

이 인프라가 이 세션에 잡은 버그 **2건**: D-01(계획된 것), 그리고 `removeItem`의 슬롯 오염
(계획에 없던 것 — 코드를 눈으로 읽어서는 나오지 않았다). 상세는 tech-debt D-01·D-15.

판정 ADR §9의 해동 조건 "AC 사다리 3단을 채울 테스트 인프라 존재"가 **서버 한정으로 충족**됐다.
UE 클라 쪽은 아직이므로, 배치 모드 해동은 여전히 이르다.
