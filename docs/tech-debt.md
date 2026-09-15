# Tech Debt

지금 틀린 것만 담는다. 해결이 확정되면 항목을 지운다 — 수정 완료 표기를 남기지 않는다.
무엇을 어떻게 고쳤는지는 커밋이 갖는다.

항목 21개 (높음 3 · 중간 12 · 낮음 6)

## 작성 방법

번호를 붙이지 않는다. 제목이 식별자다.

심각도가 높은 항목을 위에 둔다. 심각도가 같으면 난이도가 낮은 항목을 위에 둔다.

### 새 항목 양식

아래 블록을 복사해서 쓴다.

```markdown
## 무엇이 틀렸는가를 한 줄로
> **심각도:** 높음 · **난이도:** 중간 · **범위:** 기능 · client
> 위치: `경로` N~N줄
> 등록일: 2026년 0월 0일

문제 상황을 적는다. 필요하면 표를 쓴다.

### 영향

**문제 유형** · **문제 유형** — 방치했을 때의 손실과 리스크를 실측 결과로 적는다.
```

- 줄 번호는 특정할 수 있을 때만 적는다. 파일 전체가 문제면 괄호에 총 줄 수를 적는다
- 경로가 넷을 넘거나 특정할 수 없으면 공통 상위 디렉터리를 적는다
- 영역은 하나만 고른다. 나머지는 본문에서 언급한다
- 문제 유형은 최대 두 개까지 적는다
- 항목을 추가하거나 지우면 파일 맨 위의 개수 줄을 함께 고친다

### 심각도와 난이도

| 값 | 심각도 | 난이도 |
|---|---|---|
| 높음 | 지금 장애나 데이터 손상으로 이어진다 | 여러 영역을 고치거나 설계를 바꿔야 한다 |
| 중간 | 개발 속도와 변경 안전성을 깎는다 | 여러 파일이나 모듈을 고치지만 구조는 그대로다 |
| 낮음 | 불편하지만 손실이 드러나지 않는다 | 단일 파일이나 제한된 범위에서 끝난다 |

### 범위

부채가 영향을 미치는 코드의 범위다.

| 값 | 기준 |
|---|---|
| 함수 | 특정 함수 또는 메서드에 한정된다 |
| 파일 | 하나의 파일에 한정된다 |
| 모듈 | 하나의 모듈이나 패키지에 영향을 준다. 한 폴더의 동종 에셋 여러 개도 여기에 넣는다 |
| 기능 | 하나의 기능이나 사용자 시나리오 전반에 영향을 준다 |
| 프로젝트 | 프로젝트 전반의 구조나 여러 기능에 영향을 준다 |

### 영역

| 값 | 구속 대상 |
|---|---|
| `client` | `P1/` 아래. UE 클라이언트 |
| `server` | `Server/GameServer`, `Server/AuthServer` |
| `protocol` | `.proto`와 생성 파이프라인. 클라와 서버를 동시에 구속한다 |
| `shared` | 양쪽이 쓰는 공용 코드 |
| `build` | 빌드 구성, 솔루션, 의존성, 테스트 인프라 |
| `ops` | DB 스키마, 배포, 운영 |

### 문제 유형

변경 비용 증가 · 버그 발생 가능성 증가 · 변경 영향 범위 확대 · 새 기능 개발 지연 ·
유지보수 어려움 · 테스트 어려움 · 동일한 문제의 반복 · 부채의 연쇄 증가

---

## 네트워크 수신 펌프가 레벨 블루프린트에 있다
> **심각도:** 높음 · **난이도:** 중간 · **범위:** 기능 · client
> 위치: `P1/Source/P1/P1GameInstance.cpp` 97줄 · `P1/Content/Maps/`
> 등록일: 2026년 8월 19일

`UP1GameInstance::HandleRecvPackets()`를 호출하는 C++ 코드가 없다. 호출부는 레벨 스크립트
블루프린트의 `ReceiveTick` 안이고, 5개 맵 중 3개에만 있다(UE 에디터 실측).

| 맵 | 레벨 BP | 펌프 |
|---|---|---|
| `InGameMap` `LoginMap` `TestMap` | 있음 | 돈다 |
| `TownMap` `CrashTestMap` | 레벨 스크립트 BP 자체가 없음 | 돌지 않는다 |

프레임워크가 강제하지 않는 규약이 5곳에 손으로 복제돼 있다.

