# Tech Debt

지금 틀린 것만 담는다. 해결이 확정되면 항목을 지운다 — 수정 완료 표기를 남기지 않는다.
무엇을 어떻게 고쳤는지는 커밋이 갖는다.

항목 13개 (높음 2 · 중간 5 · 낮음 6)

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

## `Room` / `DBRequestFunctions` 갓 클래스
> **심각도:** 높음 · **난이도:** 높음 · **범위:** 모듈 · server
> 위치: `Server/GameServer/Game/Room/Room.cpp` (1,161줄) · `Server/GameServer/DB/DBRequestFunctions.cpp` (1,531줄)
> 등록일: 2026년 8월 19일

`Room` 하나가 입장·퇴장·이동·전투·피격·처치·사망·보상·리스폰·채팅·셀 행렬·몬스터 스폰을 전부
들고 있다(`Room.h` 32~83줄). `DBRequestFunctions`는 캐릭터·상태·인벤토리·장비의 모든 쿼리를 한
파일에 담는다.

`Room`에는 테스트가 없다. 현재 테스트 그물은 Inventory, Player(레벨 상한과 장비 결과), 프로토콜에만 있다.

### 영향

**테스트 어려움** · **변경 영향 범위 확대** — 두 파일이 서버 코드 10,009줄의 27%다. 어느 기능을
고쳐도 같은 파일을 만지므로 변경이 서로 부딪히고, 테스트 대상을 잘라내기가 불가능하다.

