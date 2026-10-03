# 테스트 계층

실행 경로와 그 근거를 함께 적는다.

---

## 지금 도는 것

| 대상 | 빌드 | 실행 (에이전트·CI) | 실행 (사람·IDE) |
|---|---|---|---|
| 게임 서버 L1 (GoogleTest) | `MSBuild Server.sln` (`docs/build.md` 「빌드 명령」) | `Server/Binary/Debug/GameServerTests.exe` | Rider 실행 구성 `GameServerTests` |
| 인증 서버 | — | `cd Server/AuthServer && npm test` | Rider npm 구성 |
| 인증 서버 정적 검사 | — | `cd Server/AuthServer && npm run lint` | Rider npm 구성 |
| UE 클라 L2 (Automation) | `Build.bat P1Editor` (`docs/build.md` 「빌드 명령」) | `pwsh P1/Scripts/Run-UeTests.ps1` | 에디터 `Window > Test Automation` |
| 규범 검사 | — | `py -3 Tools/ConventionLint/check_conventions.py` | 같은 명령 |

**판정은 종료 코드다.** 0이 아니면 실패다. 인증 서버는 `npm test`와 `npm run lint`가 둘 다 0이어야 완료다.

**UE 클라만 예외다.** `UnrealEditor-Cmd`는 테스트가 실패해도, 필터가 아무것도 맞추지 못해도 종료
코드 `0`을 돌려준다(2026년 9월 17일 실측, 두 경우 모두 확인). 그래서 `Run-UeTests.ps1`이 리포트의
`index.json`을 읽어 판정하고 자기 종료 코드를 낸다. **에디터의 종료 코드를 보지 않는다.**
실행 전에 지난 `index.json`을 지우므로 이전 결과가 초록으로 읽히지 않는다.

UE 테스트는 에디터를 띄우지 않고 돈다. 다만 **빌드에는 에디터를 닫아야 한다**
(`docs/build.md` 「빌드 명령」). 여기가 이 계층의 유일한 사람 손이다.

**규범 검사는 자기 검증을 먼저 돌린다.**
— 위반 0건과 대상 0건은 출력이 같다. 검사가 대상을 하나도 찾지 못하면 아무것도 검사하지 않으면서
초록을 낸다. `--self-test`가 일부러 어긋낸 입력을 다섯 검사에 먹여 위반이 실제로 잡히는지 보고,
규범을 지키는 입력에서는 잡지 않는지도 함께 본다. CI의 「규범 검사」 잡이 이 순서를 그대로 쓴다.

```
py -3 Tools/ConventionLint/check_conventions.py --self-test
py -3 Tools/ConventionLint/check_conventions.py
```

검사 대상은 git이 추적하는 파일뿐이다. 빌드도 엔진도 필요 없으므로 호스티드 러너에서 그대로
돈다. 검사 항목과 그 근거 조항은 스크립트 첫머리의 표에 있다.

테스트는 `Server/GameServerTests/`, gtest는 `Server/Libraries/googletest/`에 벤더링돼 있다(v1.18.0, gmock 없음). 인증 서버는 Node 내장 러너(`node --test`)라 새 의존성이 없다.

정적 검사는 ESLint 9다. flat config(`Server/AuthServer/eslint.config.js`)가 `@eslint/js`의 recommended를 적용한다. `node_modules/`와 `obj/`는 검사하지 않는다. `obj/`는 `.esproj`가 남긴 NuGet 복원 산출물이라 소스가 아니다.

### 현재 커버리지

