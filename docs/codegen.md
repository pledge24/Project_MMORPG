# 코드 생성

**프로토콜·데이터 파일을 고치기 전에 반드시 읽는다.**

생성 결과물을 클라이언트/서버 트리로 **복사**하는 파이프라인이 두 개 있다. 와이어 프로토콜과 DataBuilder다. 복사본이 git에 커밋되어 있어서 직접 수정해도 되는 파일처럼 보이지만 아니다. **생성기를 다시 돌리는 순간 덮어써진다.**

---

## 와이어 프로토콜

**원본**: `Protocol/Schema/*.proto`. 도메인마다 한 파일이다(Common, Lobby, Entry, Entity, Combat, Item, Chat).

`.proto`는 cp949다. UTF-8로 저장하지 않는다.

### 재생성

`pwsh Protocol/Generate.ps1`을 실행한다. 이 스크립트는 `Schema/`의 `.proto`를 모두 읽어 `Tools/protoc.exe`를 한 번,
`PacketHandlerGenerator.exe`를 방향별로 두 번 돌린다. 생성물은 임시 폴더에 만들고, 모두 성공한 뒤에만 아래 목적지로 복사한다.

**어느 단계든 실패하면 아무것도 배포하지 않고 종료 코드 1로 끝난다.** protoc의 문법 오류와 생성기의 번호 범위 오류가 여기 걸린다.
— 옛 `GenPackets.bat`은 실패를 출력만 하고 복사로 넘어갔고, 생성기도 예외를 잡아 0으로 끝났다. 빌드가 옛 생성물로 통과해서
스키마와 생성물이 어긋난 것을 알아챌 수 없었다.

서버 솔루션 빌드도 이 스크립트를 부른다. `GameServer`와 `DummyClient`의 `GenerateProtoPackets` 타깃이, `.proto`가 생성물보다
새로우면 컴파일 전에 돌린다. `pwsh`가 PATH에 있어야 한다. 병렬 빌드에서 두 타깃이 함께 불러도 스크립트는 이름 있는 뮤텍스로
한 번에 하나씩 돈다.

| 목적지 | 산출물 |
|---|---|
| `Server/GameServer/Protocol/` | `*.pb.{h,cc}` |
| `Server/GameServer/Network/` | `ServerPacketHandler.h` |
| `Server/DummyClient/Protocol/` | `*.pb.{h,cc}` |
| `Server/DummyClient/Main/` | `ClientPacketHandler.h` |
| `P1/Source/P1/Network/` | `*.pb.{h,cc}` · `.proto` 전부 · `ClientPacketHandler.h` |

생성기가 읽는 템플릿은 `Protocol/Templates/PacketHandler.h`다. 핸들러 헤더는 읽은 스키마 파일의 `.pb.h`를 모두 include한다.

**스키마 파일을 더하면 서버 프로젝트 세 곳에 생성물을 등록한다.** `GameServer.vcxproj`와 `DummyClient.vcxproj`에는
`.pb.h`, `.pb.cc`, `.proto`를, `GameServerTests.vcxproj`에는 `.pb.cc`를 넣고, 각 `.filters`에도 넣는다. 클라이언트는 UBT가
폴더를 읽으므로 등록하지 않는다.
— 등록하지 않으면 `ProtocolContract` 테스트가 「스키마 파일이 빌드에 없다」로 실패한다.

### 패킷 번호

패킷 번호는 스키마 파일마다 정한 범위 안에서 선언 순서로 매긴다. 범위는 파일 첫머리의 한 줄로 정한다.

```
// @packet_id_range 200 299
```

| 파일 | 범위 |
|---|---|
| `Common.proto` | 100~199 |
| `Lobby.proto` | 200~299 |
| `Entry.proto` | 300~399 |
| `Entity.proto` | 400~499 |
| `Combat.proto` | 500~599 |
| `Item.proto` | 600~699 |
| `Chat.proto` | 700~799 |

- 범위 선언: 모든 스키마 파일에 둔다. 생성기는 패킷(`C_*`와 `S_*` 메시지)이 있는데 범위가 없는 파일에서 실패하고, `ProtocolContract` 테스트는 범위가 없는 파일에서 실패한다
- 생성기가 실패하는 범위: 다른 파일과 겹치는 범위, 패킷 수보다 좁은 범위, 1~65534를 벗어난 범위
- 번호가 밀리는 곳: 패킷을 끼워 넣은 파일 안의 뒤쪽 번호뿐. 다른 파일의 번호는 그대로
- 범위를 바꿀 때: `ProtocolContractTests.cpp`의 `SCHEMA_FILES` 표도 함께 고침. 고치지 않으면 테스트가 표와 생성물의 범위가 다르다고 실패

**패킷 번호에 65535를 쓰지 않는다.**
— 핸들러 표의 칸 수가 `UINT16_MAX`라 65535는 표 밖이다.

### 자료형의 자리와 import

- 자료형은 그것을 쓰는 도메인 파일에 둔다. 두 도메인 이상의 패킷이 쓰거나 로비와 게임의 코드가 함께 쓰면 `Common.proto`에 둔다.
  스탯 자료형 셋(`StatType`, `Stat`, `StatInfo`)은 함께 둔다
- `Common.proto`는 아무것도 import하지 않는다. `Common.proto`가 아닌 도메인 파일을 import하는 것은 `Entry.proto` 하나다
  (`Entity.proto`와 `Item.proto`). 게임 입장 응답이 내 엔티티의 개인 정보로 소지품과 퀵슬롯을 싣기 때문이다
- DB에 숫자로 저장되는 enum(`ItemType`, `CharacterClass`, `GearPart`, `QuickSlotRefType`)은 값 번호를 바꾸지 않는다.
  기획 데이터가 값 이름으로 읽는 `ItemType`은 이름도 바꾸지 않는다

**import를 바꾸면 `ProtocolContractTests.cpp`의 `SCHEMA_FILES` 표에서 그 파일의 import 목록을 고친다.**
— 도메인 파일끼리 얽히면 한 도메인의 계약을 고칠 때 다른 도메인의 생성물이 함께 바뀐다.

### 손으로 쓰는 것

생성되는 건 패킷 핸들러 **헤더뿐**이다. 아래 `.cpp`는 손으로 작성한 파일이고, 새 패킷의 실제 동작은 여기에 구현한다.

- `Server/GameServer/Network/ServerPacketHandler.cpp`
- `P1/Source/P1/Network/ClientPacketHandler.cpp`
- `P1/Source/P1/Lobby/P1LobbyPacketHandlers.cpp`
- `Server/DummyClient/Main/ClientPacketHandler.cpp`

`P1LobbyPacketHandlers.cpp`에는 로비 서브시스템을 부르는 클라이언트 핸들러(`S_CONNECT_LOBBY`, `S_CHARACTER_LIST`,
`S_CREATE_CHARACTER`, `S_DELETE_CHARACTER`, `S_ENTER_GAME`)의 정의만 둔다. `Network/`는 `Lobby/`를 부르지 않으므로 이 정의를 `Lobby/`에 둔다. 선언은 생성된
`ClientPacketHandler.h`의 것을 그대로 쓴다.

### 이름 규칙

생성기가 접두사로 핸들러를 나눈다. **접두사만 기능이고 짝은 관례다.**

- `C_*` — 클라 → 서버
- `S_*` — 서버 → 클라
- 요청과 응답이 같은 이름을 쓰는 것(`C_MOVE` / `S_MOVE`)은 관례다. 서버가 먼저 보내는 통지(`S_SPAWN`)에는 짝이 없다

### 불변식 검사

`ProtocolContractTests`(`Server/GameServerTests/`)가 아래 여섯 가지를 검사한다. 패킷을 추가하면 이 테스트가 먼저 알려준다.

- 생성기가 읽은 스키마 파일과 각 파일의 범위가 `SCHEMA_FILES` 표와 같은지. 생성기가 핸들러 헤더에 적는 `PACKET_SCHEMA_FILES`와 대조한다
- 스키마 파일마다 빌드에 들어 있고, 파일의 패킷 목록과 선언 순서가 `SCHEMA_FILES` 표와 같은지
- 패킷 번호가 그 파일 범위의 첫 번호부터 선언 순서로 이어지고, 범위를 넘지 않는지
- 파일의 범위끼리, 패킷 번호끼리 겹치지 않는지
- 모든 스키마 파일의 모든 메시지가 값을 채운 채로 왕복하는지 (`DescriptorPool` 리플렉션으로 자동 확장)
- 스키마 파일마다 import가 `SCHEMA_FILES` 표와 같은지

