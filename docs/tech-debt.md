# Tech Debt

세션 1(2026-08-19) 진단. 모든 항목은 실제 코드·라이브 DB·UE 에디터 실측 근거를 갖는다.
수정은 `docs/plans/`에 계획을 세운 뒤에만 한다.

표기: `[심각도: 상/중/하] [수정 난이도: 상/중/하]`

**갱신 이력**
- 세션 2(2026-08-27, `docs/plans/completed/verification-infra.md`) — L1 테스트 인프라를 세우고
  **D-01·D-15 수정**(둘 다 실패 테스트 선행). D-14는 재조사로 난이도 상향(프로토콜 변경 필요),
  D-22·D-23 신규 추가. D-05는 세션 1에서 수정됨.

---

## 상 — 먼저 볼 것

### D-01. 인벤토리 슬롯 타입 매핑 오타 — 기타 아이템이 장비 테이블을 본다 *(세션 2에서 수정 완료)*
`[심각도: 상] [난이도: 하]` · `Server/GameServer/Game/System/Inventory.cpp:44`

```cpp
{Protocol::SlotType::SLOT_TYPE_INVENTORY_MISC, Protocol::ItemType::ITEM_TYPE_GEAR}
//                                             ^^^^^^^^^^^^^^^^^^ MISCELLANEOUS 여야 한다
```

- **왜 문제였나** — `removeItem`과 `GetSlot`이 이 표로 대상 테이블을 고르므로,
  기타 아이템 슬롯을 지우거나 조회하면 **엉뚱하게 장비 인벤토리를 건드렸다**.
- **어떻게 고쳤나** — 값을 `ITEM_TYPE_MISCELLANEOUS`로 수정. TDD로 진행했다 —
  `Server/GameServerTests/InventoryTests.cpp`에 세 슬롯 타입 왕복 테스트를 먼저 쓰고
  **MISC 케이스 3개만 빨강인 것을 확인한 뒤** 고쳤다. 특히
  `RemovingMiscItemDoesNotTouchGearInventory`가 이 항목이 서술한 피해(기타 슬롯을 지웠는데
  장비가 사라짐)를 그대로 재현했다. 수정 후 전체 통과.
- **남은 것** — 아래 D-23. 오타는 증상이고, 손으로 쓴 매핑 3종이 어긋날 수 있는 구조가 원인이다.

### D-02. 네트워크 수신 펌프가 레벨 블루프린트에 있다
`[심각도: 상] [난이도: 중]` · `P1/Source/P1/P1GameInstance.cpp:97` · `P1/Content/Maps/*.umap`

`UP1GameInstance::HandleRecvPackets()`를 호출하는 **C++ 코드가 없다**. 호출부는 레벨 스크립트
블루프린트의 `ReceiveTick` 안이고, 5개 맵 중 3개에만 있다 (UE 에디터 실측).

| 맵 | 레벨 BP | 펌프 |
|---|---|---|
| `InGameMap` `LoginMap` `TestMap` | 있음 | ✅ |
| `TownMap` `CrashTestMap` | 레벨 스크립트 BP 자체가 없음 | ❌ |

- **왜 문제인가** — 새 레벨을 추가하면 네트워킹이 **에러 없이 조용히 죽는다**. 프레임워크가
  강제하지 않는 규약이 5곳에 손으로 복제돼 있다.
- **어디로 갈 것인가** — `UP1GameInstance::Init()`에서 `FTSTicker`(또는 `UTickableWorldSubsystem`)로
  펌프를 자기 자신이 돌리게 하고, 레벨 BP의 호출을 제거한다. 레벨과 무관하게 항상 도는 게 맞다.

### D-03. `Room` / `DBRequestFunctions` 갓 클래스
`[심각도: 상] [난이도: 상]` · `Server/GameServer/Game/Room/Room.cpp` (1,161줄) ·
`Server/GameServer/DB/DBRequestFunctions.cpp` (1,531줄)