### 영향

**버그 발생 가능성 증가** · **동일한 문제의 반복** — 새 레벨을 추가하면 네트워킹이 에러 없이
조용히 죽는다. `TownMap`과 `CrashTestMap`에서 이미 그 상태다. 로그도 경고도 남지 않으므로
원인을 레벨 BP까지 따라가기 전에는 패킷 처리 코드를 먼저 의심하게 된다.

## `Room` / `DBRequestFunctions` 갓 클래스
> **심각도:** 높음 · **난이도:** 높음 · **범위:** 모듈 · server
> 위치: `Server/GameServer/Game/Room/Room.cpp` (1,161줄) · `Server/GameServer/DB/DBRequestFunctions.cpp` (1,531줄)
> 등록일: 2026년 8월 19일

`Room` 하나가 입장·퇴장·이동·전투·피격·처치·사망·보상·리스폰·채팅·셀 행렬·몬스터 스폰을 전부
들고 있다(`Room.h` 32~83줄). `DBRequestFunctions`는 캐릭터·상태·인벤토리·장비의 모든 쿼리를 한
파일에 담는다.

`Room`에는 테스트가 없다. 현재 테스트 그물은 Inventory와 프로토콜에만 있다.

### 영향

**테스트 어려움** · **변경 영향 범위 확대** — 두 파일이 서버 코드 10,009줄의 27%다. 어느 기능을
고쳐도 같은 파일을 만지므로 변경이 서로 부딪히고, 테스트 대상을 잘라내기가 불가능하다.

## 몬스터 전체 계층과 전투 로직이 블루프린트에 있다
> **심각도:** 높음 · **난이도:** 높음 · **범위:** 기능 · client
> 위치: `P1/Content/Blueprints/`
> 등록일: 2026년 8월 19일

UE 에디터로 실측한 결과는 아래 두 가지다.

`BP_MonsterBase`(부모 C++ `Monster`)가 `ReceiveBeginPlay`·`ReceiveTick`·`ReceiveActorBeginOverlap`·
`OnPostDie`를 BP로 구현한다. 그 아래 `BP_{Melee,Ranged,Super}MonsterBase` 3개와 미니언 9개가
각자 다시 같은 3개 이벤트를 구현한다. BP 클래스 13개가 몬스터 틱 로직을 나눠 갖고 있다.

`BPC_MonsterAttackSystem`과 `BPC_WarriorAttackSystem`(부모 C++ `AttackSystemComponent`)이
`S_PerformNormalAttack`·`PerformNormalAttack`·`Reset Attack Combo`·`TestAttack`을 BP로 구현하고
`NormalAttacks` 배열을 들고 있다. 콤보 상태 머신과 몽타주 선택이 전부 BP에 있다. C++
`AttackSystemComponent`는 56+52줄뿐이다.

### 영향

**버그 발생 가능성 증가** · **유지보수 어려움** — 서버가 전투를 판정하는데
(`Room::HandleNormalAttack`) 클라 판정 로직은 BP라, 양쪽 규칙이 갈라져도 컴파일러도 테스트도
잡지 못한다. 13개 BP에 흩어진 틱은 호출 순서를 추적할 수 없어 디버깅이 불가능하다.

## 리다이렉터 스텁 18개가 커밋되어 있다
> **심각도:** 중간 · **난이도:** 낮음 · **범위:** 모듈 · client
> 위치: `P1/Content/Blueprints/`
> 등록일: 2026년 8월 19일

폴더를 재편한 뒤 "Fix Up Redirectors"를 돌리지 않아 `ObjectRedirector` 에셋이 남았다.

```
Blueprints/{BP_BoundaryWall,BP_GameInstance,BP_GameMode,BP_LoginMenuMode,BP_MyPlayer,BP_Portal,BP_Shop}
Blueprints/Creatures/{BP_MyPlayer,BP_Player,BP_RangedMonster}
Blueprints/LevelObject/{BP_MonsterSpawner,WBP_NameTag,WBP_NameTag_C,Default__WBP_NameTag_C}
Blueprints/Props/{BP_BoundaryWall,BP_Portal,BP_Shop,WBP_NameTag}
```

### 영향