| 스위트 | 개수 | 대상 |
|---|---|---|
| `PacketSerialization` | 2 | S_CHAT 가변 문자열 · S_MOVE 중첩 메시지 protobuf 왕복 |
| `ProtocolContract` | 3 | `PROTOCOL_MESSAGES(X)` 목록 대조 · 패킷 ID 연속·유일성 · 전 메시지 리플렉션 왕복 |
| `PacketDispatch` | 1 | 핸들러 테이블 밖의 id(65535)를 디스패치하지 않고 거절 |
| `InventoryTest` | 10 (+DISABLED 1) | 슬롯 타입 교차오염 · 더티 플래그 순서 · 실패한 remove 후 슬롯 재사용 · 알 수 없는 슬롯 타입과 범위 밖 슬롯 번호 거부 · 매핑 표 키 집합과 기대 집합 대조 |
| `AllSlotTypes/InventorySlotTypeTest` | 6 | 슬롯 추가·제거 왕복 전 타입 (TEST_P 2 × Gear/Consumable/Misc) |
| AuthServer `configs.test.js` | 2 | `.env` 필수 키 존재 · 커넥션 풀 크기 파싱 |
| `P1.Network.PacketFraming` | 1 | 패킷 헤더의 size·id 배치 · 본문 왕복 · 빈 메시지 경계 |
| `P1.Sync.MoveCorrection` | 1 | 원격 크리처 보정의 순간이동 경계(800) · 정지 중 접근 · 이동 중 수선의 발 접근 · Z 유지 · 회전 보정 켜고 끄기 · ACTION 중 보정 멈춤과 순간이동 |
| `P1.Sync.MoveSendThrottle` | 1 | 내 플레이어 이동 패킷의 주기 송신(0.2초)과 타이머 리셋 · 입력 변화 즉시 송신(이동 가능할 때만) · 회전 허용치(60도) 경계와 ±180도 감싸기 · 입력이 없을 때 마지막으로 보낸 yaw와의 비교 · 공격 중 즉시 송신 억제 |
| `P1.Inventory.SlotAction` | 1 | 인벤토리 칸 더블클릭의 요청 판정. 빈 칸 · 소모품 사용 · 무기와 방어구 착용 · 요구 레벨 경계 · 기타 칸과 착용 장비 칸 · 재사용 대기 중인 소모품 |
| `P1.Inventory.ItemCooldown` | 1 | 아이템 재사용 대기의 남은 시간과 남은 비율(1 → 0) · 끝나는 순간의 경계 · 길이가 0 이하인 아이템 |
| `P1.Combat.NormalAttackCombo` | 1 | 일반 공격의 콤보 순번 순환(1→N→1) · 몽타주가 하나이거나 없을 때 · 순번 N의 몽타주 인덱스 · 서버가 보낸 순번 0 · 범위 밖 순번 |
| `P1.Progress.RewardResult` | 1 | 보상 결과의 반영. 경험치만 쌓일 때 레벨을 알리지 않음 · 여러 레벨 상승의 레벨, 레벨업 스탯, 최대 경험치 · 경험치를 알릴 때 최대 경험치가 이미 새 값 · 골드를 사본에 쓴 뒤 알림 |

**안 덮는 것**: Room · DAO의 SQL 실행 · 세션/IOCP · Gamedata 로딩 · AuthServer 라우터/인증 흐름. 전부 0개. UE 클라는 패킷 프레이밍, 이동 보정 계산, 이동 패킷 송신 판정, 인벤토리 칸 요청 판정, 아이템 재사용 대기 계산, 일반 공격 콤보 순번, 보상 결과 반영 일곱뿐이고 나머지 계층은 0개다.

---

## TDD가 도는 범위

| 대상 | 러너 | 루프 |
|---|---|---|
| GameServer 순수 로직 | `GameServerTests.exe`, 종료 코드 | 가능 |
| AuthServer 설정 | `npm test` | 가능. 가장 빠름 |
| GameServer Room·DB·IOCP | 없음 | **불가.** JobQueue 비동기 |
| AuthServer 라우터·인증 | 없음 | 가능하나 비쌈 (bcrypt+MSSQL+Redis) |
| UE 클라 순수 로직 | `Run-UeTests.ps1`, 종료 코드 | 가능. 다만 한 바퀴마다 에디터를 닫고 빌드해야 한다 |
| UE 클라 액터·월드 의존 로직 | 없음 | **불가.** 월드를 띄우는 테스트를 아직 써 보지 않았다 |

불가 영역은 지금 마땅한 테스트가 없다.

---

## seam

관찰 가능한 공개 경계. 여기서 테스트하면 내부를 전부 다시 써도 동작이 살아남는다.

**합의하지 않은 seam에는 테스트를 쓰지 않는다.** 사람이 먼저 승인한다.