`Room` 하나가 입장·퇴장·이동·전투·피격·처치·사망·보상·리스폰·채팅·셀 행렬·몬스터 스폰을 전부 들고 있다
(`Room.h:32-83`). `DBRequestFunctions`는 캐릭터·상태·인벤토리·장비의 모든 쿼리를 한 파일에 담는다.

- **왜 문제인가** — 두 파일이 서버 코드(10,009줄)의 **27%**다. 어느 기능을 고쳐도 같은 파일을 만지므로
  변경이 서로 부딪히고, 테스트 대상을 잘라내기가 불가능하다.
- **어디로 갈 것인가** — `Room`에서 먼저 **셀 행렬(공간 분할)** 을 별도 타입으로 떼고, 그다음
  전투 판정을 `CombatSystem`으로 분리한다. `DBRequestFunctions`는 애그리게이트 단위
  (`CharacterRepository` / `InventoryRepository`)로 쪼갠다. 잡 큐 규약은 유지.

### D-04. 몬스터 전체 계층과 전투 로직이 블루프린트에 있다
`[심각도: 상] [난이도: 상]` · `P1/Content/Blueprints/Creatures/Monster/**` ·
`P1/Content/Blueprints/Components/BPC_*AttackSystem.uasset`

UE 에디터 실측 결과:

- `BP_MonsterBase`(부모 C++ `Monster`)가 `ReceiveBeginPlay` / `ReceiveTick` /
  `ReceiveActorBeginOverlap` / `OnPostDie`를 **BP로** 구현하고, 그 아래
  `BP_{Melee,Ranged,Super}MonsterBase` 3개 + 미니언 9개가 **각자 다시** 같은 3개 이벤트를 구현한다.
  → BP 클래스 **13개**가 몬스터 틱 로직을 나눠 갖고 있다.
- `BPC_MonsterAttackSystem` / `BPC_WarriorAttackSystem`(부모 C++ `AttackSystemComponent`)이
  `S_PerformNormalAttack` · `PerformNormalAttack` · `Reset Attack Combo` · `TestAttack`을 BP로 구현하고
  `NormalAttacks` 배열을 들고 있다. → **콤보 상태 머신과 몽타주 선택이 전부 BP에 있다.**
  C++ `AttackSystemComponent`는 56+52줄뿐이다.

- **왜 문제인가** — 서버가 전투를 판정하는데(`Room::HandleNormalAttack`) 클라 판정 로직은 BP라,
  양쪽 규칙이 갈라져도 컴파일러도 테스트도 잡지 못한다. 13개 BP에 흩어진 틱은 디버깅 불가능하다.
- **어디로 갈 것인가** — 콤보/몽타주 선택을 `UAttackSystemComponent`(C++)로 올리고 BP는 데이터
  (몽타주 에셋 배열)만 남긴다. 몬스터는 `AMonster`에 공통 틱을 구현하고 BP 하위 클래스는
  메시·스탯 데이터만 갖게 한다.

### D-05. PreToolUse 훅이 안전 규칙을 강제하지 못했다 *(세션 1에서 수정 완료)*
`[심각도: 상] [난이도: 하]` · `.claude/settings.json` · `.claude/hooks/guard_dangerous_cmd.py`

- **무엇이었나** — 훅이 `python3 -c "..."` 인라인이었는데 이 Windows에서 `python3`는
  MS Store 앱 실행 별칭 스텁이라 스크립트를 실행하지 않고 **exit 49**로 죽었다. PreToolUse는
  **exit 2만 차단**이고 그 외 non-zero는 non-blocking error다. 실측 결과 `rm -rf`가 그대로 실행됐다.
  추가로 matcher가 `"Bash"`뿐이라 `PowerShell` 툴과 Rider MCP(`execute_terminal_command`,
  `execute_sql_query`)는 애초에 검사 대상이 아니었고, 패턴에 `TRUNCATE`·WHERE 없는
  `DELETE`/`UPDATE`·`KEYS *`가 빠져 CLAUDE.md 안전 규칙과 불일치했다.
