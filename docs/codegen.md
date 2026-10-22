# 코드 생성

**프로토콜·데이터 파일을 고치기 전에 반드시 읽는다.**

생성 결과물을 클라이언트/서버 트리로 **복사**하는 파이프라인이 두 개 있다. 복사본이 git에 커밋되어 있어서 직접 수정해도 되는 파일처럼 보이지만 아니다. **생성기를 다시 돌리는 순간 덮어써진다.**

---

## 와이어 프로토콜

**원본**: `Protocol/Schema/*.proto`. 도메인마다 한 파일이다(Common, Lobby, Entry, Entity, Combat, Item, Chat).
옛 파일 셋(`Enum`, `Struct`, `Protocol`)은 계약 티켓이 메시지를 도메인 파일로 다 옮기면 지운다. 옮길 정의는
`docs/work/2026-10-23-lobby-entry-flow.md`의 「목표 정의」에 있다.

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
| `Protocol.proto` (옛 파일) | 1000~1999. 지금의 번호를 그대로 두는 임시 범위 |

- 범위 선언: 패킷(`C_*`와 `S_*` 메시지)이 있는 파일에 필수. 패킷이 없는 `Enum.proto`와 `Struct.proto`에는 없음
- 생성기가 실패하는 범위: 다른 파일과 겹치는 범위, 패킷 수보다 좁은 범위, 1~65534를 벗어난 범위
- 번호가 밀리는 곳: 패킷을 끼워 넣은 파일 안의 뒤쪽 번호뿐. 다른 파일의 번호는 그대로
- 범위를 바꿀 때: `ProtocolContractTests.cpp`의 `SCHEMA_FILES` 표도 함께 고침. 고치지 않으면 테스트가 표와 생성물의 범위가 다르다고 실패

**패킷 번호에 65535를 쓰지 않는다.**
— 핸들러 표의 칸 수가 `UINT16_MAX`라 65535는 표 밖이다.

### 손으로 쓰는 것

생성되는 건 패킷 핸들러 **헤더뿐**이다. 아래 `.cpp`는 손으로 작성한 파일이고, 새 패킷의 실제 동작은 여기에 구현한다.

- `Server/GameServer/Network/ServerPacketHandler.cpp`
- `P1/Source/P1/Network/ClientPacketHandler.cpp`
- `P1/Source/P1/Online/P1LoginPacketHandlers.cpp`
- `Server/DummyClient/Main/ClientPacketHandler.cpp`

`P1LoginPacketHandlers.cpp`에는 로그인 관리자를 부르는 클라이언트 핸들러(`S_LOGIN`, `S_CREATE_CHARACTER`, `S_DELETE_CHARACTER`,
`S_ENTER_GAME`)의 정의만 둔다. `Network/`는 `Online/`을 부르지 않으므로 이 정의를 `Online/`에 둔다. 선언은 생성된
`ClientPacketHandler.h`의 것을 그대로 쓴다.

### 이름 규칙

생성기가 접두사로 핸들러를 나눈다. **접두사만 기능이고 짝은 관례다.**

- `C_*` — 클라 → 서버
- `S_*` — 서버 → 클라
- 요청과 응답이 같은 이름을 쓰는 것(`C_MOVE` / `S_MOVE`)은 관례다. 서버가 먼저 보내는 통지(`S_SPAWN`)에는 짝이 없다

### 불변식 검사

`ProtocolContractTests`(`Server/GameServerTests/`)가 아래 다섯 가지를 검사한다. 패킷을 추가하면 이 테스트가 먼저 알려준다.

- 생성기가 읽은 스키마 파일과 각 파일의 범위가 `SCHEMA_FILES` 표와 같은지. 생성기가 핸들러 헤더에 적는 `PACKET_SCHEMA_FILES`와 대조한다
- 스키마 파일마다 빌드에 들어 있고, 파일의 패킷 목록과 선언 순서가 `SCHEMA_FILES` 표와 같은지
- 패킷 번호가 그 파일 범위의 첫 번호부터 선언 순서로 이어지고, 범위를 넘지 않는지
- 파일의 범위끼리, 패킷 번호끼리 겹치지 않는지
- 모든 스키마 파일의 모든 메시지가 값을 채운 채로 왕복하는지 (`DescriptorPool` 리플렉션으로 자동 확장)

**패킷을 더하거나 옮기면 `SCHEMA_FILES` 표에서 그 파일의 패킷 목록을 고친다. 스키마 파일을 더하거나 범위를 바꾸면 표의 줄을 고친다.**
— 표는 계약 선언이다. 표와 스키마가 어긋나면 스키마가 바뀐 것을 의식적으로 반영하라는 신호다.

왕복 검사는 메시지가 늘어도 고치지 않는다.

---

## 게임 데이터

**원본**: `DesignData/Original_*.xlsx` (Item, Map, Monster, Quest, Warrior_Level_Data, Currency, Character_Creation, Inventory_Section, Quick_Slot)

