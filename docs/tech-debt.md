# Tech Debt

지금 틀린 것만 담는다. 해결이 확정되면 항목을 지운다 — 수정 완료 표기를 남기지 않는다.
무엇을 어떻게 고쳤는지는 커밋이 갖는다.

항목 10개 (높음 3 · 중간 5 · 낮음 2)

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

## 사망한 플레이어가 부활하지 못한다
> **심각도:** 높음 · **난이도:** 중간 · **범위:** 기능 · client
> 위치: `P1/Content/P1/UI/Screens/WBP_DeathScreen.uasset` · `P1/Source/P1/Core/P1GameInstance.cpp`
> (`HandleRespawn`) · `P1/Source/P1/Game/Entities/P1Creature.cpp`
> 등록일: 2026년 9월 29일

사망 화면의 카운트다운이 끝나면 `WBP_DeathScreen`의 `ReturnToTown`이 불린다. 그런데 이 이벤트는
문구를 「마을로 귀환 중...」으로 바꿀 뿐이고 `C_RESPAWN`을 보내지 않는다. 보내는 함수
`UP1DeathWidget::SendRespawnInTownPacket`은 있지만 부르는 곳이 없다. 서버 쪽
`Room::C_HandleRespawn`은 구현되어 있다.

패킷이 오가더라도 클라이언트가 사망 상태를 풀지 않는다. `UP1GameInstance::HandleRespawn`은 로그만
남기고, `SetDeadState(false)`를 부르는 곳이 코드 전체에 없다. 애니메이션 블루프린트 넷도 `OnDie`에서
사망 플래그를 켜기만 하고 끄지 않는다.

### 영향

**버그 발생 가능성 증가** · **테스트 어려움** — 한 번 죽으면 사망 화면에서 빠져나올 수 없어
클라이언트를 다시 띄워야 한다. 사망 이후의 흐름(부활 위치, 부활 뒤 전투)을 손으로 확인할 수 없다.

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

## 패킷 핸들러 20개가 `GWorld` 전역에 묶여 있다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 모듈 · client
> 위치: `P1/Source/P1/Network/ClientPacketHandler.cpp` (핸들러 23개 중 20개)
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
> 위치: `Server/GameServer/config.h` · `P1/Source/P1/Core/P1GameInstance.h` 97~98줄 ·
> `P1/Source/P1/Online/P1LoginManager.h` 33~34줄
> 등록일: 2026년 8월 19일

| 위치 | 값 | 형태 |
|---|---|---|
| `Server/GameServer/config.h` | GameDB 접속 문자열, Redis URI | `#define` (gitignore됨) |
| `P1/Source/P1/Core/P1GameInstance.h` 97~98줄 | `127.0.0.1` / `7777` | `const` 멤버 |
| `P1/Source/P1/Online/P1LoginManager.h` 33~34줄 | `127.0.0.1` / `5000` | 멤버 초기값 |
| `Server/AuthServer/.env` | 나머지 전부 | 유일하게 런타임 설정 |

`config.h` 방식의 실질 이점은 두 가지다. 오타가 컴파일 에러로 잡히고, 배포물에 설정 파일을
딸려 보낼 필요가 없다.

### 영향

**새 기능 개발 지연** · **변경 비용 증가** — `config.h`가 gitignore돼 있고 예제 파일도 없어 새로
클론한 사람은 빌드 자체가 안 된다. 테스트용 DB를 가리키게 할 방법이 없어 L1 이상의 통합 테스트를
막는다. 접속처를 바꾸려면 재빌드해야 한다.

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

**2026년 9월 22일 덧붙임 — 엔티티 조회가 열한 곳에 흩어져 있다.** #87이 이 파일의 클래스 이름을
옮기면서 세었다. `World->GetSubsystem<UP1StatefulEntityManager>()` 호출이 11회이고, 그중 7회는
바로 뒤에서 `FindEntity(EntityId)`를 불러 `nullptr`을 검사하는 같은 세 단계를 되풀이한다. 나머지
넷은 스폰과 디스폰이다. 핸들러마다 월드와 서브시스템과 액터를 차례로 타고 내려가므로, 이
클래스를 쪼개지 않더라도 `AActor* FindEntityActor(uint64)` 하나를 두면 일곱 자리가 한 줄이 된다.

### 영향

**변경 영향 범위 확대** · **테스트 어려움** — 게임 인스턴스는 레벨 전환에 살아남는 싱글턴이라
여기 붙은 모든 것이 전역 상태가 된다. 핸들러 하나를 고치려 해도 소켓 수명과 델리게이트 구독을
함께 따져야 한다. 엔티티 조회가 흩어져 있어서 서브시스템 이름을 바꾸는 작업도 열한 자리를
함께 연다.

