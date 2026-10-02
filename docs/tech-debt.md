# Tech Debt

지금 틀린 것만 담는다. 해결이 확정되면 항목을 지운다 — 수정 완료 표기를 남기지 않는다.
무엇을 어떻게 고쳤는지는 커밋이 갖는다.

항목 19개 (높음 2 · 중간 1 · 낮음 16)

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

## 받은 패킷의 길이를 헤더 크기와 비교하지 않는다
> **심각도:** 높음 · **난이도:** 중간 · **범위:** 기능 · protocol
> 위치: `Server/ServerCore/Network/Session.cpp` 321~345줄 (`PacketSession::OnRecv`)
> 등록일: 2026년 10월 2일

헤더의 `size`가 헤더 크기(4)보다 작은지 아무 곳에서도 확인하지 않는다. 같은 결함이 세 곳에 있다.

| 위치 | `size`가 4보다 작을 때 |
|---|---|
| 서버 `PacketSession::OnRecv` | `size`가 0이면 `processLen`이 늘지 않아 루프가 끝나지 않는다. 1~3이면 핸들러가 받은 길이를 넘어 헤더를 읽는다 |
| 클라이언트 `P1RecvWorker::ReceivePacket` (`P1/Source/P1/Network/P1RecvWorker.cpp`) | 음수 `PayloadSize`를 `AddZeroed`에 넘긴다 |
| 생성된 `HandlePacket` (`Protocol/Templates/PacketHandler.h`) | `len`이 헤더 크기 이상인지 보지 않고 헤더를 읽는다 |

`OnRecv`가 음수를 돌려주면 `ProcessRecv`가 연결을 끊는 규약이 이미 있다. 클라이언트도 `ReceivePacket`이
false를 돌려주면 연결 끊김으로 처리한다. 코드를 읽고 판단했고 실행해서 확인하지는 않았다.

#122에서 찾았다. 그 티켓은 id 범위만 고치기로 했다.

### 영향

**버그 발생 가능성 증가** — 클라이언트 하나가 `size` 0인 헤더를 보내면 서버의 IOCP 스레드 하나가
무한 루프에 빠진다. 클라이언트 쪽은 서버가 보낸 값이라 위험이 낮다.

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

C++ 부모 클래스가 없는 BP는 아래 하나다.

| 에셋 | 부모 | BP에 있는 것 |
|---|---|---|
| `BP_Shop` | `Actor` | 오버랩 상호작용 + `PlayerController` 참조. C++에 `UP1ShopWidget`은 있는데 상점 액터가 없다 |

C++ 부모가 있는데도 BP 쪽 로직이 무거운 것은 아래 둘이다.

| 에셋 | 부모(C++) | BP에 남은 로직 |
|---|---|---|
| `WBP_Slot` | `SlotWidget` | 그래프 6개(`GetToolTipWidget`·`OnMouseButtonDown`·`OnMouseButtonDoubleClick` 외), 이벤트 `OnStartCooldown`·`OnUpdateCooldown`·`OnUse`, 변수 9개(`CooldownTimerHandle`·`ElapsedTime`·`IntervalTime` 외). 쿨다운 상태 머신 전체. 슬롯마다 도는 쿨다운(서버는 템플릿마다 판정). 같은 물약이 두 칸이면 다른 칸이 쓸 수 있어 보이나 서버가 거부 |
| `WBP_DeathScreen` | `DeathWidget` | `Countdown`·`StartCountdown`·`ReturnToTown` + `ReturnCountdown`·`ElapsedTime`·`Timer`. 리스폰 카운트다운 |

### 영향

**테스트 어려움** · **유지보수 어려움** — 쿨다운과 카운트다운처럼 시간과 상태를 다루는 로직이
BP에 있으면 단위 테스트가 불가능하고 Live Coding으로도 검증할 수 없다. `WBP_Slot`의 쿨다운
상태 머신과 `WBP_DeathScreen`의 리스폰 카운트다운이 여기 해당한다.

## 인벤토리의 요청 대기가 풀리지 않는 경로가 있다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 기능 · client
> 위치: `P1/Source/P1/UI/Screens/P1InventoryWidget.cpp` (`SendUseItemPacket`)
> 등록일: 2026년 9월 30일

인벤토리 위젯은 요청을 보내기 전에 `PendingPacket`을 켜고, 응답이 오면 끈다. 그런데 켠 채로 남는 경로가 있다.
- `SendUseItemPacket`에서 게임 인스턴스가 없을 때와 소모품이 아닌 분기에서는 켜기만 하고 요청을 보내지 않는다

코드를 읽고 판단했고 실행해서 확인하지는 않았다.

### 영향

**버그 발생 가능성 증가** — 한 번 이 경로를 타면 인벤토리를 다시 열어도 아이템을 쓰거나 팔 수 없다.

