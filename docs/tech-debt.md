# Tech Debt

지금 틀린 것만 담는다. 해결이 확정되면 항목을 지운다 — 수정 완료 표기를 남기지 않는다.
무엇을 어떻게 고쳤는지는 커밋이 갖는다.

항목 11개 (높음 3 · 중간 3 · 낮음 5)

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

## 캐릭터를 만들 때 클라이언트가 보낸 직업을 검증하지 않는다
> **심각도:** 높음 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/GameServer/DB/CharacterListDAO.cpp` (`CreateCharacter`) · `Server/GameServer/Main/ServerPacketHandler.cpp` (`Handle_C_CREATE_CHARACTER`)
> 등록일: 2026년 10월 1일

`Handle_C_CREATE_CHARACTER`는 요청을 검증하지 않고 DB 큐로 넘긴다. `CreateCharacter`는 요청의 직업 번호로
`Gamedata::s_classLevelDataTableMappings[_classId]`를 읽는다. 표에 없는 번호면 `operator[]`가 널 포인터를
끼워 넣고 곧바로 역참조한다. 이 표는 여러 DB 스레드가 함께 읽는 전역 표라서, 끼워 넣기 자체도 다른 스레드의
조회와 경쟁한다. 코드를 읽고 판단했고 실행해서 확인하지는 않았다.

### 영향

**버그 발생 가능성 증가** — 클라이언트가 없는 직업 번호로 캐릭터 생성을 보내면 게임 서버가 죽는다.

## `Room` 갓 클래스
> **심각도:** 높음 · **난이도:** 높음 · **범위:** 모듈 · server
> 위치: `Server/GameServer/Game/Room/Room.cpp` (1,249줄)
> 등록일: 2026년 8월 19일 · 본문 갱신: 2026년 10월 1일

`Room` 하나가 입장·퇴장·이동·리스폰·아이템 요청·채팅·셀 행렬·몬스터 스폰을 들고 있고, 피격·처치·사망
결과의 전송도 맡는다(`Room.h` 32~83줄). 피격과 처치의 판정은 `Game/Combat/`으로 옮겼다. 남은 조각은
아래 둘이다.
- 룸 이동: 입장·퇴장·포털 이동·리스폰
- 아이템 요청 핸들러: 구매·판매·사용·착용·해제

`Room`의 테스트는 무작위 위치 하나뿐이다. 입장·리스폰은 룸 큐와 세션을 함께 띄워야 해서 테스트가
없다. 나머지 테스트 그물은 Combat(피격과 처치), 저장할 아이템 행 고르기, Inventory, Player(레벨과 보상, 장비 결과, 저장 사본,
소모품 사용), Monster 초기화, 프로토콜에 있다.

### 영향

**테스트 어려움** · **변경 영향 범위 확대** — `Room.cpp`가 서버 코드 10,278줄(`.pb` 생성물
제외)의 12%다. 어느 기능을 고쳐도 같은 파일을 만지므로 변경이 서로 부딪히고, 테스트 대상을 잘라내기가
불가능하다.

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
| `WBP_Slot` | `SlotWidget` | 그래프 6개(`GetToolTipWidget`·`OnMouseButtonDown`·`OnMouseButtonDoubleClick` 외), 이벤트 `OnStartCooldown`·`OnUpdateCooldown`·`OnUse`, 변수 9개(`CooldownTimerHandle`·`ElapsedTime`·`IntervalTime` 외). 쿨다운 상태 머신 전체. 슬롯마다 도는 쿨다운(서버는 템플릿마다 판정). 같은 물약이 두 칸이면 다른 칸이 쓸 수 있어 보이나 서버가 거부 |
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

## 인벤토리의 요청 대기가 풀리지 않는 경로가 있다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 기능 · client
> 위치: `P1/Source/P1/UI/Screens/P1InventoryWidget.cpp` (`SendUseItemPacket`) · `P1/Source/P1/Core/P1GameInstance.cpp` (`HandleUseItem`)
> 등록일: 2026년 9월 30일

인벤토리 위젯은 요청을 보내기 전에 `PendingPacket`을 켜고, 응답이 오면 끈다. 그런데 켠 채로 남는 경로가 있다.
- `SendUseItemPacket`에서 게임 인스턴스가 없을 때와 소모품이 아닌 분기에서는 켜기만 하고 요청을 보내지 않는다
- `HandleUseItem`에서 `FindEntityAs`로 내 플레이어를 찾지 못하면 `OnRecvUseItemPkt`를 알리지 않는다

코드를 읽고 판단했고 실행해서 확인하지는 않았다.

### 영향

**버그 발생 가능성 증가** — 한 번 이 경로를 타면 인벤토리를 다시 열어도 아이템을 쓰거나 팔 수 없다.

## 클라이언트가 보상 결과를 반영하지 않는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · client
> 위치: `P1/Source/P1/Core/P1GameInstance.cpp` (`HandleRewardResult`)
> 등록일: 2026년 9월 30일

`HandleRewardResult`는 소켓과 월드를 확인한 뒤 아무것도 하지 않는다. 서버는 `S_REWARD_RESULT`에 경험치,
골드, 레벨업 결과(`level_up_details`)를 싣지만, 클라이언트의 HUD와 내 플레이어 데이터에는 반영되지 않는다.
지금은 플레이어가 몬스터를 때리는 경로가 없어 보상이 오지 않는다. 코드를 읽고 판단했다.

### 영향

**새 기능 개발 지연** — 공격 판정을 넣으면 몬스터를 잡아도 경험치와 골드가 화면에 바뀌지 않는다. 재접속해야
서버에 저장된 값이 보인다.

## 입장할 때 캐릭터 행이 없어도 기본 정보를 채운다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/GameServer/DB/CharacterStateDAO.cpp` (`LoadCharacter`)
> 등록일: 2026년 10월 1일