- **어떻게 고쳤나** — 로직을 `.claude/hooks/guard_dangerous_cmd.py`로 분리하고 `py -3`(실 exe)로
  exec form 실행. matcher 2블록으로 셸 2종 + MCP 2종을 덮고, `permissionDecision=deny` + exit 2.
  단위테스트 27/27 통과, Bash·PowerShell·MCP SQL 3경로 실차단 확인.
- **남은 것** — `mcp__rider__ue_execute_python`(에디터 임의 파이썬)은 아직 훅 밖이다.

---

## 중

### D-06. `UP1GameInstance`가 클라 측 갓 클래스
`[심각도: 중] [난이도: 상]` · `P1/Source/P1/P1GameInstance.cpp` (620줄)

소켓 소유 + 세션 관리 + `S_*` 핸들러 16개 + 스폰/디스폰 + 델리게이트 5종 브로드캐스트 + 토큰 보관.

- **왜 문제인가** — 게임 인스턴스는 레벨 전환에 살아남는 싱글턴이라 여기 붙은 모든 것이 전역 상태가 된다.
- **어디로 갈 것인가** — 전송/수신은 `UNetworkSubsystem`, 오브젝트 스폰은 기존
  `UStatefulObjectManager`로 옮기고, `UP1GameInstance`는 소유권만 갖게 한다.

### D-07. 패킷 핸들러 2개가 룸 잡 큐를 우회한다
`[심각도: 중] [난이도: 중]` · `Server/GameServer/Main/ServerPacketHandler.cpp:125`, `:216-225`

`Handle_C_ENTER_GAME`은 `ObjectUtils::CreatePlayer()`를 IOCP 워커에서 인라인 호출하고,
`Handle_C_ENTER_MAP`은 `player->OnEnterMap()` + `SEND_PACKET`을 룸 큐 밖에서 실행한다.

- **왜 문제인가** — "룸 소유 상태는 큐 위에서 직렬화되므로 락이 없다"는 전제가 이 두 곳에서 깨진다.
  경합이 나면 재현이 극도로 어렵다.
- **어디로 갈 것인가** — `Room::C_HandleEnterMap` / `C_HandleEnterGame`을 만들어 `DoAsync`로 넘긴다.

### D-08. 접속 정보가 3곳에 컴파일 타임 상수로 흩어져 있다
`[심각도: 중] [난이도: 중]`

| 위치 | 값 | 형태 |
|---|---|---|
| `Server/GameServer/config.h` | GameDB 접속 문자열, Redis URI | `#define` (gitignore됨) |
| `P1/Source/P1/P1GameInstance.h:97-98` | `127.0.0.1` / `7777` | `const` 멤버 |
| `P1/Source/P1/Login/LoginManager.h:33-34` | `127.0.0.1` / `5000` | 멤버 초기값 |
| `Server/AuthServer/.env` | 나머지 전부 | 유일하게 런타임 설정 |

- **트레이드오프 평가** — `config.h` 방식의 실질 이점은 두 가지다. (1) 오타가 **컴파일 에러**로 잡힌다.
  (2) 배포물에 설정 파일을 딸려 보낼 필요가 없다. 대가는 (a) 접속처를 바꾸려면 **재빌드**해야 하고,
  (b) 파일이 gitignore돼 있어 **새로 클론한 사람은 빌드 자체가 안 되며**(예제 파일도 없다),
  (c) 테스트용 DB를 가리키게 할 방법이 없어 **L1 이상의 통합 테스트를 막는다**.
  포트폴리오 단계에서 (a)는 비용이 작지만 (b)와 (c)는 곧 비용이 된다.
- **어디로 갈 것인가** — 전면 환경변수화는 과하다. **`config.h.example`을 추적하고 `config.h`는 계속
  ignore**하는 것이 최소 조치다(b 해결). 그다음 세션 2에서 테스트가 필요해지면 그때
  `GameServerConfig` 구조체 + 환경변수 오버라이드를 넣는다(c 해결). 클라 쪽 2곳은
  `DefaultGame.ini`의 `[/Script/P1.P1GameInstance]` 섹션으로 옮기면 재빌드 없이 바뀐다.

