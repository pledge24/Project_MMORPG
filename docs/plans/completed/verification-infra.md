# L1 테스트 인프라 구축 + 첫 실전 버그(D-01)

- 세션: 2 (2026-08-27)
- 상태: **완료** (Phase B만 `[B] blocked`)
- 관련: `docs/decisions/2026-08-27-l1-test-infra.md`(이 세션의 ADR) ·
  `docs/decisions/2026-08-19-final-harness-verdict.md` §9 · `docs/tech-debt.md`
- 표기: `[x]` 완료(옆에 summary 한 줄) · `[B] blocked — 무엇이/어디서`

## Context

자동 테스트가 0개였다. 그래서 세 가지가 동시에 막혀 있었다.

1. **완료 기준이 "빌드 통과"에서 멈춘다.** "자기신고를 믿지 않는다"는 검증 커맨드를 요구하는데
   서버에는 실행할 커맨드가 없었다.
2. 판정 ADR §9의 배치 모드 해동 조건 중 하나가 "AC 사다리 3단을 채울 테스트 인프라 존재"였다.
3. D-03(갓 클래스) 같은 리팩토링에 회귀 그물이 없었다.

이 세션은 서버 L1 파이프라인을 세우고, 그 파이프라인으로 D-01을 TDD로 잡았다.

---

## Phase A — 게임 서버 GoogleTest

- [x] **A-1 googletest 벤더링** — `Server/Libraries/googletest/{include,src,LICENSE,README.ko.md}`.
      v1.18.0(커밋 `063de7e`), gmock 제외, 38파일 1.1MB.
      *세션 1 권고(vcpkg)에서 변경* — vcpkg 미설치 + gtest는 MSBuild 자동 링크 불가(MS 문서) +
      리포가 이미 전 서드파티를 벤더링 중. 사용자 승인 후 진행. 근거 표는 ADR 결정 1.
- [x] **A-2 테스트 프로젝트** — `Server/GameServerTests/GameServerTests.vcxproj` + `Server.sln` 등록.
      `Main/GameServer.cpp`(main, gitignore된 `config.h`의 유일한 소비자)만 빼고 GameServer .cpp 전부를
      직접 컴파일. **첫 빌드에 통과.** Rider가 `GameServerTests` 실행 구성을 자동 생성.
- [x] **A-3 파이프라인 증명** — `PacketSerializationTests.cpp`(S_CHAT 가변 문자열, S_MOVE 중첩).
      작성→빌드→실행→확인 루프 성립 확인, 종료 코드 0.
- [x] **콘솔 UTF-8** *(계획에 없던 항목)* — `TestMain.cpp`가 `gtest_main.cc`를 대체하고
      `SetConsoleOutputCP(CP_UTF8)`를 부른다. cp949 콘솔에서 한국어 실패 메시지가 깨져
      (`湲고? ?щ’ ??젣媛`) 읽히지 않았다. 읽히지 않는 진단은 하네스로서 제 역할을 못 한다.
- [x] **A-4 D-01 TDD** — `InventoryTests.cpp`. 실패 3개(MISC만) 확인 → `Inventory.cpp:44`
      `ITEM_TYPE_GEAR`→`ITEM_TYPE_MISCELLANEOUS` → 9/9 통과.
      `RemovingMiscItemDoesNotTouchGearInventory`가 D-01이 서술한 피해를 그대로 재현했다.
- [x] **A-5 D-15 + 신규 버그** — 실패 2개 확인 → `removeItem` 재구조화 → 11/11 통과.
      D-15(더티 플래그 순서)만 고치려 했는데 **같은 자리에서 더 큰 것이 나왔다**:
      `mutable_item()`이 검증보다 먼저 불려 빈 슬롯을 영구 점유 상태로 만들고 있었다.
      한 번의 재구조화로 둘 다 해소. 상세는 tech-debt D-15.
- [x] **A-5 D-14 — 범위 판단으로 중단** — `DISABLED_StackDoesNotExceedMaxStack`로 남겼다.
      `--gtest_also_run_disabled_tests`로 **실제 빨강임을 확인**했다(미검증 스텁 아님).
      중단 이유: 초과분을 다음 슬롯으로 넘기면 슬롯 두 개가 바뀌는데 `S_BUY_ITEM`은 `Slot` 하나만
      나른다 → 프로토콜 변경이 필요하다. tech-debt D-14에 난이도 중→상으로 상향 기록.