## 게임 도메인이 배선 계층을 거꾸로 부른다
> **심각도:** 낮음 · **난이도:** 중간 · **범위:** 모듈 · client
> 위치: `P1/Source/P1/Game/`
> 등록일: 2026년 9월 22일

`docs/folder-structure.md` 3.3이 정한 화살표는 `Core → Game/Entities → 게임 도메인` 한 방향인데,
`Game/` 아래 열두 자리가 반대로 배선을 부른다. #73이 폴더를 옮기면서 드러났고 그 티켓이 만든
것은 아니다.

| 부르는 쪽 | 부르는 것 | 건수 |
|---|---|---|
| `Game/Entities/`의 `.cpp` 둘 | `Core/P1InGamePlayerController.h` · `Core/P1MyPlayerData.h` | 2 |
| `Game/Entities/`의 `.cpp` 둘 | `UI/WorldSpace/P1NameplateWidget.h` | 2 |
| `Game/` 아래 `.cpp` 다섯 | 모듈 헤더 `P1.h` | 5 |
| `Game/` 아래 헤더 셋 | `Protocol.pb.h` | 3 |

`P1.h`를 부르는 다섯 자리의 원인은 `SEND_PACKET` 매크로다. 그 매크로가 `P1.h`에 있고 안에서
`ClientPacketHandler`와 `UP1GameInstance`를 함께 부르므로, 패킷 하나를 보내려는 게임 코드가
네트워크와 `Core/`를 통째로 끌어온다.

**2026년 9월 22일 덧붙임 — 전수로 다시 세었다.** 위 표는 헤더만 세어서 생성물 참조가 3건으로
적혀 있다. `.cpp`를 함께 세면 3.3이 금지한 방향의 `#include`가 모두 27건이고 구성은 아래와 같다.

| 부르는 쪽 | 부르는 것 | 건수 |
| --- | --- | --- |
| `Game/` 아래 여섯 폴더 | `.pb.h` (`Data` 6 · `Entities` 5 · `Equipment` 2 · `Inventory` 2 · `World` 2 · `Combat` 1) | 18 |
| `Game/` 아래 네 폴더 | 모듈 헤더 `P1.h` (`Entities` 2 · `Equipment` 1 · `Inventory` 1 · `World` 1) | 5 |
| `Game/Entities/` | `Core/` | 2 |
| `Game/Entities/` | `UI/` | 2 |

**생성물을 부르는 18건은 모듈 경계의 문제가 아니라 자료형 설계의 문제다.** 도메인이 프로토콜
자료형을 그대로 쓰고 있어서, `P1`을 여러 모듈로 쪼개도 게임 모듈이 프로토콜 모듈에 의존하는
형태로 그대로 남는다. 그래서 앞으로 만들 의존 방향 검사에서 이 18건을 대상에서 빼고 나머지
9건을 본다.

### 영향

**변경 영향 범위 확대** · **테스트 어려움** — 3.3이 「게임 도메인은 `Network/`를 직접 참조하지
않는다」를 불변식으로 적는데 매크로 하나가 그것을 우회한다. 통신 방식을 바꾸면 게임 도메인의
열두 자리를 함께 연다.

## 인게임에서 받은 `S_MOVE`가 전부 빈 페이로드였다
> **심각도:** 낮음 · **난이도:** 중간 · **범위:** 기능 · protocol
> 위치: `Server/GameServer/Game/Room/Room.cpp` · `P1/Source/P1/Network/ClientPacketHandler.cpp`
> 등록일: 2026년 9월 16일 · 분리: 2026년 9월 29일

2026년 9월 16일에 인게임 세션 하나가 받은 패킷을 세었을 때 `S_MOVE`(1020) 38건이 전부 페이로드
0바이트였다. 프로토버프는 설정하지 않은 필드를 직렬화하지 않으므로, 좌표를 담는 `PosInfo`가 한
번도 채워지지 않았다는 뜻이다. 좌표가 0인 `PosInfo`를 채웠다면 필드 헤더만큼은 바이트가 남는다.

이 측정은 캐릭터가 3초 만에 스스로 죽던 시기에 했다. 그 원인은 `BP_MyPlayer`의 테스트 노드였고
2026년 9월 29일에 지웠다. 그 뒤로는 다시 재지 않았다.

### 영향

**버그 발생 가능성 증가** — 원인이 서버의 송신 쪽에 있다면 다른 클라이언트에게 위치가 전달되지
않는다. 캐릭터가 오래 살아 있는 상태에서 다시 재야 틀렸는지가 확정된다.