**유지보수 어려움** · **변경 영향 범위 확대** — 같은 이름의 BP가 콘텐츠 브라우저 세 곳에 보여
어느 것이 실물인지 알 수 없다. `BP_MyPlayer`, `BP_Portal`, `BP_Shop`, `WBP_NameTag`가 각각
세 곳에 나타난다. 스텁을 실물로 착각해 열면 빈 에셋을 편집하게 된다.

## `Server.sln` 빌드가 항상 실패한다 — 원인은 C++가 아니다
> **심각도:** 중간 · **난이도:** 낮음 · **범위:** 프로젝트 · build
> 위치: `Server/AuthServer/AuthServer.esproj`
> 등록일: 2026년 8월 19일

솔루션 전체 빌드 결과가 `buildIsSuccess: false`다. Rider의 Problems 뷰는 비어 있고 진단 출력도
없다. 빌드 로그 실물에서 확인한 유일한 에러는 아래와 같다.

```
AuthServer.esproj -> Microsoft.NuGet.targets(198,5): error :
  Your project does not reference ".NETCoreApp,Version=v6.0" framework.
```

C++ 3개 프로젝트는 정상이다. `DummyClient.cpp`·`GameServer.cpp`·`Inventory.cpp`·`JobQueue.cpp`
범위 빌드가 전부 성공했고 `Binary/Debug/DummyClient.exe`가 재생성됐다. DummyClient 빌드는
2026-08-11 커밋 `286eee9`에서 이미 복구됐고 이번 실측으로 재확인했다.

### 영향

**변경 비용 증가** · **테스트 어려움** — 매 빌드가 빨간불이라 진짜 에러가 묻힌다.
`build_solution_state`가 "실패"만 돌려주고 원인을 주지 않으므로 빌드 검증을 자동화할 수 없다.

## pre-commit 훅이 클론마다 손으로 켜야 동작한다
> **심각도:** 중간 · **난이도:** 낮음 · **범위:** 프로젝트 · build
> 위치: `.githooks/pre-commit`
> 등록일: 2026년 9월 15일

`.githooks/pre-commit`이 대용량 파일의 커밋을 막는다. 훅을 켜는 것은 `core.hooksPath` 설정이다.
이 값은 로컬 git config에 들어가므로 커밋되지 않는다. 새로 클론한 사람에게는 훅이 없는 것과 같다.

설정 방법을 적어 둔 문서도 없다. `grep -rn 'hooksPath' --include='*.md'`가 0건이다
(2026-09-15 실측).

### 영향

**부채의 연쇄 증가** · **동일한 문제의 반복** — 막으려던 대용량 파일이 히스토리에 들어간다.
되돌리려면 히스토리를 다시 써야 한다. 훅이 없는 클론에서는 경고도 실패도 남지 않으므로
누락을 알아챌 신호가 없다.

## Rider 빌드가 셸로 고친 파일을 건너뛰고 성공을 보고한다
> **심각도:** 중간 · **난이도:** 낮음 · **범위:** 프로젝트 · build
> 위치: `docs/build.md` 「게임 서버」 · `CLAUDE.md` 「도구 라우팅」
> 등록일: 2026년 9월 15일

에이전트가 셸(`sed`, 리다이렉션)로 소스를 고친 뒤 `build_solution_start`를 부르면, Rider가
바뀐 파일을 인식하지 못한 채 재컴파일 없이 `buildIsSuccess: true`를 돌려줄 때가 있다.

2026-09-15 실측 기록이다. `InventoryTests.cpp`를 23시 31분 26초에 고치고 빌드했으나 `.obj`는
23시 31분 11초에 멈춰 있었고, 그 상태로 돌린 테스트는 옛 바이너리의 결과였다. `.obj`를 지운
뒤 빌드해도 같은 증상이 재현됐다. 이 세션에서 통한 회피는 `open_file_in_editor`로 파일을 열어
가상 파일 시스템을 갱신한 뒤 다시 빌드하는 것 하나이며, 이것도 매번 통하지는 않았다.

재컴파일 여부를 판정하는 방법은 `.obj`와 실행 파일의 수정 시각을 소스와 대조하는 것이다.
빌드 결과의 `buildIsSuccess`만으로는 구별되지 않는다.

### 영향

**버그 발생 가능성 증가** · **테스트 어려움** — 「완료 기준」이 요구하는 빌드 초록과 테스트
초록이 변경을 반영하지 않은 채 나올 수 있다. 검증하지 않은 것을 검증했다고 적게 되는 경로이며,
「자기신고를 믿지 않는다」가 막으려는 상황이 도구 쪽에서 그대로 발생한다.