- [x] **A-6 전수 왕복 + ID 불변식** — `ProtocolContractTests.cpp` 3개.
      수작업 40개 대신 `PROTOCOL_MESSAGES(X)` 매크로 목록 하나가 이름·순서 검사와 패킷 ID 검사를
      함께 구동하고, 왕복은 `DescriptorPool` 리플렉션으로 전 메시지를 돈다(메시지가 늘어도 안 고침).
      첫 실행에서 어서션 버그 1건 자체 발견(필드 없는 메시지 5개를 고려 안 함) → 수정.

## Phase C — 인증 서버

- [x] **C 최소 테스트** — `src/Config/configs.test.js` 2개, `package.json`에 `"test": "node --test"`.
      Node v20.12.2 내장 러너라 **새 의존성 0개**. 2/2 통과, 종료 코드 0.
      `configs.js`를 고른 이유: `.env`가 gitignore돼 새 클론에 없고, 키가 빠져도 기동은 성공한 뒤
      DB·Redis 접속 시점에야 터진다. `parseInt(undefined)`는 NaN이라 풀 설정이 조용히 무의미해진다.
      *(D-18 eslint는 범위 밖 — 건드리지 않음)*

## Phase B — UE 클라 Low-Level Tests

- [B] **blocked — `P1Tests` 타깃 빌드 / 런처 설치본(Installed Build) 엔진**

      `Build.bat P1Tests Win64 Development` → `Targets with a unique build environment cannot be
      built with an installed engine.` (RulesError, exit 8)

      UBT 소스에서 원인 확인: `TargetRules.cs:2690-2693`이 프로젝트 폴더 안의 Program 타깃을
      **조건 없이** `Unique`로 잡고, `RulesAssembly.cs:677-680`이 설치본에서 `Unique`를 거부한다.
      `BuildEnvironment = Shared`로 저 분기를 건너뛸 수는 있으나 LLT에는 통하지 않는다 —
      `TestTargetRules.SetupCommonProperties`가 잡는 속성 목록이 곧 빌드 환경의 정의이고
      설치본 바이너리는 정반대 설정으로 컴파일돼 있다. 설정 문제가 아니라 설치본의 정의상 한계다.

      **막힌 범위는 UE L1 한 계층뿐이다.** 게임 빌드·실행·패키징과 무관하고 L2/L3/L4는
      설치본에서 그대로 된다. **소스 빌드 엔진은 필수가 아니라 선택이다** —
      다른 이유로 필요해졌을 때 덤으로 가져가는 것이 맞다.

      *이 판정에서 내가 틀렸던 것*: 계획 단계에서 설치본에 Catch2·LowLevelTestsRunner가 있는 것을
      확인하고 "LLT 가능"이라고 적었다. 재료 존재로 능력을 추론했고, 같은 조사에서 구멍 하나
      (`LowLevelTests.xml` 부재)를 찾아 메운 것이 나머지에 대한 확신을 오히려 높였다.
      → CLAUDE.md 「완료 기준」에 **"존재 ≠ 가능"** 규칙으로 승격.

      작성한 파일 3개는 엔진 예제 구조를 그대로 따랐고 문제가 없다 →
      `docs/references/p1-lowlevel-tests/`에 README와 함께 보존.
      `P1/Source/`에 두지 않은 이유: `ProjectFileGenerator.cs:3176`이 프로젝트 파일 생성 시에도
      같은 검증을 하므로 P1 프로젝트 파일 재생성이 깨질 수 있다(지뢰 회피).

## Phase D — 마무리

- [x] **CLAUDE.md** — 「테스트 계층」을 추정형에서 실측 실행 경로 표로 교체(빌드=Rider / 실행=셸,
      사람용 Run Configuration 병기). 「완료 기준」에 "테스트가 존재하는 영역은 테스트 통과까지가
      완료" 추가, 적용 범위를 게임 서버·인증 서버로 명시.
