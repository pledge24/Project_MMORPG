# Tech Debt

지금 틀린 것만 담는다. 해결이 확정되면 항목을 지운다 — 수정 완료 표기를 남기지 않는다.
무엇을 어떻게 고쳤는지는 커밋이 갖는다.

항목 15개 (높음 3 · 중간 11 · 낮음 1)

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

## 인게임 진입 직후 캐릭터가 스스로 죽는다
> **심각도:** 높음 · **난이도:** 중간 · **범위:** 기능 · client
> 위치: `P1/Content/Blueprints/Creatures/Monster/` · `P1/Content/Blueprints/UI/InGame/WBP_DeathScreen.uasset`
> 등록일: 2026년 9월 16일

인게임 맵에 진입하고 약 3초 뒤에 사망 화면이 뜬다. **서버는 `S_DIE`(1034)도 `S_HIT`(1023)도
보내지 않는다.** 2026년 9월 16일 실측에서 그 세션이 받은 패킷은 아래가 전부다.

| 패킷 | ID | 건수 |
|---|---|---|
| `S_MOVE` | 1020 | 38 |
| `S_SPAWN` | 1017 | 2 |
| `S_LOGIN` | 1003 | 2 |
| `S_ENTER_ROOM` `S_ENTER_GAME` `S_CREATE_CHARACTER` | 1016 1009 1005 | 각 1 |

그럼에도 `WBP_DeathScreen_C_0`이 생성됐다. 즉 클라이언트가 서버 판정 없이 혼자 사망으로
결론지었다. 전투 판정과 사망 화면이 모두 블루프린트에 있어 C++에서 추적할 수 없다.

S_MOVE 38건이 전부 페이로드 0바이트인 점도 같이 본다. 프로토버프가 기본값 필드를 생략하므로,
이는 좌표가 전부 0인 이동이 오갔다는 뜻이다.

### 영향

**버그 발생 가능성 증가** · **테스트 어려움** — 인게임을 3초 넘게 유지할 수 없어 전투·인벤토리·
상점 등 인게임 기능 전체를 손으로 확인할 수 없다. 서버 로그와 대조해도 원인이 클라 블루프린트
안에 있어 좁혀지지 않는다.

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