### D-09. 리다이렉터 스텁 18개가 커밋되어 있다
`[심각도: 중] [난이도: 하]` · `P1/Content/Blueprints/**`

폴더 재편 후 "Fix Up Redirectors"를 돌리지 않아 남은 `ObjectRedirector` 에셋:

```
Blueprints/{BP_BoundaryWall,BP_GameInstance,BP_GameMode,BP_LoginMenuMode,BP_MyPlayer,BP_Portal,BP_Shop}
Blueprints/Creatures/{BP_MyPlayer,BP_Player,BP_RangedMonster}
Blueprints/LevelObject/{BP_MonsterSpawner,WBP_NameTag,WBP_NameTag_C,Default__WBP_NameTag_C}
Blueprints/Props/{BP_BoundaryWall,BP_Portal,BP_Shop,WBP_NameTag}
```

- **왜 문제인가** — 같은 이름의 BP가 3곳에 보여서 어느 게 실물인지 알 수 없다. 실제로
  `BP_MyPlayer`는 3곳, `BP_Portal`·`BP_Shop`·`WBP_NameTag`는 각각 3곳에 나타난다.
- **어디로 갈 것인가** — 콘텐츠 브라우저에서 `Blueprints` 폴더 우클릭 →
  `Fix Up Redirectors in Folder` 실행 후 스텁 삭제, 한 커밋으로 정리.

### D-10. C++ 베이스 없이 BP에만 사는 UI/액터
`[심각도: 중] [난이도: 중]`

UE 에디터로 41개 BP의 부모 클래스를 전수 확인한 결과, 대부분(위젯 12/15)은 이미 C++ 클래스로
리페어런트되어 있다. 남은 것:

| 에셋 | 부모 | BP에 있는 것 | 이관 우선순위 |
|---|---|---|---|
| `WBP_CharacterSlot` | `UserWidget` | 함수 그래프 `UpdateCharacterInfo`·`DisableHighlight`·`Clear`, 디스패처 `OnSlotButtonClicked`, 변수 `Characterid`·`ThisSlotId` | **1** — 캐릭터 선택 로직 |
| `BP_Shop` | `Actor` | 오버랩 상호작용 + `PlayerController` 참조 | **2** — C++에 `UShopWidget`은 있는데 상점 액터가 없다 |
| `WBP_NameTag` | `UserWidget` | `Tick`·`PreConstruct`·`Construct` | 3 — `UNameplateWidget`과 역할 중복. 통합 후보 |
| `WBP_Help` | `UserWidget` | `Tick`·`PreConstruct`·`Construct` | 4 — 순수 표시용, 남겨도 무방 |

C++ 부모가 있는데도 BP 쪽 로직이 무거운 것:

| 에셋 | 부모(C++) | BP에 남은 로직 |
|---|---|---|
| `WBP_Slot` | `SlotWidget` | 그래프 6개(`GetToolTipWidget`·`OnMouseButtonDown`·`OnMouseButtonDoubleClick`…), 이벤트 `OnStartCooldown`·`OnUpdateCooldown`·`OnUse`, 변수 9개(`CooldownTimerHandle`·`ElapsedTime`·`IntervalTime`) → **쿨다운 상태 머신 전체** |
| `WBP_LoginMenu` | `LoginWidget` | 그래프 4개(`CC_Init`·`DisableAllSlotsHighlight`·`ClearAllSlots`·`IsValidCharacter`) + `OnDisplayCharacterOverviews` |
| `WBP_DeathScreen` | `DeathWidget` | `Countdown`·`StartCountdown`·`ReturnToTown` + `ReturnCountdown`·`ElapsedTime`·`Timer` → **리스폰 카운트다운** |
| `BP_MyPlayer` / `BP_Player` | `P1MyPlayer` / `P1Player` | `Load and Set SK Mesh`·`Load And Set ST Mesh` + `ChangeMesh` → **장비 메시 교체** |