| seam | 상태 |
|---|---|
| `Inventory` 공개 API | 존재 |
| protobuf 메시지 왕복 | 존재. 리플렉션으로 자동 확장 |
| 프로토콜 ID 목록 | 존재 |
| 패킷 디스패치 | 존재. `ServerPacketHandler::HandlePacket`. 서버 생성물만 테스트한다. 클라이언트와 DummyClient의 헤더는 같은 템플릿에서 생성되지만 테스트하지 않는다 |
| 클라 패킷 프레이밍 | 존재. `ClientPacketHandler::MakeSerializedPacket`의 공개 오버로드 |
| 원격 크리처 이동 보정 | 존재. `FP1MoveCorrection::Compute`. 이동 상태 분기와 크리처 종류 판정은 `UP1MoveSyncComponent::TickRemote`에 있다 |
| 내 플레이어 이동 패킷 송신 판정 | 존재. `FP1MoveSendThrottle::Decide`. 이동 상태 판정과 패킷 구성은 `UP1MoveSyncComponent::TickMyPlayer`에 있다 |
| 인벤토리 칸 더블클릭의 요청 판정 | 존재. `FP1InventorySlotAction::Decide`. 응답 대기와 패킷 구성은 `UP1InventoryWidget::HandleSlotDoubleClicked`에 있다 |
| 아이템 재사용 대기 계산 | 존재. `FP1ItemCooldown`. 템플릿별 대기의 보관과 시작은 `UP1MyPlayerData`에, 막대 갱신은 `UP1SlotWidget::RefreshCooldown`에 있다 |
| 일반 공격 콤보 순번과 몽타주 선택 | 존재. `FP1NormalAttackCombo`. 몽타주 재생, 입력 가능 상태, 2초 초기화 타이머는 `UP1AttackSystemComponent`에 있다 |
| 보상 결과 반영 | 존재. `UP1MyPlayerData::HandleRewardResult`. 경험치와 레벨은 서버가 계산하므로 클라이언트는 사본에 쓰고 알리기만 한다 |
| 전투 판정 | 없음. `Room` 안에 얽혀 있다 |
| `Gamedata` 테이블 로딩 | 미확인 |

seam이 없으면 만드는 작업이 선행된다. 그것은 리팩토링이므로 별도 계획을 세운다.

---

## 빌드도 실행도 셸에서 한다

**빌드는 터미널에서 돌리고 종료 코드로 판정한다. 빌드에 Rider MCP를 쓰지 않는다.** 두 티어의 명령은 `docs/build.md` 「빌드 명령」에 있다. 그렇게 정한 이유와 Rider 경로의 오보 사례는 `docs/adr/0001-unify-build-path.md`에 있다.

`execute_run_configuration`은 호출마다 Rider가 확인 대화상자를 띄우고, 그걸 끄는 수단은 Brave 모드(IDE 전역으로 셸·실행구성 확인 해제)뿐이라 쓰지 않는다.

`Server/Binary/`는 gitignore되어 있으므로 **실행 전 빌드는 필수다.**

파일 편집에는 셸이 아니라 편집 도구를 쓴다. 근거는 `CLAUDE.md` 「안전」에 있다.

### Live Coding은 메인 DLL을 대체하지 않는다

에디터에서 Live Coding으로 컴파일하면 헤더 변경과 `UFUNCTION` 같은 리플렉션 변경이 반영되지
않는다. 결과는 `P1/Saved/Logs/P1.log`의 `LogLiveCoding`으로 판정한다. 성공하면
`Live coding succeeded`를 남긴다.

**Live Coding으로 검증한 코드는 정식 빌드를 거친 것이 아니다.** 패치는
`Binaries/Win64/UnrealEditor-P1.patch_N.*`로 따로 나가고 `UnrealEditor-P1.dll`은 그대로 남는다.
에디터를 닫고 다시 열면 패치가 사라지고 옛 바이너리가 로드된다. **커밋하기 전에 에디터를 닫고
전체 빌드를 한 번 돌린다.**

새 코드가 메인 DLL에 들어갔는지는 PDB의 심볼로 확인한다.

```
strings -n 6 P1/Binaries/Win64/UnrealEditor-P1.pdb | grep -c <새 심볼>
```
— 셸을 거치지 않는 편집 도구를 쓴다. 이것이 예방이다.