## 클라이언트가 보상 결과를 반영하지 않는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · client
> 위치: `P1/Source/P1/Network/ClientPacketHandler.cpp` (`Handle_S_REWARD_RESULT`)
> 등록일: 2026년 9월 30일

`Handle_S_REWARD_RESULT`는 아무것도 하지 않고 `true`를 돌려준다. 서버는 `S_REWARD_RESULT`에 경험치,
골드, 레벨업 결과(`level_up_details`)를 싣지만, 클라이언트의 HUD와 내 플레이어 데이터에는 반영되지 않는다.
지금은 플레이어가 몬스터를 때리는 경로가 없어 보상이 오지 않는다. 코드를 읽고 판단했다.

### 영향

**새 기능 개발 지연** — 공격 판정을 넣으면 몬스터를 잡아도 경험치와 골드가 화면에 바뀌지 않는다. 재접속해야
서버에 저장된 값이 보인다.

## 캐릭터 수 한도가 클라이언트에만 있다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 기능 · server
> 위치: `Server/GameServer/DB/CharacterListDAO.cpp` (`CreateCharacter`) · `P1/Source/P1/UI/Frontend/P1LoginMenuWidget.cpp` (`OnCreateButtonClicked`)
> 등록일: 2026년 10월 1일 · 위치 갱신: 2026년 10월 3일 (#131)

계정당 캐릭터 수는 로그인 메뉴 위젯이 캐릭터 목록 길이를 슬롯 수와 비교해 막을 뿐이다. 게임 서버의 생성 요청은
한도를 보지 않는다. 한도 값이 `WBP_LoginMenu` 디자이너에 놓인 슬롯 수에만 있어서 서버가 참조할 원천도 없다.
코드와 BP 그래프를 읽고 판단했다. #131이 비교를 BP 그래프에서 C++로 옮겼다.

### 영향

**버그 발생 가능성 증가** — 조작한 클라이언트는 캐릭터를 한도 없이 만들 수 있다. 로그인 목록이 슬롯 수보다
길면 넘친 캐릭터는 화면에 나오지 않는다.

## 룸의 X·Y 범위가 반폭을 뒤바꿔 쓴다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/GameServer/Game/Room/Room.cpp` (`CacheRoomData`)
> 등록일: 2026년 10월 1일

`CacheRoomData`는 룸의 X 범위를 `heightHalfExtent`로, Y 범위를 `widthHalfExtent`로 계산한다. 지금 맵 데이터는
두 값이 같아서 드러나지 않는다. 고치려면 기획이 width를 어느 축으로 의도했는지 확인하고, 클라이언트 레벨의 경계
벽과도 맞춰 봐야 한다. 코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 가로세로가 다른 룸을 만들면 몬스터 스폰과 배회, 셀 행렬의 범위가 실제 룸과 어긋난다.

## 스포너가 플레이어 이름을 UTF-8로 풀지 않고 네임플레이트에 넘긴다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · client
> 위치: `P1/Source/P1/Sync/P1EntitySpawner.cpp` 174~175줄 (`AP1EntitySpawner::SpawnPlayer`)
> 등록일: 2026년 10월 2일

스포너는 서버가 보낸 UTF-8 이름을 `FString(const char*)`로 바꾼다. 이 생성자는 바이트를 UTF-8이 아니라
ANSI로 읽는다. 네임플레이트는 `FinishSpawning` 안의 `BeginPlay`에서 이 값을 읽는다. 그 뒤
`AP1Creature::Initialize`가 `UTF8_TO_TCHAR`로 이름을 다시 넣지만, 네임플레이트는 이미 글자를 정한 뒤다.
코드를 읽고 판단했고 실행해서 확인하지는 않았다.

#123에서 찾았다. 그 티켓은 동작을 바꾸지 않는 정리라서 고치지 않았다.

### 영향

**버그 발생 가능성 증가** — 한국어 이름을 쓴 플레이어의 네임플레이트 글자가 깨져 보일 수 있다.

## `AP1Creature`의 assert가 없는 식별자를 가리킨다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · client
> 위치: `P1/Source/P1/Sync/P1MoveSyncComponent.cpp` (`UP1MoveSyncComponent::SetClientPos`). #128이 `AP1Creature::SetClientPos`에서 그대로 옮겼다
> 등록일: 2026년 10월 2일

`assert(SrcInfo->entity_id() == Info.entity_id())`의 `SrcInfo`는 어디에도 선언되어 있지 않다. 이 빌드에서
`assert`가 빈 매크로로 펼쳐지므로 컴파일만 될 뿐이다. 같은 함수의 다른 줄은 `ClientPos`를 쓴다.
이름을 바꾸면서 남은 흔적으로 보인다.

#123에서 찾았다.

### 영향

**유지보수 어려움** — `assert`를 켜는 구성에서는 빌드가 깨진다. 지금은 검사하려던 조건을 아무도 검사하지 않는다.

## 이동 패킷 송신 판정의 회전 비교가 틀린다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · client
> 위치: `P1/Source/P1/Sync/P1MoveSendThrottle.cpp` (`FP1MoveSendThrottle::Decide`)
> 등록일: 2026년 10월 3일

원하는 이동 Yaw와 현재 Yaw의 차이를 두 값의 차의 절댓값으로 잰다. 차이가 허용치(60도) 이상이면 주기를 기다리지
않고 보낸다. 이 비교에서 결함 둘이 생긴다.
- 179도와 -179도는 실제로 2도 차이인데 358도로 잰다
- 이동 입력을 떼면 `AP1MyPlayer::Move`가 0 벡터를 받아 원하는 이동 Yaw가 0이 된다. 캐릭터가 60도 이상이나
  -60도 이하를 바라보고 서 있으면 공격 중이 아닌 동안 매 틱 보낸다

`P1.Sync.MoveSendThrottle`이 첫째 동작을 그대로 고정하고 있다. 고칠 때는 테스트의 기대값도 함께 바꾼다.
#156이 둘을 함께 고친다.

첫째는 #128에서 판정을 떼어 내며, 둘째는 #129에서 코드를 읽으며 찾았다. 실행해서 송신 빈도를 재지는 않았다.

### 영향

**버그 발생 가능성 증가** — ±180도 경계 근처를 바라보며 움직이면 이동 패킷이 의도한 주기(0.2초)보다 자주 나갈 수 있다.
입력 없이 서 있는 동안에는 프레임마다 `C_MOVE`가 나갈 수 있다. 둘 다 실측하지 않았다.

## 쓰이지 않는 레거시 입력 매핑이 남아 있다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · client
> 위치: `P1/Config/DefaultInput.ini` (`ActionMappings`, `AxisMappings`)
> 등록일: 2026년 10월 3일

`Jump` 액션 매핑 2줄과 축 매핑 10줄(`Move Forward / Backward`, `Move Right / Left`, `Turn Right / Left ...`,
`Look Up / Down ...`)이 남아 있다. 입력은 모두 Enhanced Input(`IMC_Default`, `IMC_InGameUI`)으로 받고,
이 이름들을 바인딩하는 코드가 없다. 3인칭 템플릿에서 온 것으로 보인다.

#130에서 화면 단축키 매핑 둘을 지우며 찾았다.

### 영향

**유지보수 어려움** — 입력을 고치려는 사람이 이 매핑이 실제로 쓰이는지 따로 확인해야 한다.

## 생성한 캐릭터의 이름을 응답이 온 시점의 입력 칸에서 읽는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · client
> 위치: `P1/Source/P1/UI/Frontend/P1LoginMenuWidget.cpp` (`UP1LoginMenuWidget::AddCharacterOverview`)
> 등록일: 2026년 10월 3일

캐릭터 생성 응답(`S_CREATE_CHARACTER`)이 오면 로그인 메뉴는 목록에 더할 이름을 그 시점의 이름 입력 칸에서
읽는다. 직업도 그 시점에 고른 값을 쓴다. 요청을 보낸 뒤 응답이 오기 전에 입력을 고치면, 서버에는 보낸 이름과
직업이 저장되고 목록에는 고친 값이 보인다. 응답 패킷에는 캐릭터 id만 있다. 코드를 읽고 판단했고 실행해서
확인하지는 않았다.

#131에서 찾았다. 그 티켓은 동작을 바꾸지 않는 이관이라서 고치지 않았다.

### 영향

**버그 발생 가능성 증가** — 다시 로그인해 목록을 받기 전까지 캐릭터 선택 화면에 서버와 다른 이름이나 직업이 보일 수 있다.

## 모르는 직업 값을 받으면 로그인 메뉴가 멈춘다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · client
> 위치: `P1/Source/P1/UI/Frontend/P1LoginMenuWidget.cpp` (`ClassEnumToStringMappings`를 읽는 `FetchCharacterOverviews`, `SelectClass`, `AddCharacterOverview`)
> 등록일: 2026년 10월 3일

직업 값을 직업 이름으로 바꿀 때 `TMap::operator[]`를 쓴다. 이 연산자는 키가 없으면 `check`로 멈춘다. 맵에는
전사와 마법사만 있으므로, 서버가 캐릭터 목록(`S_LOGIN`)에 `CLASS_TYPE_NONE`이나 새로 더한 직업을 실어 보내면
클라이언트가 멈춘다. 지금 서버는 생성 요청을 `CharacterCreation::Validate`로 걸러 표에 있는 직업만 저장한다.
코드를 읽고 판단했다.

#131에서 찾았다. 그 티켓은 동작을 바꾸지 않는 이관이라서 고치지 않았다.

### 영향

**변경 영향 범위 확대** — 서버 데이터에 직업을 더하면 클라이언트의 이 맵도 함께 고치지 않는 한 로그인 화면에서 멈춘다.

## `P1QuestRewardData.h`가 쓰는 타입의 헤더를 부르지 않는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · client
> 위치: `P1/Source/P1/Game/Data/P1QuestRewardData.h` 18줄
> 등록일: 2026년 10월 2일

`TArray<FP1ItemData>` 멤버를 두면서 `Game/Data/P1ItemData.h`를 부르지 않는다. 이 헤더보다 먼저
`P1ItemData.h`를 부른 파일이 있어야 컴파일된다.

#123에서 찾았다.

### 영향

**변경 영향 범위 확대** — 다른 파일의 include를 정리하거나 유니티 빌드 묶음이 바뀌면 관계없어 보이는 곳에서
빌드가 깨진다.

## 헤더에 쓰지 않는 include가 남아 있다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 모듈 · client
> 위치: `P1/Source/P1/`
> 등록일: 2026년 10월 2일

#123은 `.cpp`의 쓰지 않는 include만 지웠다. 아래 헤더의 include는 그 헤더 자신은 쓰지 않지만, 그 헤더를
부르는 파일이 전이적으로 기대고 있을 수 있어서 남겼다. 유니티 빌드가 누락을 가릴 수 있으므로 하나씩
지우고 유니티 빌드를 끈 빌드로 확인해야 한다.

| 헤더 | 쓰지 않는 include |
|---|---|
| `Network/P1SendWorker.h` | `Containers/Queue.h` |
| `UI/WorldSpace/P1NameplateWidget.h` | `Utils/Types.h` |
| `Game/Inventory/P1Inventory.h` | `Game/Data/P1ItemData.h` |
| `UI/Screens/P1HUDWidget.h` | `Protocol.pb.h` |

`P1Inventory.h`의 `P1ItemData.h`는 위 `P1QuestRewardData.h` 항목과 엮여 있을 수 있다.

### 영향

**변경 영향 범위 확대** — 헤더 하나를 고치면 그 헤더를 부르는 파일이 모두 다시 컴파일된다.

## 상점 판매 목록과 상점 위치를 서버가 보지 않는다
> **심각도:** 낮음 · **난이도:** 중간 · **범위:** 기능 · server
> 위치: `Server/GameServer/Game/Entities/Player.cpp` (`ProcessBuyItem`, `ProcessSellItem`)
> 등록일: 2026년 10월 1일

게임 서버에는 상점이 파는 아이템의 목록이 없어서, 아이템 표에 있는 아이템은 모두 살 수 있다. 플레이어가 상점
근처에 있는지도 보지 않으므로 어디서나 사고팔 수 있다. 상점 판매 목록은 기획 데이터에 없어서 원천부터 정해야
한다. 위치 검증은 「룸 이동 요청이 플레이어의 위치를 보지 않는다」와 같은 논의(클라이언트가 상호작용을 알리고
서버가 믿는 구조)에 들어간다. 코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 조작한 클라이언트는 상점에 없는 아이템을 사고, 상점에 가지 않고 거래할 수 있다.

## 룸 이동 요청이 플레이어의 위치를 보지 않는다
> **심각도:** 낮음 · **난이도:** 중간 · **범위:** 기능 · server
> 위치: `Server/GameServer/Game/Room/RoomTransfer.cpp` · `Server/GameServer/Game/Room/Room.cpp` (`C_HandleEnterMap`)
> 등록일: 2026년 10월 1일

포털 이동은 포털 번호가 현재 룸에 있는지만 보고, 플레이어가 그 포털 근처에 있는지는 보지 않는다. 맵 간 이동의
목적지는 `C_ENTER_MAP`이 받아 두는데, `C_HandleEnterMap`은 아무 룸 번호나 받는다. 코드를 읽고 판단했다.

고치기 전에 포털을 쓰는 지금 방식이 근본적으로 맞는지 함께 논의해야 한다. 클라이언트가 포털을 밟았다고
알리고 서버가 믿는 구조인데, 서버가 포털 반경을 판정하려면 포털 위치와 반경의 원천을 서버에 두어야 한다.
맵 간 이동이 `C_ENTER_MAP`과 `C_ENTER_ROOM` 두 요청으로 나뉜 것도 같은 논의에 들어간다.

### 영향

**버그 발생 가능성 증가** — 조작한 클라이언트는 룸 안 어디서든 포털을 타고, 아무 룸으로나 맵 간 이동을 할 수 있다.

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