- **왜 문제인가** — 쿨다운·카운트다운처럼 **시간과 상태를 다루는 로직**이 BP에 있으면 단위 테스트가
  불가능하고, Live Coding으로도 검증할 수 없다.
- **어디로 갈 것인가** — 시간/상태를 가진 것부터 C++로 올린다: `WBP_Slot` 쿨다운 →
  `WBP_DeathScreen` 카운트다운 → `WBP_CharacterSlot` 선택 로직. 메시 교체와 순수 표시 로직은 BP에 둔다.

### D-11. 캐릭터 클래스와 컨트롤러에 관심사가 뭉쳐 있다
`[심각도: 중] [난이도: 중]`

| 클래스 | 뭉쳐 있는 것 | 분리 후보 |
|---|---|---|
| `ACreature` (`Game/Objects/Creature.h`, 258줄) | 이동 보간(`MoveQueue`·`CorrectionMaxThreshold`·`CORR_INTERP_SPEED`) + 어택 컴포넌트 + 네임플레이트 위젯 + 사망 상태 + `S_*` 수신 처리 | `UNetMovementSyncComponent`(보간+큐), `UNameplateComponent` |
| `AP1MyPlayer` (126+257줄) | 카메라 붐 + Enhanced Input 액션 5종 + 이동 패킷 스로틀(`MOVE_PACKET_SEND_DELAY`·`YAW_TOLERANCE`·더티 플래그) + 전투 모드 + 디버그 카운터(`Activate`·`SendCounter`·`TotalSecond`) | `UPlayerInputComponent`, 송신 스로틀은 `UNetMovementSyncComponent`로 합류 |
| `AInGamePlayerController` (124+219줄) | 위젯 7종의 `TSubclassOf`/인스턴스 쌍 + `WidgetMappings` + `WidgetFlag` 비트마스크 + `CurrentMaxZOrder` 관리 | `UUIManagerSubsystem` 또는 `UHUDLayerComponent` |

- **왜 문제인가** — 이동 동기화 로직이 `ACreature`(수신)와 `AP1MyPlayer`(송신) 양쪽에 갈라져 있어
  한쪽만 고치는 사고가 나기 쉽다.
- **어디로 갈 것인가** — 송수신을 한 컴포넌트로 합치는 것부터. 그러면 스로틀 상수와 보간 상수가
  한 파일에 모인다.

### D-12. CI가 없다
`[심각도: 중] [난이도: 중]` · `.github/workflows/` (빈 디렉터리)

- **왜 문제인가** — 3티어 중 어느 하나만 깨져도 손으로 띄워보기 전에는 모른다.
- **어디로 갈 것인가** — 세션 2에서 L1 테스트가 생긴 뒤에 붙인다. 순서는
  ServerCore/GameServer 빌드 → GoogleTest 실행 → AuthServer `npm ci` + 기동 스모크.
- **세션 2 진행분** — 전제 조건이 갖춰졌다. `GameServerTests`는 **비밀 없이 빌드된다**
  (gitignore된 `config.h`의 유일한 소비자인 `Main/GameServer.cpp`를 제외하므로),
  판정은 종료 코드 하나다. AuthServer도 `npm test`가 생겼다. 남은 건 워크플로 작성뿐.

### D-13. `Server.sln` 빌드가 항상 실패한다 — 원인은 C++가 아니다
`[심각도: 중] [난이도: 하]` · `Server/AuthServer/AuthServer.esproj`

솔루션 전체 빌드 결과 `buildIsSuccess: false`. Rider의 Problems 뷰는 비어 있고 진단 출력도 없다.
빌드 로그 실물에서 확인한 유일한 에러:

```
AuthServer.esproj -> Microsoft.NuGet.targets(198,5): error :
  Your project does not reference ".NETCoreApp,Version=v6.0" framework.
```

