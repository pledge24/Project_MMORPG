---
작업: l1-test-infra
세션: 2
날짜: 2026-08-27
상태: 완료
관련: decisions/0002-l1-test-infra.md · decisions/0001-harness-verdict.md §9 · tech-debt D-01, D-14, D-15, D-22, D-23
---

> **하네스 전환 시 처리 (2026-09-XX)**
> 프론트매터 추가, 헤더 불릿 흡수, `Context` → `왜 이 작업인가` 제목 변경,
> 「다음 세션 후보」를 `docs/NEXT.md` 로 이관. **본문 미변경.**
> 「결정」·「위험 요소」·「증명」·「작업 중 기록」 절은 이 시점 양식에 없었다.
> 누락이 아니라 시점 차이다.
>
> 본문의 ADR 경로는 작성 당시 파일명이다. 1d 에서 다음과 같이 번호로 바뀌었다.
> `2026-08-19-final-harness-verdict.md` → `0001-harness-verdict.md`
> `2026-08-27-l1-test-infra.md` → `0002-l1-test-infra.md`

# L1 테스트 인프라 구축 + 첫 실전 버그(D-01)

표기: `[x]` 완료(옆에 summary 한 줄) · `[B] blocked — 무엇이/어디서`

## 왜 이 작업인가

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

      **막힌 범위는 UE L1 한 계층뿐이다.** 게임 빌드·실행·패키징과 무관하다.

      *이 판정에서 내가 틀렸던 것*: 계획 단계에서 설치본에 Catch2·LowLevelTestsRunner가 있는 것을
      확인하고 "LLT 가능"이라고 적었다. 재료 존재로 능력을 추론했고, 같은 조사에서 구멍 하나
      (`LowLevelTests.xml` 부재)를 찾아 메운 것이 나머지에 대한 확신을 오히려 높였다.
      → CLAUDE.md 「완료 기준」에 **"존재 ≠ 가능"** 규칙으로 승격.

      **→ 사후 확정 (같은 날): `[B] blocked`가 아니라 폐기다.** 사용자가 이 프로젝트는
      런처 설치본 엔진만 쓰고 소스 빌드는 하지 않는다고 확정했다. 해제 조건이 오지 않으므로
      보류로 둘 이유가 없다. 작성했던 파일 3개와 보존 폴더
      `docs/references/p1-lowlevel-tests/`는 **제거했다** — 오지 않을 해동을 위한 절차는
      살아 있는 부패다. 기록은 `docs/decisions/2026-08-27-l1-test-infra.md` 결정 6 하나로
      모았고, 파일은 git 이력에 남아 있다. UE 경로는 L2로 확정.

## Phase D — 마무리

- [x] **CLAUDE.md** — 「테스트 계층」을 추정형에서 실측 실행 경로 표로 교체(빌드=Rider / 실행=셸,
      사람용 Run Configuration 병기). 「완료 기준」에 "테스트가 존재하는 영역은 테스트 통과까지가
      완료" 추가, 적용 범위를 게임 서버·인증 서버로 명시.
- [x] **ADR** — `docs/decisions/2026-08-27-l1-test-infra.md`. 결정 6건과 기각안
      (vcpkg / MS NuGet / Catch2 통일 / Brave 모드), 재검토 조건.
- [x] **tech-debt** — D-01·D-15 수정 완료 표기, D-14 난이도 상향, **D-22·D-23 신규**, D-12에 진행분 추가.
- [x] **메모리** — Rider Brave 모드 비활성화 약속을 feedback 메모리로 저장.
- [x] 계획 파일 `completed/` 이동, 숫자 보고, 다음 세션 후보 제안.