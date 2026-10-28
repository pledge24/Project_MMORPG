# 폴더 구조

무엇을 어디에 둘지를 다룬다. 저장소 최상위, 클라이언트의 `Source`와 `Content`, 서버 트리,
에셋 이름, 그리고 버전 관리 대상이 여기 속한다.
**코드 안의 규칙은 `.claude/rules/`의 티어별 파일이 다룬다.**

이 문서는 **목표 상태**를 적는다. 현행 코드가 아직 이 문서와 다른 자리가 있다. 그럴 때는 이
문서가 맞고 코드가 틀린 것으로 본다.

---

## 1. 분류 원칙

**`Content`는 대상 중심으로, `Source`는 도메인 중심으로 나눈다.** 두 방식의 목표는 같다. 함께
변경되는 것끼리 묶는 것이다. 몬스터 하나를 추가할 때는 메시와 머티리얼과 애니메이션이 함께
바뀌므로 대상 단위로 묶는다. 데미지 표시를 고칠 때는 여러 클래스가 함께 바뀌므로 규칙 단위로
묶는다.

**함께 지워져야 하는 것은 같은 폴더에 둔다.** 어디에 둘지 애매하면 개별 폴더에 먼저 둔다. 두
번째로 쓰이는 시점에 가장 가까운 `Common/`으로 올린다.

**클라이언트 `Source`와 게임 서버에서 같은 분류명을 쓴다.** 인벤토리를 고칠 때
`P1/Source/P1/Game/Inventory/`와 `Server/GameServer/Game/Inventory/`를 함께 열게 되기 때문이다. 양쪽에
모두 있는 도메인만 맞추고, 한쪽에만 있는 도메인의 상대 폴더는 만들지 않는다.
— 판정 기준과 기각한 대안은 `docs/adr/0007-limit-folder-symmetry-to-shared-domains.md`에 있다.

**`Content`는 `Source`의 분류명을 빌려 쓰되 강제하지 않는다.** 기준은 `Source`를 보며 작업할 때
혼동을 주지 않는 것이다. 지금 빌리지 않는 자리는 하나다. `Source`가 엔티티 폴더를
`Game/Entities/`로 불러도 `Content`는 `Characters/`로 부른다.

**쓰지 않는 폴더를 미리 만들지 않는다.** 내용이 생기는 날 만든다. 예외는 `Content`의 예정된
묶음이다. 에셋 이동이 에디터를 거쳐야 해서 나중에 옮기는 비용이 크다.
— 빈 폴더는 무엇을 넣어야 하는지 아무도 모른 채 남는다. 예외의 근거는
`docs/adr/0008-split-ui-by-presentation.md`에 있다.

**폴더 이름은 영어, 공백 없음, PascalCase로 짓는다.** 담는 대상이 여럿인 폴더는 복수형으로 쓴다
(`Monsters`, `Weapons`). `Core`, `Data`, `UI`처럼 개념 이름인 폴더는 그대로 둔다.

---

## 2. 저장소 최상위

```
Project_MMORPG/
├── P1/                     언리얼 클라이언트 (.uproject, Content/, Source/, Config/)
├── Server/                 외부 C++ 게임 서버와 Node 인증 서버
├── Protocol/               클라이언트와 서버가 공유하는 패킷 계약
│   ├── Schema/             .proto 원본. 도메인마다 한 파일
│   ├── Templates/          핸들러 헤더 템플릿
│   └── Generate.ps1        생성 스크립트
├── DesignData/             기획 수치 원본. 클라이언트와 서버가 함께 읽는다
├── Tools/                  생성기 소스와 저장소 전체를 보는 검사 스크립트
└── docs/                   문서
```

**한 티어 안에서만 도는 스크립트는 `Tools/`에 두지 않는다.** 클라이언트의 빌드와 테스트 실행은
`P1/Scripts/`가 맡는다. `Tools/`에 두는 것은 저장소 전체를 대상으로 삼거나 두 티어가 함께 쓰는
것이다. 규범 검사(`Tools/ConventionLint/`)와 문서 검사(`Tools/DocLint/`)가 앞에, 기획 데이터 생성기(`Tools/DataBuilder/`)와 패킷 핸들러
생성기가 뒤에 해당한다.

`Server/`와 `Protocol/`이 언리얼 프로젝트 **바깥에** 있는 이유는 둘이다. 첫째, 언리얼의 쿠킹과
패키징 대상에서 자연히 빠진다. 둘째, 서버를 빌드할 때 언리얼 빌드 도구가 필요 없어진다.

클라이언트 폴더 이름이 역할(`Client`)이 아니라 프로젝트 이름(`P1`)인 이유는
`docs/adr/0004-partially-relayout-repository-root.md`에 있다. 같은 문서가 `Shared/`를 만들지 않는
이유도 담는다.

**이 구조는 바꾸지 않는다.** 최상위 경로를 옮기면 CI 워크플로, 훅, 테스트 스크립트, 문서의 경로
참조를 모두 고쳐야 한다.

---

## 3. 클라이언트 Source

### 3.1 모듈 구성

```
P1/Source/
├── P1.Target.cs
├── P1Editor.Target.cs
├── P1/                     런타임 게임 모듈
├── P1Editor/               에디터 모듈. Validation/ 에셋 검증기, Tests/ 그 테스트
└── ProtobufCore/           프리빌트 protobuf 링크 (ModuleType.External)
```

런타임 모듈과 에디터 모듈은 Public과 Private을 나누지 않는다. 도메인 폴더에 `.h`와 `.cpp`를 함께 둔다.
다른 모듈에 공개할 API가 생기는 시점에 그 모듈만 나눈다.

**도메인을 별도 모듈로 나누지 않는다.** 수작업 `.h`·`.cpp`가 130개 남짓인 지금 규모에서는 빌드
구성만 늘고 얻는 것이 적다. 도메인을 넘는 `#include`가 계속 늘어나면 그때 검토한다.

### 3.2 도메인 폴더

**`Game/`이 게임 도메인과 배선을 가른다.** 모듈 루트 바로 아래의 `Core/`, `Network/`, `Sync/`,
`Auth/`, `Lobby/`, `UI/`, `Utils/`, `Tests/`는 클라이언트를 돌리는 배선이고, `Game/` 아래는 게임 규칙이다.
— 서버도 같은 층으로 가른다. 분류명이 양쪽에서 같으므로 `P1/Source/P1/Game/Inventory/`를 알면
`Server/GameServer/Game/Inventory/`를 찾는 데 지장이 없다.