- C++ 3개 프로젝트는 정상이다. 실측: `DummyClient.cpp`·`GameServer.cpp`·`Inventory.cpp`·
  `JobQueue.cpp` 범위 빌드 전부 성공, `Binary/Debug/DummyClient.exe`가 이번 빌드로 재생성됨.
  (DummyClient 빌드는 2026-08-11 커밋 `286eee9`에서 이미 복구됐다 — 이번 실측으로 재확인.)
- **왜 문제인가** — 매 빌드가 빨간불이라 진짜 에러가 묻힌다. `build_solution_state`가
  "실패"만 돌려주고 원인을 안 주므로 하네스가 빌드 검증을 못 한다.
- **어디로 갈 것인가** — Node 프로젝트를 MSBuild 솔루션에 넣을 이유가 없다.
  `AuthServer.esproj`를 솔루션에서 제거하고 `npm start`로만 다룬다.
  (`.pyproj` 2개도 같은 이유로 검토 대상.)

### D-14. 스택 상한 없는 아이템 누적 *(세션 2에서 재조사 — 난이도 상향, 프로토콜 변경 필요)*
`[심각도: 중] [난이도: ~~중~~ → **상**]` · `Server/GameServer/Game/System/Inventory.cpp`

`findFirstAvailableSlotId`는 비장비 아이템에서 같은 `template_id` 슬롯을 찾으면 무조건 거기 합친다.
아이템 데이터의 최대 스택 수를 보지 않는다. `JsonProperty::Item::MaxStack`은 선언만 돼 있고
서버 코드 어디에서도 **한 번도 읽히지 않는다**(실측).

- **재현** — `InventoryTest.DISABLED_StackDoesNotExceedMaxStack`.
  `maxStack: 10`인 소모품을 15개 구매하면 한 슬롯에 15개가 쌓인다. 실행해서 빨강임을 확인했다
  (`--gtest_also_run_disabled_tests`). **`DISABLED_`로 둔 이유는 아래 범위 문제 때문이지
  버그가 아니어서가 아니다.**
- **왜 난이도가 올라갔나** — 초과분을 다음 슬롯으로 넘기면 **한 번의 구매가 슬롯 두 개를 바꾼다.**
  그런데 `S_BUY_ITEM`은 `Slot updated_slot` **하나만** 나른다(`Protocol.proto:168-173`).
  즉 서버가 두 번째 슬롯을 클라에 알릴 방법이 없다.
- **어디로 갈 것인가** — 세 곳을 함께 고쳐야 한다:
  (1) `Protocol.proto`의 `S_BUY_ITEM`(및 같은 성질의 응답)을 `repeated Slot`으로,
  (2) `GenPackets.bat` 재실행,
  (3) `Inventory::addItem`에 초과분 분할 루프 + 서버·클라 핸들러.
  `DISABLED_`를 떼는 것이 완료 신호다.

---

## 하

### D-15. `removeItem`이 검증 전에 더티 플래그를 세운다 *(세션 2에서 수정 완료 — 더 큰 버그가 같이 나왔다)*
`[심각도: 하] [난이도: 하]` · `Server/GameServer/Game/System/Inventory.cpp`

`dirtyFlagsMappings[...] = true`가 조기 반환보다 위에 있어서 실패한 제거도 슬롯을 더티로 만들었다.
테스트를 붙이는 과정에서 **같은 자리에 더 심각한 문제가 하나 더 있었다**:

```cpp
Protocol::Item* item = updatedSlot->mutable_item();   // ← 검증보다 먼저
...
if (updatedSlot->has_item() == false || item->count() < count)
    return false;
```

protobuf의 `mutable_item()`은 **없던 필드를 만들면서 `has_item()`을 켠다.** 그래서 빈 슬롯에
제거를 한 번 시도하기만 해도 그 슬롯은 count 0짜리 빈 아이템에 점유되고,
`findFirstAvailableSlotId`가 `has_item()==false`로 빈 슬롯을 찾으므로 **다시는 채워지지 않는다.**
`has_item()` 검사는 항상 참이 되어 사실상 죽은 코드였다.

