# Tech Debt

지금 틀린 것만 담는다. 해결이 확정되면 항목을 지운다 — 수정 완료 표기를 남기지 않는다.
무엇을 어떻게 고쳤는지는 커밋이 갖는다.

항목 14개 (높음 1 · 중간 0 · 낮음 13)

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
- 항목을 고칠 GitHub 이슈에는 `tech-debt` 라벨을 붙인다. 이슈를 먼저 열고 나중에 항목을 적었으면 그때 붙인다

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

## 응답 대기 플래그 이름에 `b` 접두사가 없다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 모듈 · client
> 위치: `P1/Source/P1/UI/Screens/` (`P1InventoryWidget.h` · `P1ShopWidget.h` · `P1StatusWindowWidget.h`)
> 등록일: 2026년 10월 3일

인벤토리, 상점, 스탯 창 위젯의 `bool PendingPacket`이 `docs/conventions.md` 2.1의 「bool 변수 `b`」를 따르지
않는다. `Content`의 에셋 가운데 이 이름을 담은 것은 없으므로, 이름을 바꿔도 리다이렉트는 필요 없다.
규범 검사(`check_conventions.py`)는 이 규칙을 잡지 않는다.

### 영향

**동일한 문제의 반복** — 새 위젯이 같은 이름으로 대기 플래그를 베껴 쓴다.

## 인벤토리 칸 변경 알림의 `OnUse` 인자를 읽는 곳이 없다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 모듈 · client
> 위치: `P1/Source/P1/Game/Progress/P1MyPlayerData.h` (`FOnInvenSlotChanged`) · `P1/Source/P1/Game/Inventory/P1Inventory.h` (`Rep_SlotChanged`) · `P1/Source/P1/UI/Screens/P1InventoryWidget.h` (`UpdateSlotWidget`)
> 등록일: 2026년 10월 3일

`FOnInvenSlotChanged`는 칸과 함께 `bool`을 싣는다. 사용 응답이면 참이다. 재사용 대기를 내 플레이어 데이터가
아이템마다 세게 되면서, 이 값을 읽던 슬롯의 `OnUse` 호출이 사라졌다. 구독자 둘(`UP1Inventory::Rep_SlotChanged`,
`UP1InventoryWidget::UpdateSlotWidget`)은 인자를 받기만 하고 읽지 않는다.

### 영향

**유지보수 어려움** — 인자 이름만 보면 사용 여부로 무언가를 하는 것처럼 읽힌다.

## 아이템 데이터 테이블을 가리키는 곳이 둘이다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 기능 · client
> 위치: `P1/Source/P1/UI/Common/P1SlotWidget.h` (`ItemTable`) · `P1/Source/P1/Game/Data/P1GameDataSettings.h` (`ItemTable`)
> 등록일: 2026년 10월 3일

슬롯 위젯은 블루프린트 기본값으로 지정한 `ItemTable`에서 아이템 정의를 읽고, 내 플레이어 데이터는
`UP1GameDataSettings`가 `DefaultGame.ini`에서 가리키는 테이블에서 읽는다. 지금은 둘 다 `DT_Item`이다. 템플릿 id로
행 이름을 만들어 찾는 코드도 두 곳에 있다.

### 영향

**버그 발생 가능성 증가** — 한쪽만 다른 테이블로 바꾸면 슬롯이 보이는 아이템 정보와 재사용 대기 길이가 어긋난다.

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

## 캐릭터 슬롯 수를 서버 상수와 로그인 메뉴의 위젯 수로 따로 정한다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 기능 · server
> 위치: `Server/GameServer/Game/Entities/CharacterCreation.h` 16줄 (`DEFAULT_CHARACTER_SLOT_COUNT`) · `P1/Source/P1/UI/Frontend/P1LoginMenuWidget.cpp` (`OnCreateButtonClicked`)
> 등록일: 2026년 10월 4일

서버는 생성 요청을 `DEFAULT_CHARACTER_SLOT_COUNT`(4)로 막고, 클라이언트는 `WBP_LoginMenu` 디자이너에 놓인
슬롯 위젯 수로 막는다. 두 값을 맞춰야 한다는 사실은 양쪽 주석에만 있고, 어긋나도 빌드나 테스트가 알려 주지 않는다.
서버가 슬롯 수를 패킷으로 내려 주는 방식은 프로토콜을 바꿔야 해서 「캐릭터 수 한도가 클라이언트에만 있다」를
고칠 때 범위에서 뺐다.

### 영향

**버그 발생 가능성 증가** — 서버 값만 늘리면 넘친 캐릭터가 로그인 화면에 나오지 않고, 위젯만 늘리면 빈 슬롯이
보이는데도 서버가 생성을 거절한다.

## 캐릭터 생성 쿼리의 거절 표시를 SQL과 C++가 따로 적는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/GameServer/DB/CharacterListDAO.cpp` 103~104줄, 190줄, 218줄, 247줄 (`CreateCharacter`)
> 등록일: 2026년 10월 4일

생성 쿼리는 거절할 때 `character_id` 자리에 `-1`(이름 중복)이나 `-2`(빈 슬롯 없음)를 리터럴로 돌려준다. C++는
같은 값을 `DUPLICATE_NAME`과 `NO_EMPTY_SLOT` 상수로 따로 적는다. 화면에 사유를 보여 줄 오류도 catch 블록이
`ALREADY_EXISTING_CHARACTER || NO_EMPTY_CHARACTER_SLOT`처럼 하나씩 나열한다. 「캐릭터 수 한도가 클라이언트에만 있다」를 고친 작업의 코드 리뷰에서 찾았다.

### 영향

**동일한 문제의 반복** — 거절 사유를 더할 때마다 SQL 리터럴, C++ 상수, catch 조건 세 곳을 함께 고쳐야 하고,
한 곳을 빠뜨리면 사유가 「서버 내부 오류」로 바뀌어 보인다.

## 아이템 기획 원본의 분류 열 이름이 한 단계씩 밀려 있다
> **심각도:** 낮음 · **난이도:** 중간 · **범위:** 기능 · protocol
> 위치: `DesignData/Original_Item.xlsx` · `P1/Source/P1/Game/Data/P1ItemData.h` · `Server/GameServer/Game/Inventory/Inventory.cpp` (`ToItemType`)
> 등록일: 2026년 10월 3일

`CONTEXT.md`의 아이템 분류는 세 단계다. 아이템 종류(장비·소모품·기타), 아이템 분류(방어구·무기·소비), 그리고
장비 부위나 물약 같은 셋째 단계다. 기획 원본과 두 티어의 이름은 이와 한 단계씩 어긋난다.

| 단계 | 기획 원본의 열 | 값 |
|---|---|---|
| 아이템 종류 | 없음. 서버가 `ToItemType`으로 분류에서 만든다 | — |
| 아이템 분류 | `ItemType` | `armor` · `weapon` · `consumption` |
| 셋째 단계 | `ItemSubtype` | `helmet` · `sword` · `potion` 등 |

클라이언트의 `FP1ItemData::ItemType`도 분류를 담는다. 클라이언트는 아이템 종류를 데이터에서 얻지 못하므로
슬롯 종류로 가른다. 기획 원본에 종류 열을 더하는 안은 `docs/backlog.md`에 있다.

### 영향

**버그 발생 가능성 증가** — `ItemType`이라는 이름을 보고 아이템 종류로 읽으면, 프로토콜의 `ItemType`
(`GEAR`·`CONSUMABLE`·`MISCELLANEOUS`)과 값이 맞지 않는다. 블루프린트가 이 문자열을 `Switch on String`으로
가르던 것이 그 예다.

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