## 로그인 라우터만 에러를 로그 없이 삼킨다
> **심각도:** 중간 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/AuthServer/src/routes/login.router.js` 64~66줄
> 등록일: 2026년 9월 16일

`POST /login`의 `catch`가 500만 돌려주고 원인을 어디에도 남기지 않는다. 같은 저장소의 다른 두
곳은 남긴다(2026년 9월 16일 실측).

| 위치 | 에러 처리 |
|---|---|
| `account.router.js` 58~60줄 | `console.log`로 원문을 남긴 뒤 500 |
| `app.js` 29~31줄 | `console.error`로 원문을 남김 |
| `login.router.js` 64~66줄 | **남기지 않고** 500 |

이 핸들러가 부르는 것은 bcrypt 비교와 MSSQL 조회와 Redis 기록 셋이다. 셋 중 어디서 터져도
클라이언트가 받는 응답과 서버에 남는 흔적이 똑같다.

ESLint를 붙이자 `no-unused-vars`가 이 자리를 잡았다. 그때는 `catch (err)`의 `err`를 지워 경고만
없앴고, 삼킴 자체는 그대로다.

### 영향

**유지보수 어려움** · **테스트 어려움** — 로그인은 가장 자주 도는 경로인데 500이 나면 원인을
좁힐 방법이 없다. 세 의존성 중 무엇이 죽었는지 알려면 서버에 붙어 재현해야 한다.

## 패킷 핸들러 20개가 `GWorld` 전역에 묶여 있다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 모듈 · client
> 위치: `P1/Source/P1/ClientPacketHandler.cpp` (핸들러 23개 중 20개)
> 등록일: 2026년 9월 16일

`Handle_S_*` 23개 중 20개가 `Cast<UP1GameInstance>(GWorld->GetGameInstance())`로 시작한다.
자기 자신의 월드를 인자로 받지 않고 전역에서 끌어온다.

그래서 핸들러는 **호출되는 시점의 `GWorld`가 게임 월드일 때만** 동작한다. 레벨 스크립트
블루프린트가 펌프를 부르던 동안에는 그 조건이 우연히 성립했다. 월드 틱 안에서 호출됐기
때문이다. 펌프를 코어 티커로 옮기자 조건이 깨졌고, 에디터에서 `GWorld`가 에디터 월드를 가리켜
20개 핸들러가 전부 첫 줄에서 탈락했다(2026년 9월 16일 실측).

지금은 `UP1GameInstance::TickRecvPump`가 펌프 호출 구간에만 `GWorld`를 게임 월드로 바꿔
우회한다. 전역을 직접 대입하는 코드라 그 자체가 부채다.

### 영향

**변경 영향 범위 확대** · **테스트 어려움** — 패킷 처리를 어디에서 부르느냐가 핸들러의 동작을
바꾼다. 호출 지점을 옮길 때마다 20곳이 함께 깨지고, 컴파일러는 아무것도 잡지 못한다. 월드를
인자로 받지 않으므로 핸들러 단위 테스트도 세울 수 없다.

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

## 소켓과 세션이 해제되지 않는다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 기능 · client
> 위치: `P1/Source/P1/P1GameInstance.cpp` 52~100줄 · `P1/Source/P1/ClientPacketHandler.cpp` 100~107줄
> 등록일: 2026년 9월 16일

`Socket`과 `GameServerSession` 두 멤버에 `nullptr`을 대입하는 코드가 모듈 전체에 하나도 없다
(2026년 9월 16일 실측). 연결을 끊는 경로가 아래처럼 어디에서도 멤버를 비우지 않는다.

| 위치 | 하는 일 | 비우는가 |
|---|---|---|
| `DisconnectFromGameServer()` | `C_LEAVE_GAME` 패킷 전송 | 아니다 |
| `Handle_S_LEAVE_GAME` | 지역 변수로 받아 `Socket->Close()` | 아니다 |
| `PacketSession::Disconnect()` | 송수신 워커 스레드만 정리 | 소켓을 건드리지 않는다 |

`ClientPacketHandler.cpp` 102~104줄에 `DestroySocket`과 `Socket = nullptr`이 주석으로 남아
있다. 주석을 풀어도 지역 변수에 대입하는 형태라 멤버는 그대로다.

`ConnectToGameServer()`는 기존 소켓을 검사하지 않고 매번 새로 만든다. 연결에 실패하면 방금 만든
소켓을 닫지도 파괴하지도 않는다.

### 영향

**버그 발생 가능성 증가** · **변경 영향 범위 확대** — 재접속할 때마다 소켓이 샌다. 끊긴 뒤에도
`Socket`이 유효한 포인터로 남아 `Socket == nullptr` 가드가 통과하므로, 닫힌 소켓에 계속 쓰기를
시도한다. 수명을 고치려면 `P1GameInstance`와 `ClientPacketHandler`와 `PacketSession` 셋을 함께
봐야 한다.

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

`Inventory`는 서로 정합해야 하는 표를 넷 들고 있고, 넷 다 생성자에서 손으로 채운다. 제목의
"3종"은 서로 변환하는 표만 센 것이다.

| 표 | 방향 | 쓰는 곳 |
|---|---|---|
| `itemTypeMappings` | 아이템 데이터의 `"itemType"` 문자열 → `ItemType` | `addItem` |
| `slotTypeToItemTypeMappings` | `SlotType` → `ItemType` | `removeItem`, `GetSlot` |
| `inventorylookupMappings` | `ItemType` → 실제 슬롯 배열 | 전부 |
| `dirtyFlagsMappings` | `ItemType` → 슬롯별 더티 플래그 | `addItem`, `removeItem`, `GetDirtyFlags` |

**검사한 표와 인덱싱하는 표가 다른 자리가 둘 있다.**
— `Inventory.cpp:83`은 `addItem`이 69~71줄에서 `inventorylookupMappings`를 `find`로 확인한 뒤
`dirtyFlagsMappings`를 `operator[]`로 인덱싱한다. `Inventory.cpp:162`는 `removeItem`이 135줄에서
`slotTypeToItemTypeMappings`를 확인한 뒤 같은 일을 한다. 지금 터지지 않는 것은 생성자가 네 표를
같은 세 키로 채우기 때문이고, 코드가 그 사실을 보장하지는 않는다.

`Server/GameServerTests/InventoryTests.cpp`가 네 표 중 셋의 키 집합을 기대 집합에 고정하므로,
표가 다시 어긋나면 테스트가 먼저 잡는다. `itemTypeMappings`는 키가 문자열이라 열거형 리플렉션
대조가 닿지 않고, 관측 경로도 없다. 2026년 9월에 검토했다가 폐기한 설계가
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

### 선행 조건

**스키마 변경이라 사람 승인이 먼저다.** CLAUDE.md 「안전」이 스키마 변경을 사람 승인 후에,
저장소의 SQL 스크립트 갱신과 함께만 실행하도록 정한다. 대상이 서로 다른 두 LocalDB 인스턴스에
걸쳐 있다는 점도 함께 본다. UserDB는 `(localdb)\MSSQLLocalDB`, GameDB는 `(localdb)\ProjectModels`다.
난이도가 낮다고 적혀 있지만 그것은 수정 범위의 크기이지 착수 조건의 무게가 아니다.