- **어떻게 고쳤나** — 검증을 맨 앞으로 올리고(`updatedSlot->item().count()`로 읽기만 함),
  더티 플래그와 `mutable_item()`을 그 뒤로 옮겼다. 한 번의 재구조화로 둘 다 해소된다.
- **어떻게 드러났나** — `InventoryTest.FailedRemoveLeavesSlotUsable`.
  빈 슬롯 제거 실패 후 아이템을 넣었더니 `slot_id`가 0이 아니라 **1**로 나왔다.
  코드를 눈으로 읽어서는 안 나왔을 항목이다.

### D-16. `Users.user_id INT` vs `Characters.user_id BIGINT`
`[심각도: 하] [난이도: 하]` · `Server/Queries/UserDB_CreateUsersTable.sql:7` ·
`Server/GameServer/Queries/GameDB_CreateAllTables.sql:11`

두 티어가 같은 개념을 다른 폭으로 저장한다(`GameSession::userId`는 `int64`).
지금은 값이 작아 드러나지 않는다. → `Users.user_id`를 `BIGINT`로 맞춘다(스키마 변경이라 사람 승인 필요).

### D-17. DB가 서로 다른 LocalDB 인스턴스 2개에 분산
`[심각도: 하] [난이도: 하]`

`GameDB`는 `(localdb)\ProjectModels`, `UserDB`는 `(localdb)\MSSQLLocalDB`.

- **판정** — 우연으로 보인다. AuthServer가 기본 인스턴스를, GameServer가 별도로 만든 인스턴스를 쓴다.
  다만 **결과적으로는 "게임 서버가 UserDB에 접근하지 않는다"는 아키텍처 규칙을 물리적으로 강제**하고
  있어 우연이 규칙을 지켜주는 상태다.
- **어디로 갈 것인가** — 합치지 말 것. 대신 이 분리가 **의도임을 문서로 못 박고**
  (`docs/ARCHITECTURE.md`에 반영 완료) 인스턴스 이름을 각 설정에 주석으로 남긴다.

### D-18. AuthServer의 eslint가 연결돼 있지 않다
`[심각도: 하] [난이도: 하]` · `Server/AuthServer/package.json`

`eslint@^9.31.0`이 `devDependencies`에 있지만 flat config(`eslint.config.js`)도
`scripts.lint`도 없다. 설치만 되고 한 번도 실행되지 않는다.
→ `eslint.config.js`(flat) + `"lint": "eslint src"` 추가. ESM 프로젝트이므로 `languageOptions.sourceType: "module"` 필요.

### D-19. 생성기가 만들지 않는 게임 데이터 JSON 2개
`[심각도: 하] [난이도: 하]` · `P1/Content/Gamedata/C_Equipment.json`, `C_Gear.json`

`GenJsonFile.bat`의 `MOVE` 목록에 없다(생성물은 `C_{Item,Map,Monster,Quest}.json` 4개뿐).
손으로 만든 것인지, 이전 버전 생성기의 잔재인지 불명.
→ 참조 여부를 확인해 쓰이면 엑셀 원본으로 편입하고, 안 쓰이면 삭제.

### D-20. `WBP_Nameplate_Old` 데드 에셋
`[심각도: 하] [난이도: 하]` · `P1/Content/Blueprints/UI/InGame/WBP_Nameplate_Old.uasset`

부모는 `NameplateWidget`인데 구현된 이벤트도 변수도 없다(에디터 실측). 참조처 확인 후 삭제.

### D-22. `Inventory`가 검증 없는 인덱싱으로 널 역참조에 열려 있다
`[심각도: 중] [난이도: 하]` · `Server/GameServer/Game/System/Inventory.cpp` (`removeItem`, `GetSlot`)
*(세션 2 발견 — 기록만. 즉시 고치지 않았다)*