## 접속 정보가 3곳에 컴파일 타임 상수로 흩어져 있다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 프로젝트 · build
> 위치: `Server/GameServer/config.h` · `P1/Source/P1/P1GameInstance.h` 97~98줄 ·
> `P1/Source/P1/Login/LoginManager.h` 33~34줄
> 등록일: 2026년 8월 19일

| 위치 | 값 | 형태 |
|---|---|---|
| `Server/GameServer/config.h` | GameDB 접속 문자열, Redis URI | `#define` (gitignore됨) |
| `P1/Source/P1/P1GameInstance.h` 97~98줄 | `127.0.0.1` / `7777` | `const` 멤버 |
| `P1/Source/P1/Login/LoginManager.h` 33~34줄 | `127.0.0.1` / `5000` | 멤버 초기값 |
| `Server/AuthServer/.env` | 나머지 전부 | 유일하게 런타임 설정 |

`config.h` 방식의 실질 이점은 두 가지다. 오타가 컴파일 에러로 잡히고, 배포물에 설정 파일을
딸려 보낼 필요가 없다.

### 영향

**새 기능 개발 지연** · **변경 비용 증가** — `config.h`가 gitignore돼 있고 예제 파일도 없어 새로
클론한 사람은 빌드 자체가 안 된다. 테스트용 DB를 가리키게 할 방법이 없어 L1 이상의 통합 테스트를
막는다. 접속처를 바꾸려면 재빌드해야 한다.

## C++ 베이스 없이 BP에만 사는 UI/액터
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 기능 · client
> 위치: `P1/Content/Blueprints/`
> 등록일: 2026년 8월 19일

UE 에디터로 41개 BP의 부모 클래스를 전수 확인한 결과, 위젯 12/15는 이미 C++ 클래스로
리페어런트되어 있다. 남은 것은 아래 넷이다.

| 에셋 | 부모 | BP에 있는 것 |
|---|---|---|
| `WBP_CharacterSlot` | `UserWidget` | 함수 그래프 `UpdateCharacterInfo`·`DisableHighlight`·`Clear`, 디스패처 `OnSlotButtonClicked`, 변수 `Characterid`·`ThisSlotId` |
| `BP_Shop` | `Actor` | 오버랩 상호작용 + `PlayerController` 참조. C++에 `UShopWidget`은 있는데 상점 액터가 없다 |
| `WBP_NameTag` | `UserWidget` | `Tick`·`PreConstruct`·`Construct`. `UNameplateWidget`과 역할이 겹친다 |
| `WBP_Help` | `UserWidget` | `Tick`·`PreConstruct`·`Construct`. 순수 표시용 |

C++ 부모가 있는데도 BP 쪽 로직이 무거운 것은 아래 넷이다.

| 에셋 | 부모(C++) | BP에 남은 로직 |
|---|---|---|
| `WBP_Slot` | `SlotWidget` | 그래프 6개(`GetToolTipWidget`·`OnMouseButtonDown`·`OnMouseButtonDoubleClick` 외), 이벤트 `OnStartCooldown`·`OnUpdateCooldown`·`OnUse`, 변수 9개(`CooldownTimerHandle`·`ElapsedTime`·`IntervalTime` 외). 쿨다운 상태 머신 전체 |
| `WBP_LoginMenu` | `LoginWidget` | 그래프 4개(`CC_Init`·`DisableAllSlotsHighlight`·`ClearAllSlots`·`IsValidCharacter`) + `OnDisplayCharacterOverviews` |
| `WBP_DeathScreen` | `DeathWidget` | `Countdown`·`StartCountdown`·`ReturnToTown` + `ReturnCountdown`·`ElapsedTime`·`Timer`. 리스폰 카운트다운 |
| `BP_MyPlayer` / `BP_Player` | `P1MyPlayer` / `P1Player` | `Load and Set SK Mesh`·`Load And Set ST Mesh` + `ChangeMesh`. 장비 메시 교체 |

### 영향

**테스트 어려움** · **유지보수 어려움** — 쿨다운과 카운트다운처럼 시간과 상태를 다루는 로직이
BP에 있으면 단위 테스트가 불가능하고 Live Coding으로도 검증할 수 없다. `WBP_Slot`의 쿨다운
상태 머신과 `WBP_DeathScreen`의 리스폰 카운트다운이 여기 해당한다.