**빌드한 뒤 `.obj`와 실행 파일의 수정 시각을 소스와 대조한다.**
— 산출물이 소스보다 오래됐으면 재컴파일되지 않은 것이다. `buildIsSuccess`만으로는 두 경우가 구별되지 않는다. 이것이 확인이다.

위 두 절차 중 하나만 쓰지 않는다. 회피가 매번 통하지는 않으므로 예방과 확인을 함께 쓴다.

---

## 테스트를 추가할 때

`.cpp`를 `Server/GameServerTests/GameServerTests.vcxproj`의 `<ItemGroup Label="테스트 소스">`에 등록해야 한다. **이 프로젝트는 파일 자동 수집을 하지 않는다.**

등록을 잊으면 테스트가 조용히 안 돌아간다.

---

## 왜 이 구조인가

### gtest는 소스 벤더링이다. vcpkg를 쓰지 않는다

`Server/Libraries/googletest/`에 v1.18.0(커밋 `063de7e`) 소스를 넣고 `gtest-all.cc`와 `gtest_main.cc`를 테스트 프로젝트가 직접 컴파일한다. gmock은 넣지 않았다.

**근거는 이 저장소가 이미 모든 서드파티를 벤더링한다는 점이다.** `Server/Libraries/include/`의 `google`, `nlohmann`, `sw`, `hiredis`가 전부 그렇다. gtest만 다른 메커니즘을 들이면 새 클론에 "vcpkg를 설치한다"는 단계가 하나 늘고, 그 단계는 문서에만 존재하게 된다. 벤더링은 툴셋·CRT 완전 일치(v145로 같이 컴파일), 머신 선행조건 없음, 네트워크 없음을 동시에 만족한다. 대가는 저장소 용량 1.1MB다.

gtest는 `main()`을 재정의하므로 vcpkg가 자동 링크해 주지 못하는 예외 라이브러리다. manifest를 쓰더라도 `.vcxproj`에 수동 설정이 세 군데 붙는다.

**재검토 조건**: 서버에 벤더링할 의존성이 3개 이상 더 늘거나, gtest 버전 갱신이 번거로워지는 시점.

### 테스트 프로젝트가 GameServer의 `.cpp`를 직접 포함한다

`GameServer`는 exe라 링크할 수 없다. `GameServerTests.vcxproj`(콘솔 exe)가 **`Main/GameServer.cpp`를 제외한 GameServer `.cpp` 전부**를 `ClCompile`로 포함한다. UE Low-Level Tests와 같은 패턴이다.

"필요한 것만"이 아니라 "main 빼고 전부"인 이유는 둘이다.

- `Inventory.cpp` → `Player.cpp` → `Room.cpp` → `GameSession.cpp` → `ProgressStorage.cpp`로 전이 의존이 이어져 결국 대부분을 넣게 된다. 링크 에러가 날 때마다 파일을 추가하는 루프는 비결정적이라 재현되지 않는다.
- `Main/GameServer.cpp`에는 `main`이 있다. 테스트 타깃은 자기 `main`(`TestMain.cpp`)을 쓰므로 이 파일만 뺀다.

---

## 아직 없는 것 — 무엇을 확인했고 무엇을 안 했는지

**UE 클라의 기본 경로는 L2다.** 별도 빌드 타깃이 필요 없어 `P1` 모듈에 그대로 컴파일된다.

아래 표의 L3·L4는 **미착수**다. L2는 순수 로직 테스트만 돌고 입력과 월드를 쓰는 테스트는 미착수다. 「완료 기준」의 "존재 ≠ 가능"을 여기에도 적용해, 확인한 것과 확인하지 않은 것을 갈라 적는다. **착수할 때는 가장 싸게 실패하는 경로부터 돌린다** — 파일이 있는지 여러 번 확인하는 것보다 한 번 빌드해 보는 게 싸다.