- [x] **ADR** — `docs/decisions/2026-08-27-l1-test-infra.md`. 결정 6건과 기각안
      (vcpkg / MS NuGet / Catch2 통일 / Brave 모드), 재검토 조건.
- [x] **tech-debt** — D-01·D-15 수정 완료 표기, D-14 난이도 상향, **D-22·D-23 신규**, D-12에 진행분 추가.
- [x] **메모리** — Rider Brave 모드 비활성화 약속을 feedback 메모리로 저장.
- [x] 계획 파일 `completed/` 이동, 숫자 보고, 다음 세션 후보 제안.

---

## 다음 세션 후보 (진단 근거와 함께)

우선순위 순. 근거는 이 세션에서 실측한 것만 적는다.

1. **Inventory 정리 — D-22 + D-23** *(가장 싸고 근거가 확실함)*
   D-22는 클라가 보낸 슬롯 타입/id가 검증 없이 `operator[]`와 `Mutable()`로 직행해 **널 역참조**가
   가능한 자리다. D-23은 D-01의 근본 원인(손으로 유지되는 매핑 3종). 둘은 같은 함수를 건드리므로
   한 번에 처리된다. **그물이 이미 있다** — `InventoryTests.cpp`가 세 슬롯 타입을 전부 검사한다.
   리팩토링 첫 대상으로 위험이 가장 낮다.

2. **D-14 — 스택 상한 (프로토콜 변경 연습)**
   `DISABLED_`를 떼는 것이 완료 신호다. `.proto` → 생성기 → 클라·서버 3곳 수정이 필요해
   **"게임플레이 변경은 3곳"이라는 이 프로젝트의 기본 규약을 테스트가 있는 상태에서 처음 밟아보는**
   연습이 된다. 규모가 작아 연습용으로 적절하다.

3. **DummyClient 승격 (인자화 → 실토큰 → 봇 상태머신 → 종료코드)**
   L4의 재료이자 D-07(룸 잡 큐 우회) 검증 수단. 종료 코드를 갖게 되면 이 세션이 만든
   "종료 코드가 판정"이라는 규약에 그대로 편입된다. 단 실토큰 단계에서 Redis·AuthServer가
   함께 떠 있어야 해서 L1보다 환경 의존이 크다.

4. **CI (D-12)** — 전제가 충족됐다. `GameServerTests`는 `config.h` 없이 빌드되고 판정이 종료 코드다.
   AuthServer도 `npm test`가 생겼다. 남은 건 워크플로 작성뿐이라 비용이 작다.

5. **UE 쪽 — L2 Automation Test** *(경로 확정됨, 별도 세션으로 계획할 것)*
   L1(LLT)은 보류지만 L2는 별도 타깃이 필요 없어 설치본에서 바로 된다
   (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`가 설치본 `Core/Public/Misc/AutomationTest.h:4297`에 존재).
   착수 단계:
   `P1/Source/P1/Tests/`에 테스트 1개 → `build_solution_start(rootFolder=".../P1")`로 에디터 타깃
   빌드 → 에디터 `Window > Test Automation`(사람) 또는 `-ExecCmds="Automation RunTests ..."`.
   **실행에 에디터가 필요해 사람 손이 섞인다** — 서버 L1처럼 무인 루프가 되지 않으므로
   착수 전에 별도 계획을 세운다. 첫 대상 후보는 이동 보간 수식(`ACreature`의 `MoveQueue`) 정도로,
   클라 쪽에 남은 순수 로직이 얇다는 점도 함께 고려할 것.

6. **리팩토링 착수 (grilling + improve-codebase-architecture 도입)**
   판정 ADR이 "리팩토링 착수 직전 채택"으로 예약한 것들. **D-03(Room 분해)은 Room에 테스트가 붙은
   뒤가 안전하다** — 현재 그물은 Inventory와 프로토콜에만 있다. 순서상 1~2를 먼저 하는 게 맞다.

**하지 말 것으로 확인된 것** — 배치 실행 모드 해동은 아직 이르다. 판정 ADR §9의 조건
"AC 사다리 3단을 채울 테스트 인프라"가 **서버 한정으로만** 충족됐고 UE 클라 쪽은 비어 있다.