| 폴더 | 담는 것 |
| --- | --- |
| `Core/` | GameInstance, GameMode, GameState, PlayerController, PlayerState, 입력 바인딩, 로그인부터 게임까지의 단계와 레벨 전환을 갖는 접속 흐름 서브시스템(`UP1EntryFlowSubsystem`) |
| `Network/` | 소켓, 세션 수명, 송수신 버퍼, 패킷 디스패치. `Server/ServerCore/Network/`와 대칭을 유지한다. 쓰이지 않는 함수도 지우지 않는다 |
| `Sync/` | 서버 엔티티 ID와 언리얼 액터의 대응 관리, 스폰과 소멸, 원격 개체 보간, 로컬 예측과 서버 보정, 서버 시간 추정 |
| `Auth/` | 인증 서버와의 HTTP(회원가입, 로그인, 월드 목록, 월드 선택)를 맡고 로그인 세션과 월드 목록의 사본을 갖는 인증 클라이언트(`UP1AuthClient`). 게임 서버를 모른다 |
| `Lobby/` | 로비 접속, 캐릭터 목록과 슬롯 수의 사본, 캐릭터 생성·삭제·선택 요청을 맡는 로비 서브시스템(`UP1LobbySubsystem`)과 로비 패킷의 핸들러. 서버의 `Lobby/` 모듈과 이름을 맞춘다 |
| `UI/` | 위젯 베이스 클래스와 뷰모델. 하위를 표시 방식으로 나눈다 |
| `Utils/` | 함수 라이브러리, 로그 카테고리 선언, 공용 매크로 |
| `Game/Entities/` | 베이스 캐릭터와 플레이어, 몬스터, NPC 캐릭터 클래스 |
| `Game/Combat/` | 전투 요청 생성과 결과 연출. **판정은 여기 두지 않는다** |
| `Game/Inventory/` | 인벤토리 컴포넌트와 슬롯 규칙 |
| `Game/Equipment/` | 장비 컴포넌트와 장착 규칙 |
| `Game/Items/` | 아이템과 칸의 상태 `USTRUCT`, 컨테이너 클래스, 아이템과 칸의 패킷 변환 |
| `Game/Data/` | DataTable 행 `USTRUCT`, DataAsset 클래스, 게임 코드가 읽는 데이터 테이블을 가리키는 설정(`UP1GameDataSettings`). 하위 `Generated/`는 DataBuilder가 만드는 기획 데이터 행 구조체와 열거형이다 |
| `Game/World/` | 레벨에 배치하는 액터. 포털, 경계 벽 |
| `Game/Interaction/` | 플레이어가 다가가 쓰는 레벨 액터와 그 액터가 화면에 알리는 인터페이스. 상점 |
| `Game/Progress/` | 내 플레이어의 진행(레벨, 스탯, 골드, 소지품과 장비, 퀵슬롯)을 들고 있는 서브시스템과, 그 소지품과 퀵슬롯에 대한 요청을 판정하고 보내는 아이템 요청 창구 |
| `Game/QuickSlots/` | 퀵슬롯과 그 대상의 `USTRUCT`, 퀵슬롯 사본 클래스, 퀵슬롯의 패킷 변환 |
| `Game/Chat/` | 채팅 줄 `USTRUCT`, 받은 채팅을 보관하고 채팅을 보내는 채팅 서브시스템, 채팅의 패킷 변환 |

**`Sync/`가 엔티티 대응 관리와 스폰을 함께 담는다.** 서버가 보낸 엔티티를 액터로 세우는 일과 그
액터를 서버 상태에 맞추는 일은 같은 일의 양면이라 함께 바뀐다.
— 두 폴더로 나누면 스폰 하나를 고칠 때 양쪽을 함께 열게 된다.

**`Game/World/`는 상호작용 대상이 아닌 액터만 담는다.** 상점처럼 플레이어가 골라서 다가가 쓰는 대상은
`Game/Interaction/`에 둔다.
— 포털은 밟으면 맵 이동을 요청하고 경계 벽은 통과를 막는다. 둘 다 플레이어가 고르는 대상이
아니라 레벨이 놓아 둔 장치다.

**`UI/` 하위는 표시 방식으로 나눈다.** 게임 도메인으로 나누지 않는다.

| 하위 폴더 | 담는 것 | 판정 조건 |
| --- | --- | --- |
| `Common/` | 다른 위젯이 품는 조각 | 다른 위젯이 품는가 |
| `Frontend/` | 게임 입장 전 화면 | 게임에 입장하기 전에 뜨는가 |
| `Screens/` | 플레이 중 띄우는 화면 | 화면 서브시스템(`UP1ScreenSubsystem`)이 만들어 띄우는가 |
| `Loading/` | 레벨 전환을 건너 화면을 덮는 로딩 화면 | 레벨이 바뀌어도 화면에 남아야 하는가 |
| `WorldSpace/` | 레벨 공간에 붙는 위젯 | 화면이 아니라 레벨 안의 좌표를 따라가는가 |

`Frontend/`의 화면은 로그인 맵의 로그인 메뉴(`P1LoginMenuWidget`)가 품는다. 로그인, 월드 선택, 캐릭터 선택, 캐릭터 생성
화면은 화면 전환기로 하나씩 보이고, 캐릭터 삭제 확인 창은 그 위에 덮는다.
화면마다 C++ 클래스와 WBP가 한 쌍이다(`P1LoginWidget`과 `WBP_Login`). 캐릭터 선택 화면의 슬롯(`P1CharacterSlotWidget`)과
월드 선택 화면의 줄(`P1WorldRowWidget`)은 그 화면만 품으므로 `Common/`이 아니라 `Frontend/`에 있다.

`Loading/`의 접속 진행 표시(`P1EntryProgressWidget`)는 로그인 메뉴가 품지 않는다. 로딩 화면 서브시스템(`P1LoadingScreenSubsystem`)이
게임 인스턴스에서 만들어 게임 뷰포트에 직접 붙인다. 레벨에 속한 위젯은 레벨이 바뀌면 함께 내려가서, 맵을 여는 동안과
새 맵에서 내 플레이어가 스폰되기 전까지 화면을 덮을 수 없다. 서브시스템도 그 위젯만 다루므로 같은 폴더에 둔다.

`Common/`의 조건은 품는 곳의 수를 보지 않는다. 조각을 품는 위젯이 하나뿐이면 1절의 원칙대로
그 위젯 가까이에 둔다. `P1ItemTooltipWidget`을 품는 것은 `P1SlotWidget` 하나뿐인데, 그 슬롯
위젯이 `Common/`에 있으므로 툴팁도 `Common/`에 있다.
— `Screens/`의 조건은 HUD처럼 늘 떠 있는 화면도 참으로 판정한다. 화면 서브시스템이 창 종류
열거형으로 여닫는 것은 스탯 창과 인벤토리와 상점 셋뿐이고, HUD와 도움말과 경고 문구와 사망 화면과 게임 메뉴는
서브시스템의 개별 멤버로 다뤄진다. 창을 담는 레이어 위젯(`P1WindowLayerWidget`)도 서브시스템이 띄우므로
여기 있다.

`Styles/`는 `Content`에만 만들고 폰트만 담는다. 폰트는 `Source`에 짝이 없다.
— 도메인으로 나누지 않는 이유와 `HUD/`를 분류 이름으로 쓰지 않는 이유는
`docs/adr/0008-split-ui-by-presentation.md`에 있다.

**위젯 하나만 쓰는 텍스처는 그 위젯이 있는 폴더의 `Textures/`에 둔다.** `WBP_LoginMenu`만 쓰는
버튼 이미지는 `Frontend/Textures/`에, `WBP_HUD`만 쓰는 이미지는 `Screens/HUD/Textures/`에 둔다.
두 위젯 이상이 쓰게 되면 `Common/Textures/`로 옮긴다.
— 4.2의 「에셋 하나가 폴더 하나로 완결된다」를 위젯에 적용한 것이다. 위젯을 지울 때 그 폴더의
`Textures/`에서 이 위젯이 쓰던 것을 찾으면 된다.
— 공유 텍스처를 `Styles/`가 아니라 `Common/`에 두는 것은 `Common/`이 이미 공유를 뜻하기 때문이다.
둘로 나누면 공유 위젯은 `Common/`에, 공유 텍스처는 `Styles/`에 있게 되어 공유를 찾을 곳이 둘이 된다.

**`AI/`와 `Abilities/`를 만들지 않는다.** 몬스터의 행동은 게임 서버가 결정하고, 스킬 판정도
서버가 한다. 클라이언트가 받는 것은 결과 상태뿐이다.

새 기능이 위 표의 어디에도 맞지 않으면 도메인 폴더를 새로 만든다. 기존 폴더에 억지로 넣지
않는다.