| 계층 | 확인한 것 | 확인 안 한 것 |
|---|---|---|
| L2 게임 로직+입력 (Simple Automation Test + `InjectInputForAction`) | 순수 로직 테스트의 컴파일과 실행 · 실패한 테스트의 `failed` 집계와 `Run-UeTests.ps1`의 종료 코드 1 (2026년 10월 2일, `P1.Sync.MoveCorrection`의 빨강 단계에서 실측) | `InjectInputForAction`으로 입력을 넣는 테스트. 월드를 띄우는 테스트 |
| L3 UI 입력 (Automation Spec + Automation Driver) | 없음 — **아직 안 봤다** | 전부 |
| L4 E2E (Gauntlet TestController) | 설치본에 `Engine/Plugins/Experimental/Gauntlet` 플러그인 + public `GauntletTestController.h` + 컴파일된 `Gauntlet.Automation.dll` | **실행 전체** |

- L2의 기본 실행 경로는 `Run-UeTests.ps1`이다. 에디터를 띄우지 않는다. 대체 경로는 둘이다. 하나는 사람이 쓰는 에디터의 `Window > Test Automation`이고, 다른 하나는 에디터가 이미 떠 있을 때 프로세스 시작 비용을 치르지 않는 언리얼 MCP의 `RunTests`다. 두 대체 경로는 리포트를 판정하지 않으므로 완료 판정에 쓰지 않는다.
- L3 실행에는 **에디터가 필요하다** (`Window > Test Automation` 또는 `-ExecCmds="Automation RunTests ..."`). 서버처럼 무인 루프가 되지 않는다.
- **L3는 Live Coding 비호환 — TDD 루프 금지, 배치 전용.**
- L4는 병렬 실행 시 포트 파라미터화.
- CI: 서버 쪽 전제는 갖춰졌다. 저장소에 없는 파일 없이 `GameServerTests`가 빌드된다. UE 쪽은 에디터 의존 때문에 별도 검토가 필요하다.

---

## UE L1(Low-Level Tests)은 채택하지 않는다

**재검토 조건은 없다.** 폐기다. 근거는 둘이고 순서가 중요하다.

**1. 프로젝트 제약.** 이 프로젝트는 런처 설치본 엔진만 쓴다. 소스 빌드는 선택지가 아니다(2026-08-27 확정). 아래 2번이 뒤집히더라도 결론은 그대로다.

**2. 기술적 불가.** 설치본에서 LLT 타깃은 빌드 자체가 거부되고 설정으로 우회할 수 없다.

표면의 거부는 두 줄이다.

- `TargetRules.cs:2690-2693` — Program 타입 타깃이 `.uproject` 폴더 안에 있으면 조건 없이 `Unique`
- `RulesAssembly.cs:677-680` — `Unique` + 설치본 엔진이면 예외를 던지고 중단

여기까지만 보면 검사를 통과시키면 될 것처럼 읽힌다. 타깃에서 `BuildEnvironment = Shared`를 지정하면 실제로 저 분기를 건너뛰고, 커뮤니티 workaround도 그것이다. **그런데 LLT에는 통하지 않는다.** `TestTargetRules.SetupCommonProperties`가 `bCompileAgainstEngine = false`, `bCompileAgainstEditor = false`, `LinkType = Monolithic`, `STATS=0` 외 다수를 조건 없이 잡는데, 이 목록이 곧 "빌드 환경"의 정의다. 설치본이 주는 프리빌트 바이너리는 정반대 설정으로 컴파일돼 있다. `Shared`로 강제한다는 것은 UBT에게 그 바이너리를 재사용하라고 시키는 것이라, 검사를 통과시켜도 정의가 어긋난 산출물이 나온다.

플래그로 우회할 성질이 아니라 설치본의 정의상 한계다. 유일한 해제 수단인 소스 빌드 엔진은 근거 1에 의해 선택지가 아니다.

손실은 작다 — L1으로 검증할 순수 로직은 전투 판정·인벤토리·레벨 테이블처럼 대부분 서버 소유이고 그쪽은 GoogleTest가 덮는다. 클라에 남는 L1 대상은 이동 보간 수식 정도로 얇다.

**`P1/Source/`에 Program 타깃을 추가하지 않는다.** `ProjectFileGenerator.cs:3176`이 프로젝트 파일 생성 시에도 같은 검증을 하므로, 두면 P1의 프로젝트 파일 재생성이 깨진다.