**패킷을 더하거나 옮기면 `SCHEMA_FILES` 표에서 그 파일의 패킷 목록을 고친다. 스키마 파일을 더하거나 범위를 바꾸면 표의 줄을 고친다.**
— 표는 계약 선언이다. 표와 스키마가 어긋나면 스키마가 바뀐 것을 의식적으로 반영하라는 신호다.

왕복 검사는 메시지가 늘어도 고치지 않는다.

---

## 기획 데이터 (DataBuilder)

`Tools/DataBuilder/`의 파이썬 스크립트 하나가 `DesignData/`의 워크북을 읽어 검사하고, 서버 모듈이 읽을 JSON과 그 JSON을 담는
C++ 타입, 클라이언트가 데이터 테이블에 채울 JSON과 그 행 구조체(`USTRUCT`)를 함께 생성한다. 결정과 근거는 `docs/adr/0022-generate-design-data-schema-with-databuilder.md`에 있다.

2026년 10월 27일(#317) 기준으로 DataBuilder가 맡은 표는 재화, 아이템 세 표(`Item`, `ItemEquipStat`, `ItemConsumeEffect`), 몬스터와 드롭 표(`Monster`, `DropGroup`, `DropEntry`), 레벨 두 표(`Level`, `ClassLevelStat`), 맵 세 표(`Map`, `Area`, `Portal`)다.
설정은 설정 워크북의 초기 진행(`Progress`)과 로비(`Lobby`) 두 시트다.
표를 옮기는 순서는 `docs/work/2026-10-27-design-data-pipeline.md`에 있다. 클라이언트 산출물은 2026년 10월 27일(#319)부터 만든다. 같은 날(#320)부터 클라이언트 코드는 DataBuilder가 만든 데이터 테이블만 읽는다(아래 「클라이언트가 읽는 법」).

### 재생성

```
pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1
```

래퍼는 첫 실행에 `Tools/DataBuilder/.venv/`에 가상 환경을 만들고 `requirements.txt`의 고정 버전 패키지를 설치한다.
`requirements.txt`가 바뀌면 다시 설치한다. exe는 만들지 않는다.

**검사가 하나라도 실패하면 산출물을 하나도 바꾸지 않고 종료 코드 1로 끝난다.** DataBuilder는 오류를 모두 모아 한 줄에 하나씩
출력한다. 오류 한 줄에는 워크북, 시트, 행, 열이 들어 있다. 예: `Currency.xlsx 시트 '재화' 6행 A열(Id): 키(Id) 1이 시트 '재화' 5행과 겹친다`.
모두 통과하면 임시 폴더에 생성한 뒤 산출물 폴더에 배포한다.

**원본이나 설정을 고치면 DataBuilder를 돌리고 산출물을 같은 커밋에 담는다.** DataBuilder 테스트가 커밋된 산출물과 원본에서
다시 만든 산출물을 비교한다. 둘이 다르면 테스트가 실패한다.

### 설정 파일

`Tools/DataBuilder/databuilder.toml`이 어느 워크북을 읽고, 어느 서버 모듈이 그 표를 읽는지 정한다. 엑셀은 정하지 않는다.

| 항목 | 정하는 것 |
|---|---|
| `[modules.<모듈>]` | 서버 모듈의 산출물 폴더(`server_dir`)와 생성되는 헤더의 C++ 네임스페이스(`namespace`). 폴더 이름은 `Generated`여야 한다. `namespace`를 적지 않으면 `DesignData`다 |
| `[client]` | 클라이언트 JSON 폴더(`json_dir`)와 생성 헤더 폴더(`source_dir`). 두 폴더 이름은 `Generated`여야 하고, `source_dir`은 `Source/<모듈>/` 아래여야 한다. 없으면 클라이언트 산출물을 만들지 않는다 |
| `[tables.<표>]` | 가로형 표 하나. 워크북 파일 이름과 그 표를 읽는 모듈 목록. `nest`를 적으면 부모 표 행의 그 필드 아래에 중첩한다(아래 「산출물」) |
| `[settings.<설정>]` | 세로형 설정 시트 하나. 워크북, 시트 이름, 모듈 목록 |
| `[enums.<열거형>]` | `.proto`에 없는 기획 데이터 전용 열거형의 값 이름과 번호. 번호는 1부터다 |

표 이름과 설정 이름이 그대로 생성되는 C++ 이름(`CurrencyRow`, `CurrencyTable`, `ParseCurrency`)이 된다. 모듈의 산출물 폴더 이름을 고정한
이유는 배포할 때 그 폴더를 통째로 지우기 때문이다.
— 설정 파일의 경로를 잘못 적으면 사람이 쓴 폴더가 지워진다.

**모듈마다 `namespace`가 달라야 한다.** 두 모듈이 같은 네임스페이스를 쓰면 DataBuilder가 설정 파일 오류로 멈춘다.
게임 모듈은 `DesignData`, 로비 모듈은 `LobbyDesignData`다.
— 서버의 pch가 게임 모듈의 헤더를 include하고, 기동할 때의 대조(`Core/DataCrossCheck`)가 로비 모듈의 헤더를 함께 include한다.
네임스페이스가 같으면 두 헤더의 공통 함수와 열거형 이름 표가 한 번역 단위에 두 번 정의된다.

### 시트 규칙

가로형 표는 한 워크북이 표 하나다.

| 행 | 내용 |
|---|---|
| 1 | 열 이름. PascalCase 식별자이고, 생성되는 C++ 필드 이름이 된다 |
| 2 | 타입. 아래 「타입」 표 |
| 3 | 대상. `both` · `client` · `server` · `none` 중 하나 |
| 4 | 설명. 생성되는 필드의 주석이 된다. 산출물의 값과 데이터 버전에는 들어가지 않는다 |
| 5~ | 데이터. 셀이 모두 빈 행은 건너뛴다 |

- 한 워크북의 시트는 1~3행이 같아야 한다. DataBuilder는 시트의 데이터 행을 시트 순서대로 이어 붙인다. 시트 이름은 데이터가 아니다
- 열 이름은 1행 A열부터 처음 빈 칸 앞까지다. 그 오른쪽 열에 값이 있으면 오류다
- 키 열이 하나 이상 있어야 한다. 키 열이 둘 이상이면 그 조합이 유일해야 한다
- 서버로 가는 열이 하나라도 있으면 키 열도 서버로 가야 한다. 클라이언트도 같다
- `none` 열은 어디에도 싣지 않는다. 빈 셀을 허용한다. 값이 있으면 타입을 검사한다
- `none`이 아닌 열의 빈 셀은 오류다. 예외는 선택 참조(`ref?:`)와 목록이다

세로형 설정 시트는 시트 하나가 구조체 하나다. 1행은 `Key | Type | Target | Value | Description`이고, 2행부터 한 행이 필드 하나다.
`Key`가 필드 이름이다. 키와 부모 표 참조는 쓰지 않는다. 나머지 규칙은 가로형 표와 같다.

### 타입

| 표기 | 셀에 적는 값 | C++ 타입 |
|---|---|---|
| `int32` · `int64` | 정수. 범위를 넘으면 오류다 | `int32` · `int64` |
| `float` | 수 | `float` |
| `string` | 문자열. 정수 셀은 문자열로 읽는다 | `string` |
| `bool` | 엑셀의 TRUE/FALSE, 또는 문자열 `TRUE`/`FALSE`(대소문자 무관) | `bool` |
| `enum:<열거형>` | 열거형 값 이름 하나 | 열거형 |
| `enum[]:<열거형>` | `;`로 구분한 값 이름 목록 | `vector<열거형>` |
| `ref:<표>` | 그 표의 키 하나 | 그 표의 키 타입 |
| `ref?:<표>` | 그 표의 키 하나, 또는 빈 셀 | `optional<키 타입>` |
| `ref[]:<표>` | `;`로 구분한 그 표의 키 목록 | `vector<키 타입>` |
| `parent:<표>` | 이 행이 속한 부모 표의 키. 필수다 | 그 표의 키 타입 |
| `int32?` · `int64?` · `float?` | 수 하나, 또는 빈 셀. 빈 셀은 산출물 JSON에서 `null`이다 | `optional<수 타입>` |

- `key:`를 앞에 붙이면 키 열이다(`key:int32`, `key:parent:Item`). 키 열이 될 수 있는 것은 정수, 문자열, 열거형, 참조 하나다. 실수, 불, 목록, 선택 참조, 선택 수는 키가 될 수 없다
- 선택 수(`int32?` 등)는 다른 열의 값에 따라 채울지가 정해지는 열에 쓴다. 채울지 말지는 표의 의미 규칙이 검사한다(아래 「몬스터와 드롭 표」)
- 참조하는 표는 설정 파일에 있고 키 열이 하나여야 한다. 셀의 값이 그 표의 키에 없으면 오류다
- 부모 표 참조 열은 표에 하나까지다. 자기 자신을 부모 표로 삼을 수 없다
- 목록 셀에 빈 항목(`A;;B`)이 있으면 오류다. 빈 셀은 빈 목록이다
- **JSON 셀을 쓰지 않는다.** `{`나 `[`로 시작하는 셀은 어느 타입이든 오류다. 좌표와 범위는 열로 나눈다

### 열거형 값의 표기

**셀에는 `.proto` 값 이름에서 열거형 이름의 접두사를 뺀 이름을 적는다.** 접두사는 열거형 이름을 대문자와 밑줄로 바꾼 것이다.

| 열거형 | `.proto` 값 | 셀에 적는 값 |
|---|---|---|
| `ItemType` | `ITEM_TYPE_GEAR` | `GEAR` |
| `CharacterClass` | `CHARACTER_CLASS_WARRIOR` | `WARRIOR` |
| `CurrencyType`(설정 파일) | — | `GOLD` |

- 0번 값(`ITEM_TYPE_NONE`)은 「값 없음」이라 셀에 적을 수 없다
- 접두사를 붙인 전체 이름(`ITEM_TYPE_GEAR`)과 소문자(`warrior`)는 오류다
- DataBuilder는 `Protocol/Schema/`의 최상위 열거형을 모두 읽는다. 메시지 안의 열거형은 읽지 않는다
- 설정 파일의 열거형과 같은 이름이 `.proto`에 있으면 오류다
- 산출물 JSON에는 열거형을 번호가 아니라 이 이름으로 싣는다. 생성된 역직렬화가 이름을 C++ 값으로 바꾼다

### 산출물

모듈마다 산출물 폴더 하나에 아래 파일을 만든다. 게임 모듈의 폴더는 `Server/GameServer/Game/Data/Generated/`, 로비 모듈의 폴더는
`Server/GameServer/Lobby/Data/Generated/`다. 데이터 버전 파일은 두 폴더에 같은 값으로 들어간다.

| 파일 | 내용 |
|---|---|
| `<표>.json` | 행 배열. 한 행이 한 줄이고, 행은 키 순서, 필드는 시트의 열 순서다. 서버로 가는 열(`both`, `server`)만 싣는다 |
| `<설정>.json` | 객체 하나. 한 필드가 한 줄이고, 시트의 행 순서다 |
| `DataVersion.json` | 데이터 버전 하나(`{"Version":"..."}`) |
| `DesignData.h` | 그 모듈이 읽는 표의 행 구조체(`<표>Row`), 키 → 행 맵(`<표>Table`), 설정 구조체(`<설정>Config`), `.proto`에 없는 열거형, 역직렬화 함수(`Parse<표>`, `Parse<설정>`, `ParseDataVersion`) |

- 키 순서에서 열거형 키는 값 번호 순서다
- 같은 원본으로 두 번 돌리면 산출물이 바이트 단위로 같다. 줄바꿈은 LF다
- 행 하나의 값을 바꾸면 그 표의 JSON diff는 그 행의 한 줄이다. 데이터 버전 파일도 함께 바뀐다
- **산출물 폴더에 사람이 쓴 파일을 두지 않는다.** DataBuilder는 실행할 때마다 산출물 폴더를 비우고 다시 쓴다

**설정 파일에서 `nest`를 적은 표는 자기 파일을 만들지 않고 부모 표의 파일에 중첩된다.** 부모 표는 그 표의 부모 표 참조 열(`parent:`)이 가리키는 표다.
부모 행 한 줄 뒤에 그 행에 속한 자식 행이 키 순서로 한 줄씩 오고, `]}`로 닫는다. 자식 행에는 부모 표 참조 열을 싣지 않는다.

```
{"Id":1,"Method":"INDEPENDENT","Rate":1.0,"PickCount":null,"Entries":[
{"ItemId":2000,"Rate":0.1,"Weight":null,"MinCount":1,"MaxCount":2}
]},
```

- 생성된 헤더에는 자식 행 구조체(`<표>Row`)와 원소 하나를 읽는 함수(`Read<표>Row`)만 있다. 부모 행 구조체가 `vector<<표>Row> <nest>` 필드를 갖는다
- 중첩한 표에는 부모 표 참조 열이 있어야 한다. 중첩은 한 단계까지다. `nest` 이름은 부모 표의 열 이름과 겹칠 수 없다
- 중첩한 표는 부모 표와 모듈이 같아야 한다. 한쪽만 가는 모듈이 있으면 그 모듈의 헤더가 컴파일되지 않는다
- 자식 행 하나를 바꾸면 diff는 그 자식 행의 한 줄이다

**데이터 버전은 원본의 값으로 만든 해시다.** 표마다 열 이름, 타입, 대상과 키 순서로 정렬한 행 값을 넣고, 설정과 열거형도 넣는다.
4행 설명, 시트 이름, 시트 안의 행 순서, 대상이 `none`인 열은 산출물을 바꾸지 않으므로 넣지 않는다. 워크북을 열어 고치지 않고 저장해도 버전은 그대로다.
서버는 기동할 때 이 값을 로그에 남긴다(`기획 데이터 버전 342624c01048663c을 불러왔다`).

### 검사하는 곳

기획 데이터 자체의 규칙은 DataBuilder가, 코드가 정한 제약은 서버 기동이 검사한다(ADR-0022). 같은 규칙을 두 곳에 쓰지 않는다.

| 검사 | 하는 곳 |
|---|---|
| 시트 규칙, 타입, 빈 셀, 키 중복, 다른 표의 키 참조, 열거형 값 | DataBuilder (`databuilder/workbooks.py`) |
| 표나 설정 하나의 의미 규칙. 재화 표에 `Type`이 `GOLD`인 행이 정확히 하나, 아래 「아이템 표」, 「몬스터와 드롭 표」, 「레벨 표」, 「맵, 지역, 포털 표」, 「설정 워크북」의 규칙 | DataBuilder (`databuilder/rules.py`) |
| JSON의 형식. 필드의 유무와 타입, 열거형 이름, 중첩된 행 | 서버. 생성된 `Parse<표>` |
| 코드가 정한 제약. DB 열 크기, 로비와 게임의 대조 | 서버 기동 |

표 하나의 의미 규칙을 더할 때는 `databuilder/rules.py`의 `TABLE_RULES`에 그 표 이름으로 함수를 넣는다. 설정의 의미 규칙은 `SETTING_RULES`에 그 설정 이름으로 넣는다.

**산출물을 막지 않는 규칙은 `TABLE_WARNINGS`에 넣는다.** 경고는 `[DataBuilder] 경고: ...`로 출력하고, 오류가 없으면 산출물을 배포하고 종료 코드 0으로 끝난다.
작업 중이라 아직 틀리지 않은 상태(어느 몬스터도 쓰지 않는 드롭 그룹)에 쓴다.

### 서버가 읽는 법

`Gamedata::LoadAllGamedata()`가 `Game/Data/Generated/`에서 표의 JSON과 `DataVersion.json`을 읽는다. 표는 생성된
`DesignData::Parse<표>`로 읽고, 게임 코드는 생성된 행 구조체를 그대로 쓴다. 재화는 `GamedataTables::currencies`가
`DesignData::CurrencyTable`이다. 아이템은 `Gamedata::FindItem`, `FindItemEquipStat`, `FindItemConsumeEffect`가 아이템 번호로
세 표의 행을 찾는다. 몬스터는 `Gamedata::FindMonster`가, 드롭 그룹은 `Gamedata::FindDropGroup`이 찾는다. 레벨은 `Gamedata::GetLevels`와 `GetMaxLevel`이, 능력치는 `Gamedata::FindClassLevelStat`이 직업과 레벨로 행을 찾는다.
지역은 `Gamedata::GetAreas`와 `FindArea`가 지역 번호로 찾고, 마을은 `Gamedata::GetTownAreaId`가 돌려준다. 포털은 지역 행의 `Portals`에 있다.
지역과 포털의 공간은 배치 데이터에서 `Gamedata::FindAreaPlacement`, `FindPortalPlacement`, `FindRespawnPoint`, `FindMapPlacement`가 찾는다(아래 「배치 추출」).
초기 진행은 `Gamedata::GetInitialProgress`가 생성된 `DesignData::ProgressConfig`로 돌려준다.

`LobbyData::LoadAll()`이 `Lobby/Data/Generated/`에서 `Lobby.json`을 생성된 `LobbyDesignData::ParseLobby`로 읽는다. 로비 코드는
`Lobby/Data/LobbyData.h`만 include하고 게임의 `Game/Data/` 헤더를 include하지 않는다. `LobbyModuleTest`가 `Lobby/`의 소스에서 이 include를 찾는다.

테스트는 생성된 행을 채운 `GamedataTables`를 `Gamedata::Install`로, 생성된 `LobbyDesignData::LobbyConfig`를 `LobbyData::Install`로 주입한다. 아이템은 `Server/GameServerTests/TestItemTables.h`로,
레벨은 `Server/GameServerTests/TestLevelTables.h`로, 지역과 포털은 `Server/GameServerTests/TestAreaTables.h`로 넣는다. 레벨 헬퍼는 레벨 표의 행과 그 직업의 능력치 행을 함께 넣는다.
지역 헬퍼는 지역의 맵을 맵 표에도 넣고, 마을을 넣으면 마을의 지역 번호도 정한다.
장비에는 장비 능력치 행을, 소모품에는 소모 효과 행을 함께 넣고, `MaxStack`을 채운다.
— 생성된 행의 `MaxStack` 기본값은 0이다. 0이면 아이템이 인벤토리에 들어가지 않는다. 실제 표에는 DataBuilder가 0을 막는다.

**게임 모듈의 헤더는 `Templates.h`가, 로비 모듈의 헤더는 `LobbyData.h`가 include한다.** 프로젝트 파일에는 `GameServer.vcxproj`에 두 모듈의 `DesignData.h`와 JSON을 등록한다. 헤더만 생성하므로
`GameServerTests.vcxproj`에 넣을 `.cpp`는 없다. 표나 설정을 더하면 그 JSON을 `GameServer.vcxproj`와 `.filters`에 등록한다.

생성된 역직렬화의 공통 함수(`DesignData::Detail::Read`)는 `DesignDataReadTests`가 목록, 선택 값, 열거형 목록까지 인스턴스화해서 확인한다.
아직 그 타입을 쓰는 표가 없어도 빌드가 그 코드를 컴파일하게 하려는 것이다.

### 아이템 표

아이템은 공통 열과 종류별 열을 세 워크북에 나눠 둔다. 종류별 표의 키는 아이템 번호(`key:parent:Item`)다.

| 워크북 | 담는 것 |
|---|---|
| `Item.xlsx` | 모든 아이템의 공통 열. 이름, 종류, 착용 조건, 가격, 최대 개수, 설명. 시트는 종류마다 하나다 |
| `ItemEquipStat.xlsx` | 장비의 착용 부위(`enum:GearPart`)와 착용하면 오르는 스탯 |
| `ItemConsumeEffect.xlsx` | 소모품이 회복하는 HP와 MP의 비율, 재사용 대기(`CooldownMs`) |

DataBuilder가 검사하는 규칙은 다섯 가지다. 기타(`MISCELLANEOUS`) 아이템은 종류별 표에 행이 없다.

- 아이템 번호의 범위: 장비 1000~1999, 소모품 2000~2999, 기타 3000~3999
- `MaxStack`: 1 이상
- `CooldownMs`: 0 이상. 대기가 없으면 0이다. 옛 표의 -1은 쓰지 않는다
- 장비 능력치 표: 장비(`GEAR`)만 참조한다. 모든 장비는 이 표에 행이 있다
- 소모 효과 표: 소모품(`CONSUMABLE`)만 참조한다. 모든 소모품은 이 표에 행이 있다

**착용할 수 있는 직업(`AllowedClasses`)이 빈 셀이면 모든 직업이 입는다.** 옛 표의 `all`이 빈 셀이 됐다.

이름(`Name`)과 설명(`Description`)은 클라이언트만 쓰므로 대상이 `client`다.

### 몬스터와 드롭 표

용어는 `CONTEXT.md`의 보상, 기본 보상, 드롭, 드롭 그룹, 드롭 항목을 따른다.

| 워크북 | 담는 것 |
|---|---|
| `Monster.xlsx` | 몬스터의 능력치, AI 거리, 기본 보상(`MinExp`, `MaxExp`, `MinGold`, `MaxGold`), 드롭 그룹 목록(`DropGroups`, `ref[]:DropGroup`) |
| `DropGroup.xlsx` | 판정 방식(`Method`, `enum:DropMethod`), 그룹 확률(`Rate`), 뽑는 개수(`PickCount`, `int32?`), 기획자용 메모(`Memo`, 대상 `none`) |
| `DropEntry.xlsx` | 소속 그룹(`key:parent:DropGroup`), 아이템(`key:ref:Item`), 확률(`Rate`, `float?`), 가중치(`Weight`, `int32?`), 최소와 최대 수량 |

**드롭 항목은 `DropGroup.json`의 각 그룹 아래 `Entries`에 중첩된다.** 설정 파일에서 `DropEntry`에 `nest = "Entries"`를 적었다. `DropEntry.json`은 없다.
한 그룹 안에서 같은 아이템은 한 번만 나온다(키가 그룹과 아이템의 조합이다).

판정 방식은 둘이다. `INDEPENDENT`는 항목마다 `Rate`로 따로 판정하고, `WEIGHTED`는 `Weight`에 비례해 `PickCount`개를 뽑는다.
드롭의 판정과 지급은 아직 없다. 서버는 드롭 그룹 표를 읽어 설치만 한다.

DataBuilder가 검사하는 규칙은 아래와 같다.

- 기본 보상: 최솟값은 0 이상이고 최댓값보다 크지 않다
- 그룹 확률(`Rate`): 0 초과 1 이하
- 그룹에 드롭 항목이 하나 이상 있다
- `WEIGHTED` 그룹은 `PickCount`가 1 이상이고 항목 수 이하다. `INDEPENDENT` 그룹은 `PickCount`를 비워 둔다
- `INDEPENDENT` 그룹의 항목은 `Rate`만, `WEIGHTED` 그룹의 항목은 `Weight`만 채운다. 항목의 `Rate`는 0 초과 1 이하, `Weight`는 1 이상이다
- 항목의 수량: `MinCount`는 1 이상이고 `MaxCount`보다 크지 않다
- 어느 몬스터도 쓰지 않는 그룹은 경고다

몬스터의 이름, 레벨, 공격 방식(`AttackType`, `enum:MonsterAttackType`)은 클라이언트만 쓰므로 대상이 `client`다.
옛 표의 `is_targeting`은 서버도 클라이언트도 읽지 않아 옮기지 않았다.

### 레벨 표

레벨마다 필요한 경험치는 직업과 무관하고, 레벨마다의 능력치는 직업마다 다르다. 그래서 두 워크북에 나눠 둔다.
2026년 10월 27일(#318)에 옛 전사 레벨 표(`Original_Warrior_Level_Data.xlsx`)에서 옮겼다.

| 워크북 | 키 | 담는 것 |
|---|---|---|
| `Level.xlsx` | `Level` | 다음 레벨까지 필요한 경험치(`ExpRequirement`) |
| `ClassLevelStat.xlsx` | `Class`(`enum:CharacterClass`), `Level`(`ref:Level`) | 최대 HP, 최대 MP, 물리 공격력, 마법 공격력. 시트는 직업마다 하나이고, 각 행에 직업을 그대로 적는다 |

DataBuilder가 검사하는 규칙은 세 가지다.

- 레벨 표: 행이 있고, 레벨이 시트의 행 순서대로 1부터 빠짐없이 이어진다. 마지막 행이 최대 레벨이다
- 레벨 표: 최대 레벨의 `ExpRequirement`는 0이고, 나머지 레벨은 1 이상이다. 0이면 경험치 없이 다음 레벨로 넘어간다
- 직업별 능력치 표: 표에 나오는 직업은 레벨 표의 모든 레벨에 행이 있다. 표에 없는 직업은 행이 하나도 없다

같은 직업과 레벨이 두 번 나오는 것은 복합 키 규칙이, 레벨 표에 없는 레벨은 참조 규칙이 막는다.

**레벨이 오를 때 더하는 능력치는 두 레벨 행의 차이다.** 옛 표의 증가분 열 넷은 인접한 두 행의 차이와 같아서 없앴다.
— 지금 스탯에는 장비의 증감량이 들어 있으므로, 서버는 새 레벨의 능력치로 덮지 않고 차이만 더한다.

마법사처럼 새 직업을 더할 때는 `ClassLevelStat.xlsx`에 그 직업의 시트를 더한다. 설정 워크북의 `Lobby` 시트에 생성할 수 있는 직업으로 넣으면
서버가 기동할 때 `Core/DataCrossCheck`가 이 표에 그 직업이 있는지 대조한다.

### 맵, 지역, 포털 표

용어는 `CONTEXT.md`의 맵과 지역을 따른다. 옛 맵 표의 행은 맵이 아니라 맵 1111 안의 지역이었다. 2026년 10월 27일(#316)에
옛 맵 워크북(`Original_Map.xlsx`)을 세 워크북으로 나눠 옮겼다.

| 워크북 | 키 | 담는 것 |
|---|---|---|
| `Map.xlsx` | `Id` | 맵 이름. 시작 맵과 플레이어의 마지막 위치가 이 번호를 가리킨다 |
| `Area.xlsx` | `Id` | 소속 맵(`MapId`, `ref:Map`), 이름, 유형(`Type`, `enum:AreaType`), 리스폰 지점 유무, 나오는 몬스터(`Monsters`, `ref[]:Monster`), 최대 몬스터 수, 몬스터 리스폰 시간(`MonsterRespawnSec`) |
| `Portal.xlsx` | `Id` | 소속 지역(`AreaId`, `parent:Area`), 유형(`Type`, `enum:PortalType`), 도착 지역(`DestAreaId`, `ref:Area`) |

**지역 표와 포털 표에는 공간 열이 없다.** 지역의 경계, 리스폰 지점, 포털의 출발 위치와 반경, 도착 위치와 방향은 레벨의 마커에서
추출한다(아래 「배치 추출」). 2026년 10월 29일(#336)에 표의 좌표 열을 걷어 내고 값을 레벨로 옮겼다.

**포털은 `Area.json`의 각 지역 아래 `Portals`에 중첩된다.** 설정 파일에서 `Portal`에 `nest = "Portals"`를 적었다. `Portal.json`은 없다.
포털 번호는 클라이언트가 포털 이동을 요청할 때 보내는 값이라 옛 번호를 그대로 옮겼다.

DataBuilder가 검사하는 규칙은 아래와 같다.

- 지역의 `MaxMonsterCount`와 `MonsterRespawnSec`는 0 이상이다
- `HasRespawnPoint`가 참인 지역(마을)은 정확히 하나다

없는 맵, 몬스터, 지역을 가리키는 것은 참조 규칙이 막는다. 서버는 마을이 없을 때만 부팅을 멈춘다(사망한 플레이어를 보낼 곳이 없어서).

**지금은 지역 하나를 룸 하나가 맡고, 지역 번호를 룸 번호로 쓴다.** 그 규칙은 `RoomTransfer`의 `SelectRoomForArea` 한 곳에 있다.

- 포털의 도착 지역은 클라이언트에도 싣는다. 배치 추출이 도착 지점이 그 지역 안에 있는지 검사한다
- 유형 이름: 옛 지역 유형 `safe zone`과 `hunting field`는 `SAFE_ZONE`과 `HUNTING_FIELD`가 됐다. 옛 포털 유형 `room`은 `AREA`가 됐다. 기획 데이터는 룸을 모르기 때문이다
- 서버가 읽지 않는 열: 지역 유형과 이름은 대상이 `client`다. 포털 유형과 몬스터 리스폰 시간은 서버로 가지만 아직 읽는 코드가 없다. 사망한 몬스터의 리젠은 백로그에 있다
- 맵 1111의 이름: 옛 표에 없어서 「시작의 섬」으로 정했다. 클라이언트 산출물의 `Map.json`에 실린다


### 클라이언트 산출물

**키 말고 클라이언트로 가는 열(`both`, `client`)이 하나라도 있는 표만 클라이언트 산출물을 만든다.** 키만 클라이언트로 가는 표(`Level`)와
서버 전용 표(`Currency`, `DropGroup`, `ClassLevelStat`)는 만들지 않는다. 2026년 10월 27일(#319) 기준으로 `Item`, `ItemEquipStat`, `ItemConsumeEffect`,
`Monster`, `Map`, `Area`와 `Area` 아래 중첩되는 `Portal`이 만든다.

| 폴더 | 파일 | 내용 |
|---|---|---|
| `P1/DesignData/Generated/` | `<표>.json` | 데이터 테이블에 채울 행 배열. `Content/` 밖에 있어 패키지에 들어가지 않는다 |
| `P1/Source/P1/Game/Data/Generated/` | `P1<표>Row.h` | 행 구조체 `FP1<표>Row`. 중첩한 표는 `FTableRowBase`를 상속하지 않는 구조체이고, 부모 행이 `TArray`로 갖는다 |
| 같은 폴더 | `P1<열거형>.h` | 설정 파일의 열거형을 `UENUM`(`EP1<열거형>`)으로 만든 것. 0번 값은 `None`이다 |
| 같은 폴더 | `P1DesignDataEnumNames.h` | `.proto` 열거형의 값 이름을 프로토콜 값으로 읽는 `P1DesignData::ParseEnumName` |
| 같은 폴더 | `P1DesignDataVersion.h` | 데이터 버전(`P1DesignData::Version`). 클라이언트가 게임 인스턴스를 초기화할 때 로그에 남긴다 |

- JSON은 서버 산출물과 같이 한 행 한 줄, 키 순서, 열 순서다. 각 행의 첫 필드 `RowName`이 데이터 테이블의 행 이름이고, 값은 기획 데이터의 키다. 복합 키는 `_`로 잇는다(`WARRIOR_1`)
- 행 구조체의 속성은 열 이름 그대로이고 `VisibleAnywhere`다. 에디터에서 행 값을 고치지 않는다. 값은 원본을 고치고 DataBuilder와 에디터 메뉴를 다시 돌려 바꾼다
- `.proto` 열거형은 `UPROPERTY`가 될 수 없어서 값 이름(`FString`)으로 싣는다. 게임 코드는 생성된 `Get<열>()`로 프로토콜 값을 얻는다. 값 이름이 아니면 0번 값이다
- 클라이언트로 가는 열에는 선택 값(`int32?`, `ref?:` 등)을 쓰지 않는다. 행 구조체가 빈 값을 담지 못해서 DataBuilder가 막는다
- 설정 시트는 클라이언트 산출물을 만들지 않는다. 설정 시트의 필드 대상이 `both`나 `client`면 DataBuilder가 막는다
- 중첩한 표가 클라이언트 산출물을 만들면 부모 표도 만들어야 한다. 아니면 DataBuilder가 막는다
- 클라이언트로 가는 설정 파일의 열거형에는 값 이름 `NONE`과 255를 넘는 번호를 쓰지 않는다. `UENUM`의 0번 값과 겹치고, 값 타입이 `uint8`이다

**데이터 버전 로그는 빌드에 들어간 헤더의 값이다.** DataBuilder를 다시 돌리고 에디터 메뉴를 돌리지 않으면 로그와 데이터 테이블의 값이 어긋난다.

### 클라이언트가 읽는 법

게임 코드는 프로젝트 설정 「P1 Game Data」(`UP1GameDataSettings`)의 정적 함수로 행을 찾는다. 행 이름이 기획 데이터의 키라서 함수는 번호를 받는다.
테이블 경로는 `Config/DefaultGame.ini`의 `[/Script/P1.P1GameDataSettings]`에 있다.

| 설정 | 테이블 | 찾는 함수 |
|---|---|---|
| `ItemTable` | `DT_Item` | `FindItemRow` |
| `ItemEquipStatTable` | `DT_ItemEquipStat` | `FindItemEquipStatRow`. 장비가 아니면 `nullptr`이다 |
| `ItemConsumeEffectTable` | `DT_ItemConsumeEffect` | `FindItemConsumeEffectRow`. 소모품이 아니면 `nullptr`이다 |
| `MonsterTable` | `DT_Monster` | `FindMonsterRow` |
| `ItemAssetTable` | `DT_ItemAssets` | `LoadItemIcon`. 에셋 테이블이다 |
| `MonsterAssetTable` | `DT_MonsterAssets` | `LoadMonsterClass`. 에셋 테이블이다 |

- 기획 데이터 테이블은 모두 `/Game/P1/Data/DesignData/`에 있다. 에셋 테이블(`DT_ItemAssets`, `DT_MonsterAssets`)은 옛 폴더(`/Game/P1/Data/DataTables/`)에 그대로 있다
- 에셋 테이블은 에디터가 소유하고 에디터 메뉴가 채우지 않는다. 행 이름은 짝이 되는 수치 테이블의 행 이름(번호)과 같다. 에디터 모듈의 검증기가 두 테이블의 짝을 대조한다(아래 「수치 테이블과 에셋 테이블의 짝」)
- 재사용 대기는 소모 효과 표의 `CooldownMs`다. 클라이언트의 대기 계산(`FP1ItemCooldown`)은 초 단위라 1000으로 나눠 넘긴다
- 맵과 지역 테이블(`DT_Map`, `DT_Area`)을 읽는 코드는 없다. 서버가 알린 맵 번호로 레벨을 여는 표(`MapLevels`)의 키가 맵 표의 `Id`다
- 몬스터 스포너(`AP1EntitySpawner`)는 몬스터 행과 몬스터 클래스를 모두 이 설정에서 찾는다. 스포너에 지정하는 테이블은 없다

UE 테스트는 에디터 메뉴가 만든 테이블 에셋을 읽지 않는다. 기획 데이터가 필요한 테스트는 `Tests/P1DesignDataTestTables.h`로
저장소의 클라이언트 JSON을 임시 테이블에 채워 설정에 끼운다.

### 수치 테이블과 에셋 테이블의 짝

에디터 모듈의 `UP1DesignDataPairValidator`(`P1/Source/P1Editor/Validation/`)가 프로젝트 설정의 수치 테이블과 에셋 테이블을 대조한다.
아이템은 `ItemTable`과 `ItemAssetTable`, 몬스터는 `MonsterTable`과 `MonsterAssetTable`이 짝이다. 짝의 어느 쪽을 검증해도 짝 전체를 검사한다.
장비 능력치 테이블(`ItemEquipStatTable`)은 메시가 필수인 아이템을 정하므로, 이 테이블을 검증해도 아이템 짝을 검사한다.
검증은 저장할 때, 콘텐츠 브라우저의 우클릭 **Asset Actions > Validate Assets**, `DataValidation` 커맨드렛(`docs/testing.md` 「에셋 검증 커맨드렛」)에서 돈다.
결과는 메시지 로그의 「Asset Check」에 나온다.

| 경우 | 심각도 |
|---|---|
| 수치 테이블에 있는데 에셋 테이블에 없는 행 | 오류 |
| 아이콘(`Icon`)이나 몬스터 클래스(`MonsterClass`)가 빈 행 | 오류 |
| 장비(장비 능력치 테이블에 행이 있는 아이템)인데 `SkeletalMesh`와 `StaticMesh`가 모두 빈 행 | 오류 |
| 채워진 참조가 없는 에셋을 가리키는 행. 패키지가 없거나, 패키지는 있지만 패키지 이름과 다른 객체를 가리킨다 | 오류 |
| 짝이 되는 테이블을 설정에서 불러오지 못함, 에셋 테이블의 행 구조체가 다름 | 오류 |
| 에셋 테이블에만 있는 행(기획에서 지운 행) | 경고 |

- 장비는 부위마다 스켈레탈 메시나 스태틱 메시 중 하나를 쓴다. 어느 부위가 어느 메시를 쓰는지는 캐릭터 블루프린트의 메시 컴포넌트가 정하므로 검증기는 하나만 요구한다
- 오류가 있어도 저장은 된다. 엔진의 저장 시 검증은 결과를 알리기만 한다
- 원본에 행을 더하면 DataBuilder와 에디터 메뉴를 돌린 뒤 에셋 테이블에도 같은 번호의 행을 넣는다. 원본에서 행을 지우면 에셋 테이블의 그 행도 지운다

### 설정 워크북

값이 하나뿐인 설정은 `Config.xlsx`의 세로형 시트에 둔다. 2026년 10월 27일(#317)에 옛 초기 진행 표(`Original_Initial_Progress.xlsx`)와
로비 표(`Original_Lobby.xlsx`)에서 값을 그대로 옮겼다. 열은 모두 대상이 `server`다.

| 시트 | 모듈 | 필드 |
|---|---|---|
| `Progress` (초기 진행) | `game` | 시작 맵(`StartMapId`, `ref:Map`), 시작 좌표(`StartX`, `StartY`, `StartZ`), 시작 방향(`StartYaw`), 시작 골드(`StartGold`) |
| `Lobby` (로비) | `lobby` | 새 계정의 캐릭터 슬롯 수(`DefaultCharacterSlotCount`), 생성할 수 있는 직업(`CreatableClasses`, `enum[]:CharacterClass`) |

**초기 진행의 산출물은 게임 모듈의 폴더로, 로비 설정의 산출물은 로비 모듈의 폴더로 간다.** 로비와 게임은 서로의 기획 데이터를 읽지 않는다(ADR-0021).

DataBuilder가 검사하는 규칙은 아래와 같다.

- 시작 맵: 맵 표(`Map.xlsx`)에 있는 맵 번호다. 지역 번호를 적으면 참조 규칙이 막는다
- 시작 골드: 0 이상이다
- 슬롯 수: 1 이상이다
- 생성할 수 있는 직업: 비어 있지 않고, 같은 직업이 두 번 나오지 않는다

같은 키가 두 번 나오는 것은 설정 시트의 키 규칙이 막는다. 서버는 기동할 때 두 가지만 검사한다.

- 슬롯 수가 255 이하인지. DB의 `Account.CharacterSlotCount`가 `TINYINT`다(`LobbyData::Load`)
- 생성할 수 있는 직업마다 직업별 능력치 표에 행이 있는지(`Core/DataCrossCheck`)

---

## 게임 데이터

기획 데이터 전체에 걸리는 규칙과 서버와 클라이언트가 산출물을 읽는 법이다. 기획 데이터는 모두 DataBuilder가 만든다.
옛 변환기(`ExcelToJsonConverter`)와 그 변환기만 만들던 퀘스트 표는 2026년 10월 27일(#322)에 걷어냈다.
퀘스트를 만들 때는 퀘스트 표를 DataBuilder에 새로 더한다.

**원본에는 수치만 둔다. 에셋 경로 열을 만들지 않는다.** 아이콘, 메시, 몬스터 클래스는
`DT_ItemAssets`와 `DT_MonsterAssets`에 에디터로 넣는다. 두 테이블은 JSON에서 임포트하지 않으며,
행 이름이 `TemplateId`다. 근거는 `docs/adr/0009-keep-asset-references-out-of-design-data.md`에 있다.

**Claude는 워크북을 `openpyxl`로 만들고 고친다. 기존 워크북을 고칠 때도 사람에게 넘기지 않는다.** 2026년 10월 27일
사용자가 워크북 작업 전체를 사람의 개입 없이 하도록 허용했다.
— `openpyxl`은 저장할 때 이미지와 차트를 버린다. 워크북에는 값과 서식만 둔다.
— Claude가 고친 워크북을 사람이 엑셀에서 열어 저장했을 때 형식이 깨지면 이 결정을 다시 본다.

**기획 데이터는 소비자별로 나눈다.** 로비 설정은 로비 모듈(`Lobby/Data/`)만, 나머지는 게임(`Game/Data/`)만 읽는다. 로비와 게임이
함께 맞아야 하는 값(생성할 수 있는 직업이 직업별 능력치 표에 있는지)은 서버가 기동할 때 `Core/DataCrossCheck`가 대조하고, 어긋나면
서버가 뜨지 않는다. 근거는 `docs/adr/0021-split-lobby-as-top-level-module.md`에 있다.

### 서버 전용 표

옛 서버 전용 표 넷은 2026년 10월 27일(#317)에 모두 옮겼다. 초기 진행 표와 로비 표는 위 「설정 워크북」의 시트가 됐고,
인벤토리 구역 표와 퀵슬롯 표는 서버 상수가 됐다. 재화 표는 그보다 먼저(#313) DataBuilder로 옮겼다.

| 옛 원본 | 지금 있는 곳 |
|---|---|
| `Original_Initial_Progress.xlsx` (초기 진행) | `Config.xlsx`의 `Progress` 시트 |
| `Original_Lobby.xlsx` (로비) | `Config.xlsx`의 `Lobby` 시트. 직업은 소문자 JSON 배열(`["warrior"]`) 대신 열거형 값 이름 목록(`WARRIOR`)이다 |
| `Original_Inventory_Section.xlsx` (인벤토리 구역) | 서버 상수 `InventorySections::SLOT_COUNT`(32). 모든 구역의 칸 수다 |
| `Original_Quick_Slot.xlsx` (퀵슬롯) | 서버 상수 `QUICK_SLOT_COUNT`(9, `Game/QuickSlots/QuickSlotTypes.h`) |

**인벤토리 구역의 칸 수와 퀵슬롯 개수는 기획 데이터가 아니다.** 모든 캐릭터에게 같고, 바꾸면 클라이언트 UI도 함께 고쳐야 하는 값이다.

- 인벤토리 구역의 칸 수: 클라이언트 UI가 구역마다 32칸을 고정으로 쓴다. 바꾸면 인벤토리 위젯도 함께 고친다
- 퀵슬롯 개수: 클라이언트는 입장 패킷에 실린 퀵슬롯을 `WBP_QuickSlots`에 놓인 슬롯 위젯 아홉 개에 번호 순서로 그린다.
  9를 넘기면 `WBP_QuickSlots`에도 슬롯 위젯을 더한다. 256을 넘기면 빌드가 실패한다(DB의 `QuickSlot.SlotIndex`가 `TINYINT`)

### 클라이언트 데이터 테이블 채우기

DataBuilder의 클라이언트 JSON(`P1/DesignData/Generated/<표>.json`)마다 데이터 테이블 `/Game/P1/Data/DesignData/DT_<표>` 하나를 채운다.
**채우기 전에는 클라이언트가 그 표의 행을 찾지 못한다.** 아이템 툴팁이 뜨지 않고 상점 구매를 보내지 않으며, 몬스터가 스폰되지 않는다.
UE 테스트는 이 에셋을 읽지 않으므로 이 상태에서도 통과한다.

1. `pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1`로 산출물을 만든다. 행 구조체가 바뀌었으면 클라이언트를 빌드한다
2. 에디터 메인 메뉴의 **Tools > P1 > 기획 데이터 테이블 채우기**를 실행한다
3. 출력 로그에서 `[기획 데이터]` 줄을 확인한다. 표마다 `DT_<표> ← <표>.json (N행)`이 나오고, 마지막 줄이 `테이블 N개를 채웠다`다.
   임포터가 알린 문제(`LogCSVImportFactory`, `LogDataTable`)가 있으면 그 표의 값을 믿지 않는다
4. 바뀐 `.uasset`을 JSON과 같은 커밋에 담는다

메뉴는 `P1/Scripts/Editor/p1_design_data_tables.py`의 `fill_all()`이다. `Config/DefaultEngine.ini`의 Python 시작 스크립트가 에디터를 띄울 때 메뉴를 더한다.

- 테이블이 없으면 행 구조체 `FP1<표>Row`로 만들고, 있으면 비우고 다시 채운다. 원본에서 지운 행은 테이블에도 남지 않는다
- 행 이름은 JSON의 `RowName`이다. 스크립트가 테이블의 `ImportKeyField`를 `RowName`으로 두고 `bPreserveExistingValues`를 끈다
- 채운 테이블은 저장한다
- 엔진이 테이블의 임포트 원본으로 실행한 머신의 JSON 절대 경로를 기록한다. 이 값은 아무도 읽지 않는다. 메뉴를 돌린 머신이 바뀌면 `.uasset`에 이 경로의 diff가 생긴다
- 행 구조체가 없으면(클라이언트를 빌드하지 않았으면) 그 표를 건너뛰고 오류를 남긴다

**사람이 에디터에서 한다.** 메뉴의 결과는 에셋 변경이라, 실행과 확인을 사람에게 인계한다.
생성된 행 구조체가 저장소의 JSON을 문제 없이 받는지는 `P1.Data.DesignDataTables` 테스트가 임시 테이블로 확인한다.

### 로딩

서버는 부팅 시 `Gamedata::LoadAllGamedata()`(`Server/GameServer/Game/Data/Gamedata.h`)로 `Game/Data/Generated/`의 재화 표, 아이템 세 표, 몬스터 표,
드롭 그룹 표, 레벨 두 표, 맵 표, 지역 표, 초기 진행 설정, 데이터 버전을, `LobbyData::LoadAll()`(`Server/GameServer/Lobby/Data/LobbyData.h`)로
`Lobby/Data/Generated/`의 로비 설정을 읽는다. 둘 다 생성된 `Parse<표>`로 읽고, 모든 표가 통과하면 그때 설치한다.
게임 코드는 Json을 읽지 않고 `Gamedata::Find*`와 `Get*`으로 생성된 행만 읽는다.

**틀린 행이 하나라도 있으면 서버가 뜨지 않는다.** 「`Item.json` 2번째 행: 'BuyPrice'가 없다」처럼
파일과 행과 필드를 로그에 남기고 종료 코드 1로 끝난다. 필수 열이 없거나 타입이 틀린 행, 열거형 값 이름이 아닌 값, 겹친 키가 여기 걸린다.
위 「검사하는 곳」의 서버 기동 검사를 어긴 값도 같다.
— 게임 중에 Json을 읽던 때는 틀린 행이 룸 스레드의 예외가 되거나 조용히 기본값으로 동작했다.

세 값은 코드 상수가 아니라 기획표에서 온다.

| 값 | 원천 |
|---|---|
| 최대 레벨 | 레벨 표(`Level.xlsx`)의 마지막 레벨. 모든 직업이 같다. 레벨이 1부터 빠짐없이 이어지는지는 DataBuilder가 검사한다 |
| 마을 | 지역 표(`Area.xlsx`)에서 `HasRespawnPoint`가 참인 지역(`Gamedata::GetTownAreaId`). 둘 이상이면 DataBuilder가 실패하고, 없으면 부팅이 실패한다 |
| 골드의 재화 번호 | 재화 표(`Currency.xlsx`)에서 `Type`이 `GOLD`인 행(`Gamedata::GetGoldCurrencyId`). 둘 이상이면 DataBuilder가 실패하고, 없으면 부팅이 실패한다 |

---

## 배치 추출

서버가 알아야 하는 공간 정보는 레벨에 마커로 배치하고, 커맨드렛이 레벨에서 추출해 서버가 읽는 배치 데이터를 쓴다.
결정과 근거는 `docs/adr/0025-place-spatial-data-with-editor-only-markers.md`에 있다. 2026년 10월 29일(#336)에 지역, 리스폰 지점, 포털, 맵 경계를 옮겼다.

### 재생성

```
pwsh P1/Scripts/Invoke-PlacementExtract.ps1
```

스크립트는 `UnrealEditor-Cmd`로 커맨드렛 `P1PlacementExtract`를 돌린다. 에디터를 띄우지 않는다. 클라이언트를 빌드한 뒤에 돌리고,
레벨을 저장한 뒤에 돌린다. 커맨드렛은 저장된 레벨 파일을 읽는다.

**검사가 하나라도 실패하면 산출물을 하나도 바꾸지 않고 종료 코드 1로 끝난다.** 오류는 `[배치 추출]`로 시작하는 줄에 하나씩 나오고,
레벨 이름과 액터 이름이나 번호가 들어 있다. 전체 로그는 `P1/Saved/PlacementExtract/run.log`에 있다.

**레벨이나 지역 표, 포털 표를 고치면 추출을 돌리고 산출물을 같은 커밋에 담는다.** 서버의 `GamedataParserTest.RepositoryGamedataPasses`가
저장소의 배치 데이터와 기획 데이터를 함께 읽어 대조한다. 레벨을 고치고 추출을 돌리지 않았는지는 아직 감지하지 않는다(`docs/backlog.md` 41번).

### 마커

마커는 런타임 모듈의 `Game/World/`에 있고 `AP1PlacementMarker`를 상속한다. 베이스가 에디터 전용 액터로 표시하므로 쿠킹한 빌드에는 없다.

| 액터 | 레벨에 두는 수 | 담는 것 |
|---|---|---|
| `AP1MapMarker` | 맵 레벨마다 하나 | 맵의 경계(상자)와 AOI 셀 크기(`CellSize`, cm) |
| `AP1AreaVolume` | 지역 표의 그 맵 지역마다 하나 | 지역의 경계(상자)와 지역 번호(`AreaId`) |
| `AP1RespawnPointMarker` | 리스폰 지점이 있는 지역마다 하나 | 위치와 방향. 소속 지역은 추출이 위치로 정한다 |
| `AP1PortalArrivalMarker` | 포털마다 하나. 도착 지역의 맵 레벨에 둔다 | 위치, 방향, 포털 번호(`PortalId`) |
| `AP1Portal`(마커가 아니다) | 포털 표의 포털마다 하나 | 포털 번호, 서버가 받아 주는 반경(`UseRadius`, 에디터 전용 속성) |

- 상자 모양의 마커(`AP1MapMarker`, `AP1AreaVolume`)는 액터의 위치가 상자 바닥의 중심이다. 상자의 크기는 `Bounds` 컴포넌트의 Box Extent로 정한다. 회전하지 않는다
- 포털은 클라이언트에도 보여야 하므로 레벨에 남는 액터다. 위치와 반경만 추출하고, 도착 지점은 마커로 따로 둔다
- 경계 벽(`AP1FieldBoundaryWall`)은 에디터 전용 속성 `AreaVolume`이 가리키는 지역 볼륨에 맞춰 벽 넷을 세운다. 볼륨을 옮기거나 키우면 벽도 다시 맞춰진다

### 검사

커맨드렛은 프로젝트 설정의 맵 표(`MapLevels`)에 있는 레벨을 모두 열고, 클라이언트 기획 데이터(`P1/DesignData/Generated/`의 `Map.json`, `Area.json`)의
번호와 대조한다. 검사는 레벨을 모르는 `FP1PlacementCheck`(`P1/Source/P1Editor/Placement/`)가 한다.

- 레벨마다 맵 마커가 하나다. 맵 번호가 맵 표에 있다. 셀 크기와 상자의 반경이 0보다 크다
- 지역 볼륨의 지역 번호가 지역 표에 있고, 지역 표에서 그 맵의 지역이다. 그 맵의 지역마다 볼륨이 하나다
- 지역끼리 겹치지 않는다. 경계는 함께 쓸 수 있다
- 지역 볼륨과 모든 마커와 포털이 맵 경계 안에 있다. 상자 모양의 마커는 회전하지 않는다
- 리스폰 지점은 리스폰 지점이 있는 지역 안에 있고, 그런 지역마다 하나다
- 포털의 포털 번호가 포털 표에 있고, 포털이 포털 표의 출발 지역 안에 있다. 포털 표의 포털마다 레벨에 포털이 하나다
- 포털 도착 지점이 포털마다 하나이고, 포털 표의 도착 지역 안에 있다
- 경계 벽이 그 레벨의 지역 볼륨을 가리키고, 맞춘 위치와 크기가 볼륨과 같다

포함 판정에서 경계 위의 점은 안쪽이다. 서버의 `PlaneBounds`와 같다. 높이는 보지 않는다.

### 산출물

| 폴더 | 파일 | 내용 |
|---|---|---|
| `Server/GameServer/Game/Data/Extracted/` | `Placement_<맵 번호>.json` | 맵의 경계와 셀 크기, 지역(`Areas`), 리스폰 지점(`RespawnPoints`), 포털(`Portals`) |

- 한 행이 한 줄이고, 행은 번호 순서다(리스폰 지점은 지역 번호 순서). 줄바꿈은 LF다
- 좌표는 소수점 둘째 자리에서 반올림한다. 옮기지 않은 마커가 float 오차로 diff를 만들지 않는다
- 포털 행은 출발 위치(`Src*`), 반경, 도착 위치(`Dest*`)와 방향을 함께 싣는다. 파일은 포털이 놓인 맵의 것이다
- **산출물 폴더에 사람이 쓴 파일을 두지 않는다.** 커맨드렛은 맵 표에 없는 맵의 `Placement_*.json`을 지운다. DataBuilder 산출물 폴더(`Generated/`)에는 쓰지 않는다

### 서버가 읽는 법

`Gamedata::LoadAllGamedata()`가 `Game/Data/Extracted/`의 `Placement_*.json`을 모두 읽어 기획 데이터와 함께 검증한다. 서버는 형식과 번호만
대조하고, 위치와 겹침은 추출이 이미 검사했으므로 다시 보지 않는다. 아래 하나라도 어기면 서버가 뜨지 않는다.

- 배치 데이터의 맵, 지역, 포털이 기획 데이터의 그 맵에 있다. 리스폰 지점은 리스폰 지점이 있는 지역의 것이다
- 맵 표의 맵마다, 지역 표의 지역마다, 포털 표의 포털마다 배치 데이터가 있다. 마을에는 리스폰 지점이 있다

룸의 경계, 룸 배정, 이동 판정의 룸 경계는 지역의 배치 데이터(`AreaPlacement::bounds`)를 쓴다. 경계 계산과 포함 판정은 `Utils/PlaneBounds.h` 한 곳에 있다.
포털 판정과 도착 위치는 포털의 배치 데이터를, 마을 리스폰은 마을의 리스폰 지점을 쓴다. 맵의 경계와 셀 크기는 읽어서 설치만 하고, 룸을 맵의 사본으로 바꾸는
작업(#337)이 쓴다.

테스트는 `Server/GameServerTests/TestAreaTables.h`의 `TestArea`로 지역 행과 그 배치 데이터를 함께 넣는다.

---

## 생성기 자체

패킷 핸들러 생성기 exe는 `Tools/PacketHandlerGenerator/`의 파이썬 소스를 그 폴더의 `MakeExe.bat`으로 PyInstaller 번들링해서 만든다.
결과 exe는 같은 폴더에 놓이고, `Protocol/Generate.ps1`이 그 exe를 부른다. `.bat`도 cp949다.

DataBuilder(`Tools/DataBuilder/`)는 exe를 만들지 않는다. 래퍼가 가상 환경의 파이썬으로 소스를 바로 돌리므로, 소스를 고치면 다음 실행에 반영된다.

**패킷 핸들러 생성기의 파이썬 소스를 고치면 `MakeExe.bat`으로 exe를 다시 만들어 소스와 함께 커밋한다.**
— 스크립트는 소스가 아니라 exe를 부른다. exe를 다시 만들지 않으면 고친 소스가 돌지 않는다.

exe를 다시 만들려면 `py -3 -m pip install pyinstaller jinja2`로 두 패키지를 설치한다. `MakeExe.bat`은 `pyinstaller`를 PATH에서
찾으므로, `--user`로 설치했으면 `%APPDATA%\Python\Python313\Scripts`를 PATH에 더한다.