### 3.3 의존 방향

도메인끼리 서로 참조하면 경계가 무너진다. 각 폴더는 아래 표에 적힌 곳만 부른다. 게임 도메인은
`Game/Combat`, `Game/Inventory`, `Game/Equipment`, `Game/Progress`, `Game/QuickSlots`, `Game/Chat`이다.

| 폴더 | 부를 수 있는 곳 |
| --- | --- |
| `Core/`, `UI/` (배선) | 서로, 그리고 아래의 모든 폴더 |
| `Auth/` | `Network/`의 설정(`P1NetworkSettings.h`), `Utils/` |
| `Lobby/` | `Network/`의 송신 창구와 생성된 핸들러 헤더, 세션(`PacketSession.h`), `Game/Progress`(받은 입장 응답을 넘기는 패킷 핸들러), `Utils/` |
| `Network/` | `Sync/`, `Game/Progress`와 `Game/Chat`(받은 패킷을 넘기는 패킷 핸들러), `Utils/` |
| `Sync/` | `Game/Entities`, 게임 도메인, `Game/Data`, `Utils/`, 송신 창구 |
| `Game/World`, `Game/Interaction` | `Game/Entities`, `Utils/`, 송신 창구 |
| `Game/Entities` | 게임 도메인, `Game/Data`, `Utils/`, 송신 창구 |
| 게임 도메인 | `Game/Items`, `Game/Data`, `Utils/`, 송신 창구 |
| `Game/Items` | `Utils/` |
| `Game/Data` | `Utils/` |
| `Utils/` | 없음 |
| `Tests/` | 테스트하는 대상 폴더 전부. 제품 코드는 `Tests/`를 부르지 않는다 |

이 표는 #138이 클라이언트의 `#include`를 전수 대조한 결과에 맞춘 것이다.

**생성된 프로토콜 자료형(스키마 파일마다 생기는 `*.pb.h`)은 어디서나 부른다.** 생성기가
`Network/`에 복사해 둘 뿐 `Network/`의 코드가 아니다. 요청을 만드는 쪽도 응답을 받는 쪽도 이 자료형을 쓴다.

**`Sync/`와 `Game/` 아래가 `Network/`에서 부르는 것은 송신 창구 `Network/P1PacketSender.h` 하나뿐이다.**
표의 「송신 창구」가 이것이다. 요청 자료형(`Protocol::C_*`)을 만들어 `FP1PacketSender::Send(this, Pkt)`에
넘긴다. 게임 인스턴스, 세션, 소켓은 그 헤더에 드러나지 않는다.
— 이 경계가 무너지면 통신 방식을 바꿀 때 게임 코드 전체를 함께 고쳐야 한다.

**`Core/`와 `UI/`는 배선이다. 배선끼리는 서로 부른다.** 위젯은 서브시스템을 얻으려고 게임 인스턴스를 부르고,
게임 인스턴스와 컨트롤러는 화면을 띄우려고 위젯을 부른다. 게임 규칙은 둘 어느 쪽에도 두지 않는다.

**게임 도메인은 `Core/`와 `UI/`를 부르지 않는다.** 배선에 알려야 할 일은
델리게이트로 알리고 배선이 구독한다(예: `AP1MyPlayer::OnBattleModeChanged`를 컨트롤러가
`OnPossess`에서 화면 서브시스템에 이어 준다). 배선의 객체를 도메인이 호출해야 하면 `Game/`에 인터페이스를 두고 배선이
구현한다(예: 네임플레이트 위젯이 구현하는 `IP1CreatureBoundWidget`, 인게임 컨트롤러가 구현하는 `IP1ShopScreen`).

**레벨에 놓인 액터(`Game/World/`, `Game/Interaction/`)는 `Game/Entities/`를 읽을 수 있다.** 들어온 액터가
내 플레이어인지, 그 상태가 어떤지를 보고 반응하기 때문이다. 포털처럼 폰의 일반 속성으로 가릴 수 있으면 `Game/Entities/`를 부르지 않는다.

**`#include` 없이 포인터나 소프트 참조의 타입으로만 쓰는 전방 선언은 방향을 따지지 않는다.** 그 타입의
멤버를 쓰려면 헤더를 불러야 하므로 그때는 표를 지킨다. `Game/Progress/P1MyPlayerData.h`는 스폰
알림(`OnMyPlayerSpawned`)의 인자로 `AP1MyPlayer`를 전방 선언한다.
— 알림을 받는 쪽이 그 액터를 쓰고, 보내는 쪽은 이름만 넘긴다. 이 알림을 다른 폴더로 옮기면 구독하는
화면의 경로까지 바뀐다.

**`Game/Items`는 게임 도메인 아래 층이다.** 인벤토리의 구역과 착용 장비가 같은 칸 타입과 컨테이너 클래스를 쓰므로,
그 타입을 한 도메인에 두면 다른 도메인이 같은 층을 부르게 된다. 아이템과 칸의 Protocol 타입은 `Game/Items`의 패킷 변환에서만 다룬다.

**`Game/Progress`는 아이템의 재사용 대기를 소유하고 아이템 요청을 판정하므로 `Game/Inventory`를 부른다.** 아이템 요청 창구가 인벤토리 칸의 사용·착용 판정을 그대로 쓴다.
같은 이유로 `Game/Progress`는 퀵슬롯 사본을 소유하고 퀵슬롯 등록 요청을 보내므로 `Game/QuickSlots`를 부른다. `Game/QuickSlots`는 `Game/Progress`를 부르지 않는다.
이 절 끝의 「같은 층의 도메인끼리 호출하는 자리는 인터페이스나 델리게이트로 연결한다」의 예외다.
— 소유 관계와 상태가 없는 판정 함수는 인터페이스로 끊을 이유가 없다.

**`Utils/`는 `Game/` 아래를 부르지 않는다.** 배선이 게임 규칙을 부르면 화살표가 뒤집힌다.
공용 매크로를 쓰려고 `Utils/`의 헤더를 여는 자리가 게임 도메인 전체를 딸려 끌고 온다.
— `Utils/Types.h`가 `Data/`와 `Entities/`를 부르던 두 줄을 #73이 지웠다. 그 두 헤더가 필요한
자리는 각자 직접 부른다.

**`Game/` 아래는 `Auth/`와 `Lobby/`를 부르지 않는다.** 로비에서 쓰는 자료형은 `Lobby/`가 소유한다. 캐릭터 목록의
항목 `FP1LobbyCharacter`와 직업 `EP1CharacterClass`가 `Game/Data/`가 아니라 `Lobby/`에 있는 이유가 이것이다.
`Lobby/`의 입장 응답 핸들러는 입장한 캐릭터의 정보를 넘기려고 `Game/Progress`를 부른다.

**인증 클라이언트, 로비 서브시스템, 연결 서브시스템은 서로의 함수를 부르지 않는다.** 폴더 사이의 의존이 아니라 클래스 사이의
호출을 말한다. 로비 서브시스템은 위 표대로 송신 창구로 패킷을 보낼 뿐 연결 서브시스템의 함수를 부르지 않는다. 로그인 성공, 연결 성공과 실패, 로비 입장,
게임 입장 성공은 각자 델리게이트로 알리고, 접속 흐름 서브시스템(`Core/`)이 구독해 다음 단계로 잇는다. 단계 사이의 순서는
접속 흐름 서브시스템만 안다.
— 한 단계를 바꿀 때 그 단계의 클래스와 접속 흐름 서브시스템만 고친다.