**원본에는 수치만 둔다. 에셋 경로 열을 만들지 않는다.** 아이콘, 메시, 몬스터 클래스는
`DT_ItemAssets`와 `DT_MonsterAssets`에 에디터로 넣는다. 두 테이블은 JSON에서 임포트하지 않으며,
행 이름이 `TemplateId`다. 근거는 `docs/adr/0009-keep-asset-references-out-of-design-data.md`에 있다.

**새 워크북은 Claude가 `openpyxl`로 만든다. 기존 워크북은 사람이 엑셀에서 고친다.** Claude가 기존 워크북을 고쳐야 하면
고칠 내용을 사람에게 넘긴다. 변환기는 PyInstaller exe라서 `openpyxl` 없이 돈다. `openpyxl`은 Claude가 엑셀 파일을 만들거나
읽을 때만 설치한다.
— Claude가 만든 워크북을 사람이 엑셀에서 열어 저장했을 때 형식이 깨지면 이 결정을 다시 본다.

### 재생성

`DesignData/GenJsonFile.bat` 실행. 각 워크북을 JSON으로 변환한 뒤 **MOVE**한다.

**산출물은 아래 표의 13개가 전부다.**
— 와일드카드로 적으면 생성기가 만들지 않는 파일이 같은 폴더에 있어도 문서와 모순되지 않는다.

| 목적지 | 산출물 |
|---|---|
| `Server/GameServer/Game/Data/Json/` | `S_Warrior_Level_Data.json` · `S_Monster.json` · `S_Item.json` · `S_Map.json` · `S_Quest.json` · `S_Currency.json` · `S_Character_Creation.json` · `S_Inventory_Section.json` · `S_Quick_Slot.json` |
| `P1/Content/P1/Data/DataTables/` | `C_Monster.json` · `C_Item.json` · `C_Map.json` · `C_Quest.json` |

**이 목록에 없는 `C_` 또는 `S_` 접두사 JSON은 생성기의 산출물이 아니다.** 엑셀 원본을 고쳐도
갱신되지 않는다. 발견하면 쓰이는지 확인하고, 쓰이지 않으면 지운다.

장비 데이터는 `Original_Item.xlsx`의 첫 시트에 있다. 세 시트가 하나의 `C_Item.json`으로
합쳐지므로 장비만 담는 산출물은 없다.

레벨, 재화, 캐릭터 생성, 인벤토리 구역, 퀵슬롯 표는 서버만 읽는다. `GenJsonFile.bat`이 변환기가 만든 `C_*.json`을 바로 지운다.

### 서버 전용 표