## 캐릭터 클래스와 컨트롤러에 관심사가 뭉쳐 있다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 모듈 · client
> 위치: `P1/Source/P1/Game/`
> 등록일: 2026년 8월 19일

| 클래스 | 뭉쳐 있는 것 |
|---|---|
| `ACreature` (`Game/Objects/Creature.h`, 258줄) | 이동 보간(`MoveQueue`·`CorrectionMaxThreshold`·`CORR_INTERP_SPEED`) + 어택 컴포넌트 + 네임플레이트 위젯 + 사망 상태 + `S_*` 수신 처리 |
| `AP1MyPlayer` (`Game/Objects/P1MyPlayer.h`, 126+257줄) | 카메라 붐 + Enhanced Input 액션 5종 + 이동 패킷 스로틀(`MOVE_PACKET_SEND_DELAY`·`YAW_TOLERANCE`·더티 플래그) + 전투 모드 + 디버그 카운터 |
| `AInGamePlayerController` (`Game/InGamePlayerController.h`, 124+219줄) | 위젯 7종의 `TSubclassOf`/인스턴스 쌍 + `WidgetMappings` + `WidgetFlag` 비트마스크 + `CurrentMaxZOrder` 관리 |

이동 동기화 로직이 수신(`ACreature`)과 송신(`AP1MyPlayer`) 양쪽에 갈라져 있다. 보간 상수와
스로틀 상수도 두 파일에 따로 산다.

### 영향

**변경 영향 범위 확대** · **버그 발생 가능성 증가** — 이동 동기화를 고칠 때 한쪽만 고치는 사고가
나기 쉽다. 두 파일의 상수가 어긋나도 컴파일러가 잡지 않고, 증상은 특정 지연 구간에서만
드러난다.

## CI가 없다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 프로젝트 · build
> 위치: `.github/workflows/` (빈 폴더)
> 등록일: 2026년 8월 19일

`GameServerTests`는 비밀 없이 빌드된다. gitignore된 `config.h`의 유일한 소비자인
`Main/GameServer.cpp`를 제외하므로 판정은 종료 코드 하나다. AuthServer에도 `npm test`가 생겼다.

### 영향

**버그 발생 가능성 증가** · **테스트 어려움** — 3티어 중 어느 하나만 깨져도 손으로 띄워보기
전에는 모른다. 빌드와 테스트가 이미 비밀 없이 도는데도 자동으로 돌지 않으므로, 깨진 커밋이
병합된 뒤에야 발견된다.

## 인벤토리 매핑 3종이 손으로 유지된다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 파일 · server
> 위치: `Server/GameServer/Game/System/Inventory.cpp` (생성자)
> 등록일: 2026년 8월 27일

`Inventory`는 서로 정합해야 하는 표를 셋 들고 있고, 셋 다 생성자에서 손으로 채운다.

| 표 | 방향 | 쓰는 곳 |
|---|---|---|
| `itemTypeMappings` | 아이템 데이터의 `"itemType"` 문자열 → `ItemType` | `addItem` |
| `slotTypeToItemTypeMappings` | `SlotType` → `ItemType` | `removeItem`, `GetSlot` |
| `inventorylookupMappings` | `ItemType` → 실제 슬롯 배열 | 전부 |

`Server/GameServerTests/InventoryTests.cpp`의 슬롯 타입 왕복 테스트가 세 타입을 전부 검사하므로,
표가 다시 어긋나면 테스트가 먼저 잡는다. 2026년 9월에 검토했다가 폐기한 설계가
`docs/references/work/2026-09-10-inventory-cleanup.md`에 있다.

### 영향

**버그 발생 가능성 증가** · **유지보수 어려움** — 넣을 때와 꺼낼 때가 다른 표를 본다. 두 표가 한
글자만 어긋나도 아이템이 다른 인벤토리로 샌다. 두 값 모두 유효한 enum이라 컴파일러가 아무 말도
하지 않는다.

## 에이전트의 파일 편집을 가로채는 층이 없다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 프로젝트 · build
> 위치: `.claude/settings.json` · `.claude/hooks/guard_dangerous_cmd.py`
> 등록일: 2026년 9월 15일

`guard_dangerous_cmd.py`가 위험한 명령을 차단한다. 이 훅을 부르는 `PreToolUse` 매처는 둘이다.