**로그인 화면 위젯은 protobuf와 송신 창구를 모른다.** 위젯은 인증 클라이언트와 로비 서브시스템의 함수를 부르고,
로비가 알리는 `FP1LobbyCharacter`와 델리게이트로 화면을 그린다. 그래서 `Auth/`와 `Lobby/`의 헤더는 protobuf 헤더를
부르지 않고 쓰는 타입을 전방 선언한다.

**`Sync/`는 판정하지 않고 표현만 맞춘다.** 예측을 구현하더라도 그 결과는 화면 표현일 뿐이고,
서버 응답이 오면 서버 값으로 덮어쓴다.

같은 층의 도메인끼리 호출하는 자리는 인터페이스나 델리게이트로 연결한다.

### 3.4 Build.cs 의존성

필요해진 시점에 추가한다. 규범을 근거로 쓰지 않는 모듈을 미리 넣지 않는다.
— 쓰지 않는 의존성이 들어 있으면 그것을 써도 된다는 신호로 읽힌다.

언리얼 리플리케이션을 쓰지 않으므로 `NetCore`를 넣지 않는다. AI를 서버가 맡으므로 `AIModule`과
`NavigationSystem`을 넣지 않는다.

---

## 4. 클라이언트 Content

### 4.1 트리

```
P1/Content/
├── P1/                     게임이 참조하는 에셋 전부
│   ├── Core/               GameMode, PlayerController, GameInstance
│   │   └── Input/          Input Action과 Mapping Context
│   ├── Characters/
│   │   ├── Player/
│   │   │   ├── Animations/ 공격 시퀀스와 몽타주
│   │   │   └── Mannequins/ 팩에서 가져온 플레이어 메시
│   │   └── Monsters/       모델마다 폴더 하나. 몬스터 종류는 에셋 테이블의 행이다
│   │       ├── Common/     공용 블루프린트(`BP_Monster`), 공용 스켈레톤, 마스터 머티리얼과 머티리얼 함수
│   │       ├── MeleeMinion/
│   │       ├── RangedMinion/
│   │       └── SuperMinion/
│   ├── Items/
│   │   ├── Icons/
│   │   └── Equipment/
│   │       ├── Armors/     레벨별 폴더를 그대로 유지한다
│   │       └── Weapons/    검별 폴더(`Sword0R`~`Sword50R`)를 둔다
│   ├── UI/
│   │   ├── Common/         여러 화면이 공유하는 부품과 텍스처
│   │   ├── Frontend/       게임 입장 전 화면
│   │   ├── Screens/        플레이 중 여는 화면
│   │   │   └── HUD/        상시 표시 요소와 그 전용 파츠
│   │   ├── Loading/        레벨 전환을 건너 덮는 로딩 화면
│   │   ├── WorldSpace/     레벨 공간에 붙는 위젯
│   │   └── Styles/         폰트
│   ├── Data/
│   │   ├── DataTables/     에셋 테이블(`DT_ItemAssets`, `DT_MonsterAssets`)
│   │   └── DesignData/     DataBuilder JSON으로 채우는 기획 데이터 테이블(`DT_<표>`)
│   ├── Maps/
│   └── World/              레벨에 배치하는 액터
├── External/               참고용 외부 팩. 게임이 참조하지 않는다
├── __ExternalActors__/     에디터 자동 생성물
└── Developers/             개인 실험 공간
```

**`P1/` 아래에는 자작 에셋과 팩에서 가져온 복사본이 함께 있다.** 가르는 기준은 만든 사람이 아니라
게임이 참조하는가다 (ADR-0006). 팩에서 쓸 것을 정하면 `P1/` 아래로 복사하고 게임은 복사본만
참조한다. `External/`은 무엇을 쓸지 고르는 자리이고 추적하지 않는다.

**팩에서 가져온 것은 이름을 바꾸지 않는다.** 접두사 규칙은 자작 에셋에만 적용한다. 이름이 팩과
같아야 다음에 같은 팩에서 무언가를 더 가져올 때 짝을 찾는다. 폴더 배치는 팩의 구조가 아니라
4.2를 따른다.

`Developers/` 아래의 에셋을 게임 에셋이 참조하면 안 된다. 쓸 만한 결과물은 정식 폴더로 옮긴 뒤
참조한다.

### 4.2 배치 규칙

- **캐릭터 하나가 폴더 하나로 완결되게 만든다.** 전용 메시, 머티리얼 인스턴스, 텍스처,
  애니메이션을 그 폴더에 모두 둔다
- **`Common/`은 그 폴더 안에서 공유되는 것만 담는다.** `Characters/Common`은 모든 캐릭터가,
  `Characters/Player/Common`은 모든 직업이 공유하는 것이다
- **아이템의 외형만 `Items/`에 둔다.** 공격력과 가격 같은 수치는 `Data/`에 둔다
- **UI 위젯의 동작은 C++에 두고 WBP는 레이아웃과 애니메이션만 담는다**
- **마스터 머티리얼만 공용 폴더에 둔다.** 개별 에셋용 머티리얼 인스턴스는 그 에셋 폴더에 둔다

**팩에서 가져온 에셋도 이 규칙을 따른다.** 팩이 정한 하위 구조를 유지하지 않는다.
— 팩 구조대로 두면 에셋 하나를 지울 때 그 에셋만 쓰는 머티리얼 인스턴스와 텍스처를 참조를
따라가며 찾아야 한다. 폴더 하나로 완결되어 있으면 폴더를 지우면 된다.

**2026년 9월 27일에 우선순위를 뒤집었다.** #81은 반대로 팩 구조를 우선해서, `DARK_F_GREATSWORD`의
무기 전용 머티리얼 인스턴스와 텍스처를 `Weapons/Common/` 아래에 팩 구조 그대로 모아 두었다. 지금은
검마다 `Sword0R`~`Sword50R` 폴더에 메시, 인스턴스, 텍스처를 두고, `Weapons/Common/`에는 여러 검이
함께 쓰는 마스터 머티리얼, 부모 인스턴스(`MMI_Greatsword`), 머티리얼 함수, 머티리얼 파라미터 컬렉션
(`MPC_Snow`), 기본 텍스처만 남는다.

방어구는 처음부터 레벨별 폴더(`Armor0R`~`Armor50R`)에 인스턴스를 두었다. 마스터 머티리얼
`M_FA_MM`과 스켈레탈 메시 `SK_Fantasy`만 `Armors/Common/`에 있다.

### 4.3 에셋 접두사

| 접두사 | 에셋 | 접두사 | 에셋 |
| --- | --- | --- | --- |
| `BP_` | 블루프린트 | `WBP_` | 위젯 블루프린트 |
| `BPC_` | 블루프린트 컴포넌트 | `ABP_` | 애니메이션 블루프린트 |
| `SK_` / `SM_` | 스켈레탈 / 스태틱 메시 | `SKEL_` / `PHYS_` | 스켈레톤 / 피직스 에셋 |
| `AM_` / `BS_` / `A_` | 몽타주 / 블렌드 스페이스 / 애니메이션 시퀀스 | `ALI_` | 애니메이션 레이어 인터페이스 |
| `M_` / `MI_` / `MF_` | 머티리얼 / 인스턴스 / 함수 | `T_` | 텍스처 |
| `NS_` | 나이아가라 시스템 | `MS_` / `SC_` | MetaSound 소스 / 사운드 클래스 |
| `DT_` / `DA_` | 데이터 테이블 / 데이터 에셋 | `CT_` / `ST_` | 커브 테이블 / 스트링 테이블 |
| `IA_` / `IMC_` | 입력 액션 / 매핑 컨텍스트 | `L_` | 레벨 |
| `F_` / `FF_` | 폰트 / 폰트 페이스 | | |