재화, 캐릭터 생성, 인벤토리 구역 표는 2026년 10월 20일(#250)에 DB 기본값과 코드 상수에서 옮겨 왔다. 퀵슬롯 표는 2026년 10월 22일에 새로 만들었다. 열은 모두 `SERVER`다.

| 원본 | 열 | 규칙 |
|---|---|---|
| `Original_Currency.xlsx` (재화) | `template_id` · `currency_name` · `currency_type` | `template_id`가 DB의 `CurrencyId`다. `currency_type`은 서버 열거형 `CurrencyType`의 이름(`GOLD`)이고, `GOLD` 행은 정확히 하나다 |
| `Original_Character_Creation.xlsx` (캐릭터 생성) | `start_map_id` · `start_pos` · `start_yaw` · `start_gold` · `default_character_slot_count` | 행이 정확히 하나다. `start_map_id`는 룸 번호(`template_id`)가 아니라 맵 표의 `map_id`다. 시작 골드는 0 이상, 슬롯 수는 1~255다(DB 열이 `TINYINT`) |
| `Original_Inventory_Section.xlsx` (인벤토리 구역) | `item_type` · `slot_count` | `NONE`을 뺀 아이템 종류마다 행이 정확히 하나다. 칸 수는 1 이상이다 |
| `Original_Quick_Slot.xlsx` (퀵슬롯) | `slot_count` | 행이 정확히 하나다. 퀵슬롯 개수는 1~256이다(DB의 퀵슬롯 번호 열이 `TINYINT`) |

**인벤토리 구역의 칸 수는 32에서 바꾸지 않는다.** 클라이언트 UI가 구역마다 32칸을 고정으로 쓴다.

**퀵슬롯 개수는 서버만 읽고 클라이언트는 입장 패킷으로 받는다.** 클라이언트의 퀵슬롯 위젯은 WBP에 놓인 슬롯
위젯 아홉 개에 받은 퀵슬롯을 번호 순서로 그리므로, 개수를 9보다 늘리면 `WBP_QuickSlots`에도 슬롯 위젯을 더한다. 지금 값은 9다.

2026년 10월 21일 현재 서버는 세 표의 값을 모두 쓴다. 캐릭터 생성은 시작 맵, 좌표, 방향, 시작 골드를
표에서 읽는다. 로그인은 새 계정 행의 슬롯 수를 표의 기본값으로 정한다. 인벤토리는 구역마다 칸 수를 인벤토리 구역 표에서 읽는다.
2026년 10월 22일부터 퀵슬롯 컴포넌트가 퀵슬롯 개수를 퀵슬롯 표에서 읽는다.

### 클라이언트 데이터 테이블 재임포트

`C_*.json`을 다시 만들면 그 JSON을 소스로 쓰는 데이터 테이블을 다시 임포트한다. 아이템은 `DT_Item`이다.
**재임포트하기 전까지 클라이언트는 옛 값을 읽는다.** UE 테스트는 데이터 테이블을 읽지 않으므로 이 상태에서도
통과한다.

1. 에디터의 콘텐츠 브라우저에서 데이터 테이블(`Content/P1/Data/DataTables/DT_Item`)을 우클릭해 Reimport를 실행한다.
2. 테이블을 열어 바뀐 열의 값을 확인하고 저장한다.
3. `.uasset`이 `C_*.json`보다 나중에 저장됐는지 수정 시각으로 확인한다.

**사람이 에디터에서 한다.** 언리얼 MCP의 `DataTableTools`에는 재임포트 툴이 없다(2026년 10월 4일 확인).
`import_file`은 새 에셋을 만드는 툴이고, `set_rows`로 값을 고치면 JSON과 테이블이 따로 바뀌므로 쓰지 않는다.
재임포트 뒤 값의 확인은 MCP의 `DataTableTools.get_rows`로 할 수 있다.

필드 이름을 바꿀 때는 대소문자만 다른 이름(`ItemSubType` → `ItemSubtype`)이면 기존 값이 그대로 읽힌다.
언리얼이 속성 이름의 대소문자를 가리지 않기 때문이다.

### 로딩

서버는 부팅 시 `Gamedata::LoadAllGamedata()`(`Server/GameServer/Game/Data/Gamedata.h`)로 `S_Quest.json`을 뺀 여덟 파일을 읽는다.
`GamedataParser::Parse`가 모든 행을 검증해 템플릿 구조체(`Game/Data/Templates.h`)로 바꾸고, 표가 통과하면 그때 설치한다.
게임 코드는 Json을 읽지 않고 `Gamedata::Find*`로 템플릿만 읽는다.

**틀린 행이 하나라도 있으면 서버가 뜨지 않는다.** 「`S_Item.json` 2번째 행(templateId 1001): 'buyPrice'가 정수가 아니다」처럼
파일과 행과 필드를 로그에 남기고 종료 코드 1로 끝난다. 필수 열이 없거나 타입이 틀린 행, 표에 없는 아이템 종류와 장비 부위, 착용 조건의 직업(`all`이 아닌 값),
보상의 최솟값이 최댓값보다 큰 행, 0 이하의 포털 반경(`portalRadius`), 중복된 templateId, 맵 표에 없는 포털 목적지와 몬스터 표에 없는 스폰 몬스터가 여기 걸린다.
위 「서버 전용 표」의 규칙을 어긴 행도 같다.
— 게임 중에 Json을 읽던 때는 틀린 행이 룸 스레드의 예외가 되거나 조용히 기본값으로 동작했다.

세 값은 코드 상수가 아니라 기획표에서 온다.

| 값 | 원천 |
|---|---|
| 직업의 최대 레벨 | 그 직업 레벨 표의 마지막 레벨. 레벨은 1부터 빠짐없이 이어져야 한다 |
| 마을 룸 번호 | 맵 표에서 `hasRespawnPoint`가 참인 유일한 맵. 둘 이상이거나 없으면 부팅이 실패한다 |
| 골드의 재화 번호 | 재화 표에서 `currency_type`이 `GOLD`인 유일한 행(`Gamedata::GetGoldCurrencyId`). 둘 이상이거나 없으면 부팅이 실패한다 |

서버가 읽는 열을 늘릴 때는 `JsonProperty.h`의 열 이름, `Templates.h`의 필드, `GamedataParser.cpp`의 변환을 함께 고친다.
테스트는 표를 직접 고치지 않고 템플릿을 채운 `GamedataTables`를 `Gamedata::Install`로 주입한다.

`S_Quest.json`은 생성되지만 서버가 읽지 않는다. 읽는 코드가 없는 표 때문에 서버가 뜨지 않는 일을 없애려고
2026년 10월 9일(#207)에 불러오기를 뺐다. 퀘스트를 만들 때 템플릿과 함께 다시 읽는다.

---

## 생성기 자체

두 생성기 exe는 `Tools/`의 파이썬 소스를 각 툴의 `MakeExe.bat`으로 PyInstaller 번들링해서 만든다.

- `Tools/PacketHandlerGenerator/`
- `Tools/ExcelToJsonConverter/`

결과 exe는 각 툴 폴더 안에 놓인다. `Protocol/Generate.ps1`과 `DesignData/GenJsonFile.bat`이 그 exe를 부른다. `.bat`도 cp949다.

**생성기의 파이썬 소스를 고치면 `MakeExe.bat`으로 exe를 다시 만들어 소스와 함께 커밋한다.**
— 스크립트는 소스가 아니라 exe를 부른다. exe를 다시 만들지 않으면 고친 소스가 돌지 않는다.

exe를 다시 만들려면 `py -3 -m pip install pyinstaller jinja2`로 두 패키지를 설치한다. `MakeExe.bat`은 `pyinstaller`를 PATH에서
찾으므로, `--user`로 설치했으면 `%APPDATA%\Python\Python313\Scripts`를 PATH에 더한다.