| 매처 | 덮는 것 |
|---|---|
| `Bash\|PowerShell` | 셸 명령 |
| Rider MCP 툴 21종 | `apply_patch`, `create_new_file`, `rename_refactoring`, `safe_delete` 등 |

두 매처 어디에도 내장 편집 도구인 `Edit`, `Write`, `NotebookEdit`가 없다(2026년 9월 15일 실측).
Rider를 거친 편집은 걸리지만 에이전트가 파일을 직접 고치는 경로는 걸리지 않는다.

CLAUDE.md 「안전」이 "파일 편집에는 셸을 거치지 않는 편집 도구를 쓴다"고 지시하므로, 권장 경로가
곧 검사받지 않는 경로다.

### 영향

**버그 발생 가능성 증가** · **동일한 문제의 반복** — 파일을 어디까지 고쳐도 되는지가 모델의 준수에만
달려 있다. 훅이 덮는 셸 경로와 덮지 않는 편집 경로 사이에 안전 수준의 단차가 생기고, 그 단차는
에이전트가 권장 경로를 쓸수록 커진다.

## `UP1GameInstance`가 클라 측 갓 클래스
> **심각도:** 중간 · **난이도:** 높음 · **범위:** 모듈 · client
> 위치: `P1/Source/P1/P1GameInstance.cpp` (620줄)
> 등록일: 2026년 8월 19일

소켓 소유 + 세션 관리 + `S_*` 핸들러 16개 + 스폰/디스폰 + 델리게이트 5종 브로드캐스트 + 토큰
보관을 한 클래스가 들고 있다.

### 영향

**변경 영향 범위 확대** · **테스트 어려움** — 게임 인스턴스는 레벨 전환에 살아남는 싱글턴이라
여기 붙은 모든 것이 전역 상태가 된다. 핸들러 하나를 고치려 해도 소켓 수명과 델리게이트 구독을
함께 따져야 한다.

## 스택 상한 없는 아이템 누적
> **심각도:** 중간 · **난이도:** 높음 · **범위:** 함수 · protocol
> 위치: `Server/GameServer/Game/System/Inventory.cpp` 166줄
> 등록일: 2026년 8월 19일

`findFirstAvailableSlotId`는 비장비 아이템에서 같은 `template_id` 슬롯을 찾으면 무조건 거기
합친다. 아이템 데이터의 최대 스택 수를 보지 않는다. `JsonProperty::Item::MaxStack`은 선언만
돼 있고 서버 코드 어디에서도 한 번도 읽히지 않는다(실측).

난이도가 높은 이유는 수정 범위가 함수 밖으로 나가기 때문이다. 초과분을 다음 슬롯으로 넘기면
한 번의 구매가 슬롯 두 개를 바꾸는데, `S_BUY_ITEM`은 `Slot updated_slot` 하나만 나른다
(`P1/Source/P1/Network/Protocol.proto` 168~173줄). 서버가 두 번째 슬롯을 클라에 알릴 방법이
없다.

재현 테스트는 `InventoryTest.DISABLED_StackDoesNotExceedMaxStack`에 있다. `DISABLED_`로 둔
이유는 이 범위 문제 때문이지 버그가 아니어서가 아니다.

### 영향

**새 기능 개발 지연** · **버그 발생 가능성 증가** — `maxStack: 10`인 소모품을 15개 구매하면 한
슬롯에 15개가 쌓인다. `--gtest_also_run_disabled_tests`로 실행해 빨강임을 확인했다. 스택 상한을
전제하는 기능(거래, 창고, 제작)은 이 상태 위에 올릴 수 없다.

## `Users.user_id INT` vs `Characters.user_id BIGINT`
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 모듈 · ops
> 위치: `Server/Queries/UserDB_CreateUsersTable.sql` 7줄 ·
> `Server/GameServer/Queries/GameDB_CreateAllTables.sql` 11줄
> 등록일: 2026년 8월 19일

두 티어가 같은 개념을 다른 폭으로 저장한다. `GameSession::userId`는 `int64`다.

### 영향

**버그 발생 가능성 증가** — 사용자 수가 `INT` 상한(2,147,483,647)에 닿으면 두 DB가 같은 사용자를
다른 값으로 보게 된다. 지금은 값이 작아 드러나지 않는다.

## AuthServer의 eslint가 연결돼 있지 않다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 모듈 · build
> 위치: `Server/AuthServer/package.json`
> 등록일: 2026년 8월 19일