**폰트는 두 에셋이 짝을 이룬다.** `.ttf`를 임포트하면 글꼴 데이터를 담은 `FontFace`와 그것을 쓰는
`Font`가 생긴다. 접두사가 둘을 가르므로 이름의 나머지는 같게 둔다
(`FF_NanumGothicBold`와 `F_NanumGothicBold`).

접두사는 자작 에셋에만 적용한다. 외부 팩 안의 이름은 고치지 않는다.

### 4.4 텍스처 접미사

텍스처는 접두사 뒤에 용도 접미사를 붙인다. `T_Goblin_Body_D`처럼 짓는다.

| 접미사 | 용도 |
| --- | --- |
| `_D` | 베이스 컬러 |
| `_N` | 노멀 |
| `_ORM` | Occlusion, Roughness, Metallic 채널 패킹 |
| `_E` | 이미시브 |
| `_M` | 마스크 |

### 4.5 에셋 이동과 삭제

- **에셋과 폴더의 이동과 이름 변경은 반드시 에디터 안에서 한다.** 탐색기나 git으로 옮기면
  참조가 깨진다
- 이동한 뒤에는 Fix Up Redirectors를 실행한다. 대상을 자작 에셋 폴더로 제한한다
- 삭제는 Reference Viewer로 참조를 확인한 뒤 한다. 강제 삭제를 쓰지 않는다

**언리얼 MCP의 `AssetTools.move`에는 제약이 다섯 가지 있다.** #52에서 실측했다(2026-09-21).

| 무엇이 | 어떻게 되는가 |
| --- | --- |
| 목적지 폴더가 이미 있으면 | `false`를 돌려주고 아무것도 하지 않는다 |
| 수백 개가 넘는 폴더를 한 번에 옮기면 | `true`를 돌려주고도 일부만 옮긴다. 메모리가 모자라면 에디터가 멈춘다 |
| 리다이렉터를 옮기면 | 그것이 가리키던 **본체까지 따라간다** |
| `DefaultEngine.ini`가 가리키는 에셋은 | 그 줄을 새 경로로 먼저 고치고 에디터를 다시 띄워야 옮겨진다 |
| `World`(레벨)는 | 언제나 거부된다. 콘텐츠 브라우저에서 사람이 옮긴다 |

그래서 **에이전트가 옮길 때는 목적지 폴더를 미리 만들지 않고, 한 번에 수십 개까지만 옮기며, 매번
원본이 남았는지 확인한다.** 원본이 남았으면 크기와 `ObjectRedirector` 문자열로 리다이렉터인지
복제인지 먼저 가른다 — 리다이렉터는 정상이고 Fix Up Redirectors가 지운다.

**옮긴 에셋의 헤더에는 옛 경로가 메타데이터로 남는다.** `.uasset`을 직접 읽어 참조를 세면 그 잔재를
실제 참조로 오해한다. 이동 뒤의 참조 판정은 에디터의 `get_dependencies`로 한다.

**엔진 버전이 다른 에셋은 옮기면 크기가 크게 바뀐다.** 5.3에서 저장된 것이 5.8로 재저장되면 절반
아래로 줄기도 한다. 크기 차이는 손상의 근거가 되지 않는다. 판정은 `load_asset`으로 한다.

**언리얼 MCP가 만든 변경은 디스크에 바로 써지지 않는다.** 2026-09-22에 `T_Boots_1`로 실측했다.

| 부른 것 | 반환 | 디스크 |
| --- | --- | --- |
| `duplicate` | `true` | **파일이 생기지 않는다.** `is_dirty`가 `true`로 남는다 |
| 이어서 `save_assets` | `true` | 파일이 생긴다 (555,277바이트) |
| `delete` | `true` | 즉시 지워진다 |

그래서 **에셋을 만들거나 고친 뒤에는 `save_assets`를 부르고 `git status`로 확인한다.** 부르지
않으면 「복사했다」는 보고와 디스크 상태가 어긋난다. 그 상태에서 에디터가 강제 종료되면 변경이
사라진다. `P1/Scripts/Invoke-UeBuild.ps1`이 `-CloseEditor` 없이 1로 끝나는 이유가 이것이다.

**삭제만 즉시 확정된다.** 만드는 쪽은 날아가기 쉽고 지우는 쪽은 이미 벌어져 있다. 되돌리려면
에디터를 닫고 `git checkout`을 거친 뒤 다시 띄운다. 에디터가 떠 있는 채로 되돌리면 메모리의
옛 상태가 디스크를 다시 덮는다.

**참조를 바꿀 때 그 참조가 들어 있는 자리는 에셋 종류마다 다르다.** #81에서 실측했다
(2026-09-23).

| 참조하는 쪽 | 참조가 들어 있는 자리 |
| --- | --- |
| 애니메이션 블루프린트 | 애님 그래프 노드의 속성 (`node.sequence`) |
| 블루프린트 변수 | CDO (`<에셋>.Default__<에셋>_C`) |
| 블루프린트에서 추가한 컴포넌트 | SCS 템플릿 (`<에셋>.<에셋>_C:<이름>_GEN_VARIABLE`) |
| C++로 선언한 컴포넌트 | CDO의 서브오브젝트 (`<에셋>.Default__<에셋>_C:<이름>`) |
| 위젯 | 위젯 트리의 위젯 속성과 CDO 양쪽 |
| 머티리얼과 머티리얼 함수 | 그래프 노드의 `texture`·`materialFunction`·`collection` |
| 머티리얼 인스턴스 | `parent`와 `textureParameterValues` 배열 |
| 메시 | `staticMaterials` 배열과 `skeleton` |

**자리를 추측하지 않는다.** `ObjectTools.list_properties`로 스키마를 받고 그중 에셋 참조를
담는 속성만 골라 읽는다. 위젯은 클래스마다 속성 이름이 달라서 추측이 특히 위험하다.

**이름이 같아도 참조 대상은 제각각이다.** `WBP_Help`의 `Image_KeyboardBg` 계열 여덟 개 중
여섯은 팩 텍스처를, 하나는 자작 텍스처를 가리켰고 하나는 비어 있었다. 값을 먼저 읽고 바꿀
것만 고른다.

**중첩 속성과 배열 요소는 부분 설정이 듣는다.** `brush.resourceObject` 하나만 지정해도
`imageSize`와 `tintColor`가 유지되고, `staticMaterials[0].materialInterface`만 지정해도
`materialSlotName`이 남는다.

**`MaterialInstanceTools.set_parent`를 쓰지 않는다.**
— 메모리의 `parent`는 바꾸지만 `dirty`를 세우지 않아서 이어지는 `save_assets`가 건너뛴다.
반환값도 `null`이라 성공과 실패를 가리지 못한다. 같은 일을 `ObjectTools.set_properties`로
하면 `dirty`가 서고 `true`가 돌아온다.