## 공격 콤보와 몽타주 선택이 블루프린트에 있다
> **심각도:** 높음 · **난이도:** 높음 · **범위:** 기능 · client
> 위치: `P1/Content/P1/Characters/Monsters/`
> 등록일: 2026년 8월 19일 · 경로 갱신: 2026년 9월 21일 (#52)

`BPC_MonsterAttackSystem`과 `BPC_WarriorAttackSystem`(부모 C++ `AttackSystemComponent`)이
`S_PerformNormalAttack`·`PerformNormalAttack`·`ResetAttackCombo`를 BP로 구현하고 `NormalAttacks`
배열을 들고 있다. 콤보 상태 머신과 몽타주 선택이 전부 BP에 있다. C++ `AttackSystemComponent`는
56+52줄뿐이다.

몬스터 쪽 BP 클래스는 `BP_MonsterBase`(부모 C++ `Monster`) 아래로 `BP_{Melee,Ranged,Super}MonsterBase`
3개와 미니언 9개, 모두 13개다. 2026년 9월 29일에 다시 읽은 결과 이 클래스들의 `BeginPlay`·`Tick`·
`ActorBeginOverlap`은 비어 있거나 부모를 부르기만 한다. 몬스터 행동 로직은 BP에 없다.

### 영향

**버그 발생 가능성 증가** · **유지보수 어려움** — 서버가 전투를 판정하는데
(`Room::HandleNormalAttack`) 클라이언트의 콤보 규칙은 BP에 있어서, 양쪽 규칙이 갈라져도
컴파일러도 테스트도 잡지 못한다.

## 몬스터 처치 뒤 경로에 잠복한 결함이 쌓여 있다
> **심각도:** 중간 · **난이도:** 낮음 · **범위:** 기능 · server
> 위치: `Server/GameServer/Game/Entities/Monster.cpp` · `Server/GameServer/Game/Entities/Player.cpp` ·
> `Server/GameServer/Game/Room/Room.cpp` (`HandleHit`, `HandleDie`)
> 등록일: 2026년 9월 30일

플레이어가 몬스터를 때리는 서버 경로가 아직 없다(`C_NORMAL_ATTACK`에 대상이 없고
`Room::C_HandleNormalAttack`은 브로드캐스트만 한다). 그래서 아래 결함은 지금 드러나지 않는다.
공격 판정을 넣는 순간 한꺼번에 드러난다.

- `Monster::Init`이 `_statInfo`에 HP를 넣지 않는다. 피격되면 `Creature::GetStatValue`의
  `Map::at`이 실패해 서버가 죽는다(`Creature::OnHit`, `Room::HandleHit`).
- `Player::OnGetReward`에 `else`가 없어서, 레벨업하지 않는 보상의 경험치가 저장되지 않는다.
- `OnGetReward`가 만든 `LevelUpInfo`를 보상 패킷에 싣지 않는다.
- 사망한 몬스터는 서버에서 지워지지만 `S_DESPAWN`을 보내지 않아서 클라이언트에 액터가 남는다.
- 보상 계산의 `Utils::GetRandom(min, max)`는 정수일 때 `[min, max)`라서 최댓값이 나오지 않는다.
  `min == max`면 분포가 `(min, min-1)`이 되어 정의되지 않은 동작이다.

### 영향

**버그 발생 가능성 증가** · **부채의 연쇄 증가** — 전투 기능을 붙이는 작업이 결함 다섯을 먼저
고쳐야 시작된다. 그중 하나는 서버 크래시다.

## 같은 계정으로 두 번 로그인해도 막지 않는다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 기능 · server
> 위치: `Server/GameServer/Main/ServerPacketHandler.cpp` (`Handle_C_LOGIN`)
> 등록일: 2026년 9월 30일

`Handle_C_LOGIN`은 Redis에서 액세스 토큰을 읽어 `userId`를 세션에 넣기만 한다. 같은 `userId`로 이미
접속한 세션이 있는지 보지 않고, 읽은 토큰을 지우지도 않는다. 그래서 같은 토큰으로 두 클라이언트가
동시에 들어와 같은 캐릭터로 입장할 수 있다. 코드를 읽고 판단했고 실행해서 확인하지는 않았다.

두 세션은 서로 다른 `Player`를 들고 각자 접속 종료 때 저장한다. 나중에 끊긴 쪽이 먼저 끊긴 쪽의
진행을 덮어쓴다.

### 영향

**버그 발생 가능성 증가** — 한쪽 창에서 얻은 경험치와 아이템이 다른 창의 종료 저장으로 사라진다.
거래 기능이 생기면 아이템 복사로 이어질 수 있다. 막으려면 인증 서버의 토큰 수명과 기존 세션을
끊는 규칙을 함께 정해야 한다.

## C++ 베이스 없이 BP에만 사는 UI/액터
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 기능 · client
> 위치: `P1/Content/P1/` 아래 (`UI/`, `World/`, `Characters/`)
> 등록일: 2026년 8월 19일 · 경로 갱신: 2026년 9월 21일 (#52)

UE 에디터로 41개 BP의 부모 클래스를 전수 확인한 결과, 위젯 12/15는 이미 C++ 클래스로
리페어런트되어 있다. 남은 것은 아래 넷이다.

| 에셋 | 부모 | BP에 있는 것 |
|---|---|---|
| `WBP_CharacterSlot` | `UserWidget` | 함수 그래프 `UpdateCharacterInfo`·`DisableHighlight`·`Clear`, 디스패처 `OnSlotButtonClicked`, 변수 `Characterid`·`ThisSlotId` |
| `BP_Shop` | `Actor` | 오버랩 상호작용 + `PlayerController` 참조. C++에 `UP1ShopWidget`은 있는데 상점 액터가 없다 |
| `WBP_NameTag` | `UserWidget` | `Tick`·`PreConstruct`·`Construct`. `UP1NameplateWidget`과 역할이 겹친다 |
| `WBP_Help` | `UserWidget` | `Tick`·`PreConstruct`·`Construct`. 순수 표시용 |

C++ 부모가 있는데도 BP 쪽 로직이 무거운 것은 아래 셋이다.

| 에셋 | 부모(C++) | BP에 남은 로직 |
|---|---|---|
| `WBP_Slot` | `SlotWidget` | 그래프 6개(`GetToolTipWidget`·`OnMouseButtonDown`·`OnMouseButtonDoubleClick` 외), 이벤트 `OnStartCooldown`·`OnUpdateCooldown`·`OnUse`, 변수 9개(`CooldownTimerHandle`·`ElapsedTime`·`IntervalTime` 외). 쿨다운 상태 머신 전체 |
| `WBP_LoginMenu` | `LoginWidget` | 그래프 4개(`CC_Init`·`DisableAllSlotsHighlight`·`ClearAllSlots`·`IsValidCharacter`) + `OnDisplayCharacterOverviews` |
| `WBP_DeathScreen` | `DeathWidget` | `Countdown`·`StartCountdown`·`ReturnToTown` + `ReturnCountdown`·`ElapsedTime`·`Timer`. 리스폰 카운트다운 |

### 영향

**테스트 어려움** · **유지보수 어려움** — 쿨다운과 카운트다운처럼 시간과 상태를 다루는 로직이
BP에 있으면 단위 테스트가 불가능하고 Live Coding으로도 검증할 수 없다. `WBP_Slot`의 쿨다운
상태 머신과 `WBP_DeathScreen`의 리스폰 카운트다운이 여기 해당한다.

## 캐릭터 클래스와 컨트롤러에 관심사가 뭉쳐 있다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 모듈 · client
> 위치: `P1/Source/P1/Game/Entities/` · `P1/Source/P1/Core/`
> 등록일: 2026년 8월 19일

| 클래스 | 뭉쳐 있는 것 |
|---|---|
| `AP1Creature` (`Game/Entities/P1Creature.h`, 258줄) | 이동 보간(`MoveQueue`·`CorrectionMaxThreshold`·`CORR_INTERP_SPEED`) + 어택 컴포넌트 + 네임플레이트 위젯 + 사망 상태 + `S_*` 수신 처리 |
| `AP1MyPlayer` (`Game/Entities/P1MyPlayer.h`, 126+257줄) | 카메라 붐 + Enhanced Input 액션 5종 + 이동 패킷 스로틀(`MOVE_PACKET_SEND_DELAY`·`YAW_TOLERANCE`·더티 플래그) + 전투 모드 + 디버그 카운터 |
| `AP1InGamePlayerController` (`Core/P1InGamePlayerController.h`, 124+219줄) | 위젯 7종의 `TSubclassOf`/인스턴스 쌍 + `WidgetMappings` + `WidgetFlag` 비트마스크 + `CurrentMaxZOrder` 관리 |

이동 동기화 로직이 수신(`AP1Creature`)과 송신(`AP1MyPlayer`) 양쪽에 갈라져 있다. 보간 상수와
스로틀 상수도 두 파일에 따로 산다.

### 영향

**변경 영향 범위 확대** · **버그 발생 가능성 증가** — 이동 동기화를 고칠 때 한쪽만 고치는 사고가
나기 쉽다. 두 파일의 상수가 어긋나도 컴파일러가 잡지 않고, 증상은 특정 지연 구간에서만
드러난다.

## `UP1GameInstance`가 클라 측 갓 클래스
> **심각도:** 중간 · **난이도:** 높음 · **범위:** 모듈 · client
> 위치: `P1/Source/P1/Core/P1GameInstance.cpp` (620줄)
> 등록일: 2026년 8월 19일

소켓 소유 + 세션 관리 + `S_*` 핸들러 16개 + 스폰/디스폰 + 델리게이트 5종 브로드캐스트 + 토큰
보관을 한 클래스가 들고 있다.

### 영향

**변경 영향 범위 확대** · **테스트 어려움** — 게임 인스턴스는 레벨 전환에 살아남는 싱글턴이라
여기 붙은 모든 것이 전역 상태가 된다. 핸들러 하나를 고치려 해도 소켓 수명과 델리게이트 구독을
함께 따져야 한다.

## `ARCHITECTURE.md`가 수신 펌프를 레벨 블루프린트가 부른다고 적는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · client
> 위치: `docs/ARCHITECTURE.md` 82~88줄
> 등록일: 2026년 9월 30일

그 절은 「수신 펌프를 호출하는 C++ 코드가 없다」를 불변식으로 적고, 호출부가 레벨 블루프린트의
`ReceiveTick`에 있다고 한다. 지금 코드는 `UP1GameInstance::Init`이 코어 티커에 `TickRecvPump`를
등록해서 레벨과 무관하게 펌프를 돌린다(`P1GameInstance.cpp` 30~35줄). 불변식이 사실과 반대다.

### 영향

**유지보수 어려움** — 불변식 문서가 틀리면 새 레벨을 만들 때 쓸모없는 노드를 손으로 넣거나,
문서의 다른 불변식까지 의심하게 된다.

## 몬스터가 룸 경계에 붙어 스폰될 수 있다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/GameServer/Game/Room/Room.cpp` (`GetRandomLocation`)
> 등록일: 2026년 9월 30일

`GetRandomLocation`이 여백을 뺀 경계(`paddedMinX` 등)를 계산해 놓고, 실제 난수는 여백 없는
`_roomMinX`~`_roomMaxX`에서 뽑는다. 몬스터 스폰 위치와 배회 목적지가 이 함수를 쓴다.

### 영향

**버그 발생 가능성 증가** — 경계 밖으로 조금만 밀려도 그 엔티티는 셀 행렬에 들어가지 않아 몬스터의
탐지에서 빠진다.

## 맵을 옮길 때마다 내 플레이어 델리게이트가 한 번 더 바인딩된다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · client
> 위치: `P1/Source/P1/Core/P1MyPlayerData.cpp` (`BindMyPlayerDelegate`)
> 등록일: 2026년 9월 30일

`UP1MyPlayerData`는 게임 인스턴스 서브시스템이라 레벨 전환에 살아남는다. 그런데 내 플레이어가
스폰될 때마다 `OnGoldChanged`·`OnInvenSlotChanged`·`OnEquipmentSlotChanged`에 같은 핸들러를
`AddUObject`로 다시 붙인다. 맵을 두 번 옮기면 슬롯 변경 하나가 인벤토리에 세 번 반영된다.
지금 핸들러는 값을 덮어쓰기만 해서 겉으로 드러나지 않는다(코드를 읽고 판단했다).

### 영향

**버그 발생 가능성 증가** — 누적되는 처리(개수 더하기, 알림 띄우기)를 핸들러에 넣는 순간 맵
이동 횟수만큼 중복 실행된다.

## 몬스터 5000의 공격력이 사망 확인용 값이다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · server
> 위치: `DesignData/Original_Monster.xlsx` → `S_Monster.json` (템플릿 5000 `baseAttack`)
> 등록일: 2026년 9월 30일

초급 근거리 몬스터 5000의 `baseAttack`이 1000이다. 다른 몬스터는 80~600이다. 리스폰을 PIE로
확인하려고 사람이 올려 둔 값이다. 원래 값은 기록에 없다.

### 영향

**버그 발생 가능성 증가** — 초급 사냥터에서 한 대에 죽는다. 밸런스를 볼 때 이 값을 원래대로
돌려야 한다.

## 송신 워커가 큐가 비어도 쉬지 않고 돈다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · client
> 위치: `P1/Source/P1/Network/P1SendWorker.cpp` (`Run`)
> 등록일: 2026년 9월 30일

`FP1SendWorker::Run`은 `while (Running)` 안에서 큐를 꺼내 보기만 하고 잠들지 않는다. 보낼 패킷이
없어도 코어 하나를 계속 쓴다. 전송 실패(`SendDesiredBytes`의 false)도 무시한다.

### 영향

**유지보수 어려움** — PIE 창을 여럿 띄우면 창마다 코어 하나씩 헛돈다. 연결이 끊겨도 송신 쪽은
알아채지 못한다.

`UP1GameInstance::Shutdown`은 `C_LEAVE_GAME`을 송신 큐에 넣은 직후 소켓을 닫아서, 이 패킷은 대개 서버에
닿지 않는다. 서버가 접속 종료를 한 경로에서 처리하므로 진행은 저장되지만, 서버 로그의 끊김 사유가
「Exit Game」이 아니라 「Recv 0」으로 남는다. 서버가 보내지 않는 `S_LEAVE_GAME`의 클라이언트 핸들러
(`ClientPacketHandler.cpp`)도 남아 있다.

## 장비를 불러올 때 추가 물리 공격력에 추가 마법 공격력을 넣는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/GameServer/DB/DBRequestFunctions.cpp` (`LoadCharactersGearItems`)
> 등록일: 2026년 9월 30일

`gearInfo->set_additional_physical_attack(bindObject._additionalMagicalAttack)`로 물리 공격력 자리에
마법 공격력 값을 넣는다. 인벤토리와 장착 장비 모두 이 경로로 불러온다.

### 영향

**버그 발생 가능성 증가** — 추가 공격력이 붙은 장비가 생기면 입장할 때마다 물리 수치가 틀린다.
지금은 추가 공격력을 부여하는 경로가 없어 값이 0이라 드러나지 않는다.