```cpp
Protocol::ItemType itemType = slotTypeToItemTypeMappings[requestSlot.type()];  // unordered_map::operator[]
Protocol::Slot* updatedSlot = inventorylookupMappings[itemType]->Mutable(slotId);
```

두 군데가 겹쳐 있다.

1. `operator[]`는 **없는 키를 조회하면 기본값을 삽입한다.** 클라가 `SLOT_TYPE_EQUIPPED`나
   `SLOT_TYPE_QUICK`처럼 이 표에 없는 슬롯 타입을 보내면 `ITEM_TYPE_NONE`(0)이 반환되고,
   `inventorylookupMappings[ITEM_TYPE_NONE]` 역시 없는 키라 **nullptr**이 나온다.
   그 다음 줄의 `->Mutable(...)`이 널 역참조다.
2. `slotId`에 **범위 검사가 없다.** `MAX_SLOTS`(32) 밖의 값이 오면 `Mutable`이 범위를 벗어난다.

- **왜 문제인가** — 둘 다 **클라가 보낸 값**으로 직행한다. 정상 클라는 안 보내지만, 신뢰 경계는
  거기가 아니다. 서버가 죽는다.
- **어디로 갈 것인가** — `find()` 기반 조회 + `slotId` 범위 검사로 조기 반환. D-23과 함께 처리하면
  한 번에 정리된다.
- **왜 지금 안 고쳤나** — 이 세션의 계획은 D-01·D-15까지였다. "발견한 부채는 즉시 고치지 말고 기록".

### D-23. 인벤토리 매핑 3종이 손으로 유지된다 — D-01의 근본 원인
`[심각도: 중] [난이도: 중]` · `Server/GameServer/Game/System/Inventory.cpp` 생성자
*(세션 2에 D-01을 고치며 명시화. 구조는 그대로 남았다)*

`Inventory`는 서로 정합해야 하는 표를 셋 들고 있고, 셋 다 생성자에서 손으로 채운다.

| 표 | 방향 | 쓰는 곳 |
|---|---|---|
| `itemTypeMappings` | 아이템 데이터의 `"itemType"` 문자열 → `ItemType` | `addItem` |
| `slotTypeToItemTypeMappings` | `SlotType` → `ItemType` | `removeItem`, `GetSlot` |
| `inventorylookupMappings` | `ItemType` → 실제 슬롯 배열 | 전부 |

- **왜 문제인가** — **넣을 때와 꺼낼 때가 다른 표를 본다.** 두 표가 한 글자만 어긋나도
  아이템이 다른 인벤토리로 샌다. D-01이 정확히 그 사고였고, 컴파일러는 둘 다 유효한
  enum 값이라 아무 말도 하지 않았다.
- **어디로 갈 것인가** — `SlotType ↔ ItemType`을 한 곳에서 유도하게 만든다(둘의 정의가 1:1이므로
  단일 변환 함수 + 컴파일 타임 검증이 가능하다). D-22와 같은 함수를 건드리므로 함께 처리한다.
- **그때까지의 그물** — `Server/GameServerTests/InventoryTests.cpp`의 슬롯 타입 왕복 테스트가
  세 타입을 전부 검사하므로, 표가 다시 어긋나면 테스트가 먼저 잡는다.

### D-21. `libprotobuf.lib`(16MB)이 gitignore를 뚫고 추적 중
`[심각도: 하] [난이도: 하]` · `P1/Source/ProtobufCore/Lib/Win64/libprotobuf.lib`

`P1/.gitignore:30`의 `*.lib`에 걸리는데 과거에 `git add -f`로 강제 추가됐다.

- **판정** — 클론 즉시 빌드되게 하려는 의도였다면 **합리적**이다(UE 프로젝트에서 흔한 절충).
  100MB 한계에서도 멀다.
- **어디로 갈 것인가** — 그대로 두되, `P1/.gitignore`에 `!Source/ProtobufCore/Lib/**` 예외를
  명시해 "실수로 들어간 게 아님"을 드러낸다.