`LoadCharacter`는 `Fetch()`의 결과를 보지 않는다. 요청한 캐릭터의 행이 없으면 초기화되지 않은 바인딩 버퍼의
값으로 직업, 이름, 레벨을 채운다. 뒤의 `LoadLastState`가 행을 찾지 못해 입장이 실패로 끝나므로 지금은
잘못된 값이 클라이언트로 나가지 않는다. 코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 뒤 단계의 실패에 기대고 있어서, 불러오는 순서를 바꾸면 쓰레기 값으로 입장할 수 있다.

## 밀려난 세션의 저장보다 새 세션의 불러오기가 먼저 끝날 수 있다
> **심각도:** 낮음 · **난이도:** 중간 · **범위:** 기능 · server
> 위치: `Server/GameServer/Main/ServerPacketHandler.cpp` (`Handle_C_LOGIN`, `Handle_C_ENTER_GAME`) · `Server/GameServer/Main/GameSession.cpp`
> 등록일: 2026년 9월 30일

같은 계정의 새 로그인이 기존 세션을 끊으면, 기존 세션의 저장은 IOCP 완료 → `OnDisconnected` → 룸 잡 →
`userId` DB 큐 순서로 늦게 들어간다. 새 세션의 `C_ENTER_GAME` 불러오기도 같은 `userId` DB 큐로 가지만,
끊기 전에 큐에 들어가면 저장 전의 진행을 불러온다. 사람은 캐릭터를 고르는 몇 초가 있어 드러나지 않는다.
코드를 읽고 판단했고 실행해서 확인하지는 않았다.

### 영향

**버그 발생 가능성 증가** — 로그인 직후 곧바로 입장하는 클라이언트(DummyClient 등)에서 밀려난 세션의
마지막 진행이 사라지고, 새 세션이 끊길 때 낡은 진행으로 덮어쓴다.

## 서버가 연결을 끊을 때 보낸 사유 패킷이 버려질 수 있다
> **심각도:** 낮음 · **난이도:** 중간 · **범위:** 함수 · server
> 위치: `Server/ServerCore/Network/Session.cpp` (`Disconnect`, `RegisterSend`)
> 등록일: 2026년 9월 30일

`Session::Disconnect`는 `_connected`를 곧바로 내린다. 다른 송신이 진행 중이라 송신 큐에서 기다리던
패킷은, 앞선 송신이 끝난 뒤 `RegisterSend`가 연결이 끊긴 것을 보고 버린다. `Handle_C_LOGIN`의
`KickSession`은 `S_LEAVE_GAME`을 보낸 직후 끊으므로 이 경로를 탈 수 있다. 코드를 읽고 판단했다.

### 영향

**유지보수 어려움** — 사유를 잃은 클라이언트는 「게임 서버와 연결이 끊겼습니다」만 보여 준다. 중복 로그인으로
밀려났는지 서버가 내려갔는지 화면으로 구분하지 못한다.