**머티리얼 인스턴스는 파라미터를 고친 뒤에도 옛 텍스처 참조를 계속 들고 있다.** 편집 가능한
속성에는 옛 경로가 없는데 `get_dependencies`만 옛 경로를 돌려주는 상태다.
— 2026년 9월 27일에 원인을 찾았다(#97). 옛 경로는 `TextureStreamingData`에 소프트 경로로 남아 있다.
에디터가 텍스처 스트리밍용으로 저장해 두는 배열이고, 편집할 수 없는 속성이라 `ObjectTools`로
읽거나 비울 수 없다. 파라미터 변경, 부모 `recompile`, 에디터 재시작, 스태틱 스위치 변경으로는
다시 계산되지 않는다. 이 배열은 에셋마다 따로 저장되고 부모에게서 물려받지 않는다.

**그래서 인스턴스를 새로 만들어 교체한다.** 새 인스턴스는 이 배열이 비어 있다. #97에서 인스턴스
18개를 이 절차로 교체했다.

1. 원본의 오버라이드 배열(`scalarParameterValues`, `vectorParameterValues`, `textureParameterValues` 등)과
   `basePropertyOverrides`, `subsurfaceProfile`을 읽는다
2. 같은 부모로 `<이름>_New`를 만들고 읽은 값을 `set_properties`로 쓴다
3. 스태틱 스위치는 `editorOnlyData`에 있어 읽히지 않는다. `get_static_switch_parameter`로 원본과
   부모를 비교해 다른 것만 설정한다. 부모가 머티리얼이면 비교할 수 없으므로 전부 원본 값으로 설정한다
4. 원본과 속성 전체, 스위치 전체를 대조한 뒤 저장한다
5. 참조자를 돌린다. 메시는 `staticMaterials`나 `materials`, 자식 인스턴스는 `parent`다. 자식은
   바꾸기 전후의 속성과 스위치를 대조한다
6. 원본을 지우고 새 인스턴스를 원래 이름으로 옮긴다

### 4.6 UI 텍스처 임포트 설정

아이콘 텍스처는 텍스처 그룹을 UI로, 밉맵 생성을 끔으로, 압축을 UserInterface2D로 지정한다.
크기는 하나로 통일한다.
— 기본 설정 그대로 두면 한 장이 수백 킬로바이트가 된다. 화면에는 128픽셀로 그려지는 그림이다.

---

## 5. Content와 Source의 경계

기준은 하나다. **다른 코드가 의존하는 로직은 C++, 값과 조립과 연출은 Content다.**
— C++은 블루프린트 클래스를 직접 참조하기 어렵다. 여러 시스템이 부르는 로직이 BP에 있으면
나중에 옮길 때 줄줄이 고쳐야 한다.

**반드시 C++로 두는 것**

- 패킷 직렬화, 송수신, 서버 응답 처리
- 이동, 피격 표현, 데미지 표시처럼 다수 개체가 자주 실행하는 코드
- 서버 검증이 필요한 시스템의 요청 생성과 결과 반영
- DataTable 행 구조체와 DataAsset 클래스
- 인터페이스, 그리고 여러 도메인이 공유하는 열거형

**BP로 둬도 되는 것**

- C++ 클래스를 상속해 값만 채운 설정용 BP
- 한 번만 쓰이는 연출과 트리거
- 프로토타입. 채택되면 C++로 옮기는 것을 전제로 한다

**에디터 그래프로 남는 것**

| 에셋 | C++로 두는 부분 | Content에 남는 부분 |
| --- | --- | --- |
| 애니메이션 블루프린트 | 변수 계산 | 스테이트 머신, 블렌드 그래프 |
| 위젯 블루프린트 | 동작 로직과 `BindWidget` | 레이아웃, 위젯 애니메이션 |
| 머티리얼, 나이아가라 | 커스텀 노드 | 그래프 전체 |

**C++ 코드에 에셋 경로 문자열을 쓰지 않는다.** C++은 `UPROPERTY`로 슬롯만 만들고, 설정용 BP나
DataAsset이 값을 채운다.
— 경로를 하드코딩하면 에셋을 옮길 때 코드가 깨지고, 깨진 것이 런타임에야 드러난다.

---

## 6. 게임 서버

```
Server/
├── Server.sln
├── ServerCore/             재사용 가능한 코어
│   ├── Network/            IOCP, 리스너, 세션, 버퍼
│   ├── Job/                잡 큐, 잡 타이머, 글로벌 큐
│   ├── Thread/
│   ├── DB/                 커넥션 풀, 바인딩, DB 큐, Redis
│   ├── Utils/              락 큐, 로거
│   └── Core/               타입, 매크로, TLS, 전역 객체, pch
├── GameServer/             게임 규칙
│   ├── GameServer.cpp      진입점
│   ├── Core/               pch, 전역 타입과 매크로, 전역 객체와 그것을 만들고 지우는 서버 컨텍스트, 서버 설정, 로비와 게임 기획 데이터의 기동 대조
│   ├── Network/            세션과 세션 단계별 허용표, 세션 관리자, 게임 패킷 핸들러, 아이템 요청 처리(퀵슬롯 등록 포함)와 아이템·퀵슬롯 패킷 변환, 저장 게이트, 진행 조율자, 게임 입장(게임 입장 창구), 인증 티켓 값 해석
│   ├── Lobby/              로비 모듈. 로비 패킷 핸들러, 로비 요청의 응답을 만드는 요청 처리(LobbyRequests), 로비 복귀 창구(LobbyReturn), 캐릭터 생성 규칙, 계정 행과 캐릭터 행의 DAO, 로비 기획 데이터(Data/)
│   ├── Game/               게임 도메인
│   │   ├── Room/           룸, 룸 매니저, 셀 행렬(CellMatrix), 룸 이동 판정과 입장할 룸 배정, 이동 위치 판정
│   │   ├── Combat/         피격과 처치 판정
│   │   ├── Entities/       엔티티 계층과 컴포넌트 베이스, 엔티티 생성, 진행 사본과 저장 사본
│   │   ├── Items/          아이템과 칸의 서버 타입, 범용 칸 배열(ItemContainer), 인벤토리 구역 대응 표
│   │   ├── Inventory/      인벤토리 컴포넌트와 재사용 대기
│   │   ├── Equipment/      장비 컴포넌트와 착용 조건
│   │   ├── QuickSlots/     퀵슬롯의 서버 타입과 퀵슬롯 컴포넌트
│   │   ├── AI/             몬스터 AI 컴포넌트
│   │   ├── Progress/       누적 경험치와 레벨 판정, 새 캐릭터의 초기 진행
│   │   └── Data/           기획 데이터 템플릿과 그 검증, 로더
│   │       └── Generated/  DataBuilder의 게임 모듈 산출물. JSON과 DesignData.h
│   ├── DB/                 게임의 데이터별 DAO(캐릭터 상태, 아이템, 퀵슬롯, 발급한 ID), 아이템 행 변환, 진행 저장소, DB 연결 가드와 DB 스레드 루프
│   ├── Migrations/         GameDB 스키마. `NNN_<이름>.sql`을 번호 순서로 GameDB와 GameDB_Test에 적용한다
│   ├── Protocol/           생성물
│   └── Utils/              난수, 평면 수학, 문자열 변환, 캐릭터 ID와 아이템 UID 발급기
├── GameServerTests/
├── DummyClient/
├── AuthServer/             Node 인증 서버
│   ├── src/
│   │   ├── main.ts         진입점. 설정 검사, AccountDB와 Redis 연결, 실제 구현 조립, 포트 열기
│   │   ├── app.ts          앱 조립. 도메인마다 라우터를 한 줄로 붙인다
│   │   ├── auth/           도메인: 회원가입과 로그인. router, handler, service, repository
│   │   ├── world/          도메인: 월드 목록과 월드 선택. 계정 상태 저장소가 repository다
│   │   ├── common/         도메인이 아닌 공용 코드. 설정, DB 연결, 인증 티켓, 로그인 세션, 월드 디렉터리, 봉투, 에러, 비밀번호
│   │   └── types/          타입 정의를 싣지 않는 패키지의 선언 파일
│   └── migrations/         AccountDB 스키마. `NNN_<이름>.sql`을 번호 순서로 적용한다
└── Libraries/              벤더링한 서드파티
```

**`ServerCore`와 `GameServer`의 분리를 유지한다.** `ServerCore`는 게임 규칙을 모른다. 게임 규칙이
`ServerCore`로 새어 들어가면 그 경계가 사라진다.

**`Game/`이 게임 도메인과 배선을 가른다.** `GameServer/` 바로 아래의 `Core/`, `Network/`, `Protocol/`,
`DB/`, `Migrations/`, `Utils/`는 서버를 돌리는 배선이고, `Game/` 아래 열은 게임 규칙이다. 이 층을 없애면
폴더 열여섯 개가 한 줄에 놓여서 어느 쪽이 규칙인지 이름만으로 갈리지 않는다. 최상위의 `Lobby/`는 배선도 게임 규칙도
아닌 세로 모듈이다(아래 `Lobby/` 단락).
— 클라이언트도 같은 층으로 가른다. 분류명과 경로 모양이 양쪽에서 같으므로
`P1/Source/P1/Game/Inventory/`를 알면 `Server/GameServer/Game/Inventory/`를 찾는 데 지장이 없다.

**`.vcxproj`에 파일이 하나씩 명시 등록된다.** 파일을 추가하거나 옮기면 `.vcxproj`와
`.vcxproj.filters`를 함께 고친다. `GameServerTests.vcxproj`가 `GameServer`의 `.cpp`를 직접
컴파일하므로 고칠 목록이 두 벌이다. 필터 GUID는 VS가 만드는 무작위 값을 쓴다.
새 폴더는 `IncludePath`에 등록하지 않는다. include 규칙은 `.claude/rules/game-server.md` 「서버 include」에 있다.

**빌드 도구를 MSBuild에서 바꾸지 않는다.** 근거는
`docs/adr/0001-unify-build-path.md`의 「검토한 대안」에 있다.

`Game/Combat/`은 피격과 처치를 판정한다. 룸과 세션을 모르고, 대상 찾기와 결과 전송은 `Game/Room/`이
맡는다. `Game/Room/`의 `RoomTransfer`도 같은 모양이다. 룸 이동과 리스폰 요청을 판정하고, 맵과 좌표로 입장할 룸을
고른다. 퇴장과 입장, 결과 전송은 `Room`이 맡는다.

`Lobby/`는 로비가 하는 일(로비 접속, 캐릭터 목록, 생성, 삭제, 선택)을 담는 세로 모듈이다. 로비 패킷의 핸들러,
캐릭터 생성 규칙(`CharacterCreation`), `GameDB`의 계정 행과 캐릭터 행을 다루는 DAO가 여기 있다. `Game/` 아래에 두지 않는
이유는 `Game/`이 게임 규칙의 층인데 로비는 핸들러와 DAO까지 함께 담기 때문이다. 로비는 배선과 게임 입장 창구
(`Network/GameEntry`)만 include하고 `Game/`을 include하지 않는다. 로비의 기획 데이터는 `Lobby/Data/`(`LobbyData`)에 있고,
로비는 게임의 기획 데이터(`Gamedata`)를 읽지 않는다. `Gamedata`는 pch에 들어 있어 컴파일러가 막지 않으므로 리뷰가 본다.
게임 쪽 코드는 `Lobby/`를 include하지 않는다. 근거는 `docs/adr/0021-split-lobby-as-top-level-module.md`에 있다.
2026년 10월 23일(#287)에 `Game/Characters/`와 `DB/`에서 옮겨 왔다.

`Game/AI/`는 몬스터의 행동(배회, 추적, 공격)을 정하는 `MonsterAIComponent`를 담는다. 몬스터는 이 컴포넌트를 멤버로
소유하고 받은 룸 틱을 넘긴다. 2026년 10월 9일(#211)에 `Game/Entities/Monster`에서 떼어 냈다.

`Game/Progress/`는 진행의 규칙을 담는다. 2026년 10월 20일(#252) 기준으로 누적 경험치와 레벨 표로 레벨을 정하고
보상과 사망 페널티를 판정하는 `Leveling`만 있다. 2026년 10월 23일(#288)에 새 캐릭터의 초기 진행을 정하는 `InitialProgress`가
더해졌다. 룸과 세션을 모르고, 판정 결과를 플레이어에 쓰는 일은
`Game/Entities/Player`가 맡는다. 클라이언트의 `Game/Progress/`는 내 플레이어의 진행을 들고 있는 서브시스템이고,
두 폴더는 같은 도메인(진행)을 다루므로 분류명을 맞춘다.

SQL 스크립트는 그 DB를 소유한 티어 안에 둔다. `GameDB`는 게임 서버가, `AccountDB`는 인증 서버가
소유한다.

**인증 서버의 `src/`는 도메인 폴더와 `common/`으로 나눈다.** 도메인 폴더 하나를 지우면 그 도메인 로직만 사라져야 한다.
근거와 버린 대안은 `docs/adr/0015-group-auth-server-by-domain-folder.md`에 있다.

- 도메인 폴더 안에는 `<도메인>.<계층>.ts` 파일을 평평하게 둔다. 계층은 `router`(경로), `handler`(본문 검증과 응답 봉투),
  `service`(판정), `repository`(저장소 인터페이스와 실제 구현)다
- 메모리 구현은 `<도메인>.repository.memory.ts`처럼 `.memory.ts` 접미사로 나눈다. 파일 이름에 DB 기술 이름을 넣지 않는다
- 테스트는 그 폴더 아래의 `tests/`에 `<대상>.test.ts`로 둔다(`auth/tests/auth.service.test.ts`, `common/tests/config.test.ts`).
  도메인 경로의 HTTP 테스트(`<도메인>.router.test.ts`)도 그 도메인의 `tests/`에 둔다
- 도메인이 아닌 코드는 평평한 `common/`에 둔다. 인증 티켓처럼 여러 도메인이 쓸 수 있는 것도 여기에 둔다.
  로그인 세션과 월드 디렉터리는 `auth/`(로그인)와 `world/`(월드 목록과 월드 선택)가 함께 쓰므로 여기에 있다(2026년 10월 23일 덧붙임)
  `common/`은 도메인 폴더를 import하지 않는다
- 도메인을 지울 때 남는 것은 `app.ts`의 등록 줄, `main.ts`의 조립 줄, `common/errors.ts`의 그 도메인 에러 코드뿐이다

---

## 7. 생성물

**패킷 정의와 게임 데이터는 생성물이다.** 저장소에 커밋되어 있어 직접 고쳐도 되는 파일처럼
보이지만, 생성기를 다시 돌리면 덮어써진다.

- 스키마 원본은 `Protocol/Schema/` 한 곳에만 둔다
- 기획 수치 원본은 `DesignData/` 한 곳에만 둔다
- 생성된 코드는 커밋한다
- 생성기의 산출물 폴더에는 사람이 쓴 파일을 두지 않는다. 한 폴더에는 생성기 하나만 쓴다
- 스키마 변경과 생성 결과를 같은 커밋에 담는다
- 손으로 쓰는 파일 목록은 `docs/codegen.md` 「손으로 쓰는 것」에 있다

**생성물에 코딩 컨벤션을 적용하지 않는다.** 생성기가 스키마 파일 이름을 그대로 따르므로 사람이
규칙을 강제할 수 없다. 규칙 검사와 코드 서식 도구에서 생성물 경로를 제외한다.

패킷 이름의 `C_`와 `S_`는 방향 표시이고 **기능적으로 중요하다.** 생성기가 이 접두사로 핸들러
테이블을 나눈다. 접두사만 기능이고, 요청과 응답이 같은 이름을 쓰는 짝은 관례다.

절차와 목적지 목록은 `docs/codegen.md`에 있다.

### 7.1 기획 데이터

기획 데이터는 원본, 도구, 산출물이 세 곳에 나뉜다. 2026년 10월 27일(#313~#322)에 모든 표를 DataBuilder로 옮기고 옛 변환기를 걷어냈다.

| 무엇 | 경로 | 이름 |
| --- | --- | --- |
| DataBuilder 원본 | `DesignData/` | 표 이름 그대로의 워크북(`Currency.xlsx`). 설정 시트는 `Config.xlsx`에 둔다 |
| DataBuilder | `Tools/DataBuilder/` | 래퍼 `Invoke-DataBuilder.ps1`, 설정 `databuilder.toml`, 패키지 `databuilder/`, 테스트 `tests/` |
| DataBuilder 산출물 | 서버 모듈의 `Data/Generated/` | 게임 모듈은 `Server/GameServer/Game/Data/Generated/`, 로비 모듈은 `Server/GameServer/Lobby/Data/Generated/`다. 폴더 이름은 `Generated`로 고정이다 |
| DataBuilder 클라이언트 산출물 | `P1/DesignData/Generated/`, `P1/Source/P1/Game/Data/Generated/` | `<표>.json`과 `P1<표>Row.h`. `Content/` 밖의 JSON을 에디터 메뉴가 `Content/P1/Data/DesignData/`의 `DT_<표>`에 채운다 |
| 데이터 테이블 채우기 메뉴 | `P1/Scripts/Editor/` | `p1_design_data_tables.py`. 에디터의 Python 시작 스크립트가 불러온다 |

**산출물 폴더는 DataBuilder가 소유한다.** 실행할 때마다 비우고 다시 쓰므로 그 폴더에 손으로 쓴 파일을 두면 지워진다.
서버 모듈(로비, 게임)마다 산출물 폴더가 하나이고, 어느 모듈이 어느 표를 읽는지는 `Tools/DataBuilder/databuilder.toml`이 정한다. 클라이언트 산출물 폴더 둘도 DataBuilder가 소유한다.
로비 모듈의 산출물 폴더는 2026년 10월 27일(#317)에 로비 설정을 DataBuilder로 옮기면서 만들었다.

---

## 8. 버전 관리

### 8.1 Content

**`Content/P1/` 아래만 Git LFS로 추적한다. `Content/External/` 아래는 추적하지 않는다.**

두 폴더의 경계가 곧 추적 여부의 경계다. 그 경계를 지키는 규칙이 셋이다.

- 게임이 참조하는 에셋은 전부 `Content/P1/` 아래에 둔다
- `Content/External/`은 참고용 보관소다. 아무것도 그것을 참조하지 않는다
- 팩에서 쓸 것을 정하면 `Content/P1/` 아래로 복사하고, 게임은 복사본만 참조한다

**`Content/External/` 아래를 직접 참조하지 않는다.** 쓰기로 정했으면 복사한다.
— 팩을 지우거나 갱신할 때 무엇이 깨지는지 알 수 없게 된다. 근거는
`docs/adr/0006-version-content-partially.md`에 있다.

새 에셋을 만들면 `P1/.gitignore`의 화이트리스트에 그 폴더가 들어 있는지 확인한다.
— 화이트리스트 방식이라 새 폴더는 기본으로 제외된다. 팩을 새로 설치해도 자동으로 빠지는 것이 이
방식을 쓰는 이유다.

### 8.2 반드시 커밋하는 자동 생성물

`__ExternalActors__/`, `__ExternalObjects__/`, `_BuiltData`, `_sharedassets/`는 에디터가 만들지만
커밋한다. 이 파일들이 없으면 레벨이 온전히 열리지 않는다.

### 8.3 추적하지 않는 것

`Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/`, 서버 빌드 산출물, 그리고
비밀을 담은 파일이다.

---

## 9. 배치 판단표

애매한 대상은 **이것을 지울 때 함께 사라져야 하는 것은 무엇인가**를 먼저 묻는다.

### 판단 순서

1. 로직인가? 다른 코드가 의존하면 `Source`. 아니면 다음 단계
2. 특정 대상 하나에만 속하는가? 그 대상의 폴더
3. 특정 맵 하나에만 속하는가? 그 맵 폴더
4. 한 분류 안에서 공유되는가? 그 분류의 `Common/`
5. 여러 분류에서 공유되는가? 공용 분류

### 사례

| 대상 | 위치 | 이유 |
| --- | --- | --- |
| 몬스터 전용 머티리얼 인스턴스 | Content 그 몬스터 폴더 | 몬스터와 함께 삭제됨 |
| 캐릭터 피부 마스터 머티리얼 | Content 공용 머티리얼 폴더 | 여러 캐릭터가 공유 |
| 모든 직업의 감정표현 모션 | Content `Characters/Player/Common` | 직업 공용 |
| 물약 3D 모델 | Content `Items/` | 아이템 외형 |
| 물약 가격과 회복량 | `DesignData/` 원본 → Content `Data/DataTables` | 기획 수치 |
| 물약 사용 요청 생성 | Source `Game/Inventory/` | 시스템 로직 |
| 물약 사용 판정 | `Server/GameServer/Game/Inventory/` | 판정은 서버 권한 |
| 아이템 행 구조체 | Source `Game/Data/` | DataTable 행은 `USTRUCT` |
| 데미지 공식 | `Server/GameServer/` | 판정은 서버 권한 |
| 데미지 숫자 표시 위젯 | Content `UI/HUD` | 서버가 보낸 결과의 표시 |
| 몬스터 이동 보간 | Source `Sync/` | 표현만 맞추는 코드 |
| 몬스터 행동 결정 | `Server/GameServer/` | AI 판단은 서버 권한 |
| 상호작용 입력 액션 | Content `Core/Input` | 입력 |
| 패킷 스키마 원본 | `Protocol/Schema/` | 두 티어의 유일한 계약 |

---

## 10. 권한 원칙

**결과는 서버가 정하고 클라이언트는 요청하고 보여준다.** 이 원칙이 배치를 가른다. 판정은
`Server/`에, 연출은 `P1/Content/`에 있게 되는 이유다.

- 보상, 드롭, 강화 결과, 거래 성립은 게임 서버만 결정한다. 클라이언트에서 난수를 굴리지 않는다
- 클라이언트가 보낸 값은 신뢰하지 않고 서버에서 다시 검증한다
- **클라이언트 코드에 판정 함수를 두지 않는다.** 예측을 구현하더라도 그 결과는 화면 표현이고,
  서버 응답이 오면 서버 값으로 덮어쓴다
- 쿨다운과 지속시간은 서버가 보낸 시각을 기준으로 표시한다

**언리얼 리플리케이션과 RPC를 쓰지 않는다.** 튜토리얼과 엔진 샘플은 대부분 언리얼 데디케이티드
서버를 전제로 쓰여 있어서 무심코 따라 쓰기 쉽다. 이 프로젝트에서 그 자리를 대신하는 것은 아래와
같다.

| 언리얼 기능 | 이 프로젝트의 대체 |
| --- | --- |
| `UPROPERTY(Replicated)`, `GetLifetimeReplicatedProps` | 서버 상태 패킷을 `Sync/`가 반영 |
| Server RPC, Multicast RPC | `Network/`의 요청 패킷과 브로드캐스트 패킷 |
| CharacterMovementComponent의 네트워크 예측 | 자체 보간과 보정 |
| Behavior Tree, StateTree, EQS | 게임 서버의 AI 코드 |
| 데디케이티드 서버 타겟, 소스 빌드 엔진 | 런처 설치본을 그대로 사용 |