`eslint@^9.31.0`이 `devDependencies`에 있지만 flat config(`eslint.config.js`)도 `scripts.lint`도
없다. 설치만 되고 한 번도 실행되지 않는다.

### 영향

**버그 발생 가능성 증가** — AuthServer 코드에 정적 검사가 사실상 없다. 미사용 변수, 잘못된 import
경로, 오타가 런타임까지 살아서 간다.

## 생성기가 만들지 않는 게임 데이터 JSON 2개
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 모듈 · shared
> 위치: `P1/Content/Gamedata/C_Equipment.json` · `P1/Content/Gamedata/C_Gear.json`
> 등록일: 2026년 8월 19일

`GenJsonFile.bat`의 `MOVE` 목록에 없다. 생성물은 `C_{Item,Map,Monster,Quest}.json` 4개뿐이다.
손으로 만든 것인지 이전 버전 생성기의 잔재인지 불명이다.

### 영향

**유지보수 어려움** — 엑셀 원본을 고쳐도 이 두 파일은 갱신되지 않는다. 쓰이고 있다면 데이터가
조용히 어긋나고, 안 쓰이고 있다면 어느 쪽이 진짜인지 매번 확인해야 한다.

## `WBP_Nameplate_Old` 데드 에셋
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · client
> 위치: `P1/Content/Blueprints/UI/InGame/WBP_Nameplate_Old.uasset`
> 등록일: 2026년 8월 19일

부모는 `NameplateWidget`인데 구현된 이벤트도 변수도 없다(에디터 실측).

### 영향

**유지보수 어려움** — 이름이 `_Old`라 현행 네임플레이트 위젯과 헷갈린다. 빈 에셋이라 열어봐도
용도를 알 수 없어, 지워도 되는지 판단하려면 참조처를 매번 다시 확인해야 한다.

## 이슈 본문 전달 방식을 두 문서가 다르게 지시한다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · build
> 위치: `docs/agents/issue-tracker.md` 7·10·45줄 · `CLAUDE.md` 91·142줄
> 등록일: 2026년 9월 15일

`docs/agents/issue-tracker.md:7`은 여러 줄 본문에 heredoc과 `--body`를 쓰라고 지시한다.

> **Create an issue**: `gh issue create --title "..." --body "..."`. Use a heredoc for multi-line bodies.

`CLAUDE.md:91`은 그 반대를 지시한다. heredoc은 본문을 명령 문자열에 넣어 `guard_dangerous_cmd.py`가
보게 되므로, 위험 패턴을 언급만 하는 본문도 차단된다는 이유다. 같은 이유로 `CLAUDE.md:142`가
`gh pr create`에 `--body-file`을 쓰라고 못박았는데, 이슈에는 같은 지시가 없다.

고치기 전에 결정할 것이 하나 있다. `docs/agents/` 세 파일은 스킬 본문과 문구를 맞추려고 시드 템플릿
원문을 그대로 두기로 했다(커밋 `6b0bafe`). 문구를 고치면 그 결정이 깨진다.

### 영향

**동일한 문제의 반복** — 지시를 따른 에이전트가 훅에 막힌다. 막힌 뒤 해결책을 매번 다시 찾아야 하고,
`--body`로 여러 줄을 넣으면 줄바꿈과 백틱이 깨진다.

## 빌드 문서의 솔루션 프로젝트 개수가 낡았다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · build
> 위치: `docs/build.md` 21줄
> 등록일: 2026년 9월 15일

`docs/build.md:21`이 `Server.sln`에 "C++ 3개(`ServerCore` · `GameServer` · `DummyClient`)와 파이썬
생성기 2개"가 들어 있다고 적는다. 실제로는 C++가 4개다. `GameServerTests`가 빠져 있다
(`Server/Server.sln` 6·8·10·12줄). 파이썬 생성기 2개는 맞다.

같은 문서의 「게임 서버」 절이 빌드 순서를 설명하면서도 테스트 프로젝트를 언급하지 않는다.

### 영향

**변경 비용 증가** — 빌드 구성을 건드릴 때 읽는 문서가 테스트 프로젝트의 존재를 감춘다. 테스트
프로젝트는 `.cpp`를 자동 수집하지 않아 등록을 빠뜨리면 테스트가 조용히 안 돌아가는데, 그 사실은
`docs/testing.md`에만 적혀 있다.
