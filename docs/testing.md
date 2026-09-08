# 테스트 계층

설계 근거는 ADR-0002(`docs/decisions/0002-l1-test-infra.md`). 여기엔 실행 경로만 적는다.

---

## 지금 도는 것

| 대상 | 빌드 | 실행 (에이전트·CI) | 실행 (사람·IDE) |
|---|---|---|---|
| 게임 서버 L1 (GoogleTest) | `build_solution_start(rootFolder=.../Server)` → `build_solution_state` | `Server/Binary/Debug/GameServerTests.exe` | Rider 실행 구성 `GameServerTests` |
| 인증 서버 | — | `cd Server/AuthServer && npm test` | Rider npm 구성 |

**판정은 종료 코드다.** 0이 아니면 실패다.

테스트는 `Server/GameServerTests/`, gtest는 `Server/Libraries/googletest/`에 벤더링돼 있다(v1.18.0, gmock 없음). 인증 서버는 Node 내장 러너(`node --test`)라 새 의존성이 없다.

### 현재 커버리지

| 스위트 | 개수 | 대상 |
|---|---|---|
| `PacketSerialization` | 2 | S_CHAT 가변 문자열 · S_MOVE 중첩 메시지 protobuf 왕복 |
| `ProtocolContract` | 3 | `PROTOCOL_MESSAGES(X)` 목록 대조 · 패킷 ID 연속·유일성 · 전 메시지 리플렉션 왕복 |
| `InventoryTest` | 3 (+DISABLED 1) | 슬롯 타입 교차오염(D-01) · 더티 플래그 순서 · 실패한 remove 후 슬롯 재사용 |
| `AllSlotTypes/InventorySlotTypeTest` | 6 | 슬롯 추가·제거 왕복 전 타입 (TEST_P 2 × Gear/Consumable/Misc) |
| AuthServer `configs.test.js` | 2 | `.env` 필수 키 존재 · 커넥션 풀 크기 파싱 |

**안 덮는 것**: Room · DBRequestFunctions · 세션/IOCP · 전투 판정 · Gamedata 로딩 · AuthServer 라우터/인증 흐름 · UE 클라 전 계층. 전부 0개.

---

## 빌드는 Rider, 실행은 셸

`execute_run_configuration`은 호출마다 Rider가 확인 대화상자를 띄우고, 그걸 끄는 수단은 Brave 모드(IDE 전역으로 셸·실행구성 확인 해제)뿐이라 쓰지 않는다.

"UBT/MSBuild를 터미널로 직접 돌리지 않는다"는 규칙의 근거는 **출력 절단으로 에러가 유실되는 것**이고, 그건 빌드에만 성립한다 — gtest 출력은 짧고 완결적이다.

`Server/Binary/`는 gitignore되어 있으므로 **실행 전 빌드는 필수다.**

---

## 테스트를 추가할 때

`.cpp`를 `Server/GameServerTests/GameServerTests.vcxproj`의 `<ItemGroup Label="테스트 소스">`에 등록해야 한다. **이 프로젝트는 파일 자동 수집을 하지 않는다.**

등록을 잊으면 테스트가 조용히 안 돌아간다.

---

## 아직 없는 것 — 무엇을 확인했고 무엇을 안 했는지

**UE 클라의 기본 경로는 L2다.** 별도 빌드 타깃이 필요 없어 `P1` 모듈에 그대로 컴파일된다.

아래는 전부 **미착수**다. 「완료 기준」의 "존재 ≠ 가능"을 여기에도 적용해, 확인한 것과 확인하지 않은 것을 갈라 적는다. **착수할 때는 가장 싸게 실패하는 경로부터 돌린다** — 파일이 있는지 여러 번 확인하는 것보다 한 번 빌드해 보는 게 싸다.

| 계층 | 확인한 것 | 확인 안 한 것 |
|---|---|---|
| L2 게임 로직+입력 (Simple Automation Test + `InjectInputForAction`) | 설치본 `Core/Public/Misc/AutomationTest.h`에 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` 존재 | **실행 전체.** 컴파일·에디터 실행 다 안 해봤다 |
| L3 UI 입력 (Automation Spec + Automation Driver) | 없음 — **아직 안 봤다** | 전부 |
| L4 E2E (Gauntlet TestController) | 설치본에 `Engine/Plugins/Experimental/Gauntlet` 플러그인 + public `GauntletTestController.h` + 컴파일된 `Gauntlet.Automation.dll` | **실행 전체** |

- L2·L3 실행에는 **에디터가 필요하다** (`Window > Test Automation` 또는 `-ExecCmds="Automation RunTests ..."`). 서버처럼 무인 루프가 되지 않는다.
- **L3는 Live Coding 비호환 — TDD 루프 금지, 배치 전용.**
- L4는 DummyClient 자산 재사용 검토. 병렬 실행 시 포트 파라미터화.
- CI: D-12. 서버 쪽 전제는 갖춰졌다 — `GameServerTests`는 gitignore된 `config.h` 없이 빌드된다. UE 쪽은 에디터 의존 때문에 별도 검토가 필요하다.

---

## UE L1(Low-Level Tests)은 채택하지 않는다

런처 설치본에서 빌드가 거부되고(프로젝트 내 `TargetType.Program` 타깃 문제 — `docs/build.md`의 엔진 제약 참조), 이 프로젝트는 소스 빌드 엔진을 쓰지 않기로 확정했다.

손실은 작다 — L1으로 검증할 순수 로직은 전투 판정·인벤토리·레벨 테이블처럼 대부분 서버 소유이고 그쪽은 GoogleTest가 덮는다.

근거는 ADR-0002 결정 6.