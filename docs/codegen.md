# 코드 생성

**프로토콜·데이터 파일을 고치기 전에 반드시 읽는다.**

생성 결과물을 클라이언트/서버 트리로 **복사**하는 파이프라인이 두 개 있다. 복사본이 git에 커밋되어 있어서 직접 수정해도 되는 파일처럼 보이지만 아니다. **생성기를 다시 돌리는 순간 덮어써진다.**

---

## 와이어 프로토콜

**원본**: `Protocol/Schema/{Enum,Struct,Protocol}.proto`

`.proto`는 cp949다. UTF-8로 저장하지 않는다.

### 재생성

`Protocol/GenPackets.bat` 실행. 이 스크립트는 `Tools/protoc.exe`를 돌리고, `PacketHandlerGenerator.exe`를 방향별로 두 번 실행한 뒤, 결과를 `XCOPY`로 아래에 뿌린다.

생성기가 읽는 템플릿은 `Protocol/Templates/PacketHandler.h`다. 생성기가 템플릿 폴더를 작업 디렉터리 기준 상대 경로로 찾고, 스크립트가 `Protocol/`로 들어간 뒤 생성기를 부르기 때문이다.

| 목적지 | 산출물 |
|---|---|
| `Server/GameServer/Protocol/` | `*.pb.{h,cc}` |
| `Server/GameServer/Network/` | `ServerPacketHandler.h` |
| `Server/DummyClient/Protocol/` | `*.pb.{h,cc}` |
| `Server/DummyClient/Main/` | `ClientPacketHandler.h` |
| `P1/Source/P1/Network/` | `*.pb.{h,cc}` · `.proto` 3개 · `ClientPacketHandler.h` |

### 손으로 쓰는 것

생성되는 건 패킷 핸들러 **헤더뿐**이다. 아래 `.cpp`는 손으로 작성한 파일이고, 새 패킷의 실제 동작은 여기에 구현한다.

- `Server/GameServer/Network/ServerPacketHandler.cpp`
- `P1/Source/P1/Network/ClientPacketHandler.cpp`
- `P1/Source/P1/Online/P1LoginPacketHandlers.cpp`
- `Server/DummyClient/Main/ClientPacketHandler.cpp`

마지막 파일에는 로그인 관리자를 부르는 클라이언트 핸들러(`S_LOGIN`, `S_CREATE_CHARACTER`, `S_DELETE_CHARACTER`,
`S_ENTER_GAME`)의 정의만 둔다. `Network/`는 `Online/`을 부르지 않으므로 이 정의를 `Online/`에 둔다. 선언은 생성된
`ClientPacketHandler.h`의 것을 그대로 쓴다.

### 이름 규칙

**기능적으로 중요하다.** 생성기가 접두사로 핸들러를 나눈다.

- `C_*` — 클라 → 서버
- `S_*` — 서버 → 클라
- 요청/응답 쌍은 이름을 공유한다 (`C_MOVE` / `S_MOVE`)

### 불변식 검사

`ProtocolContractTests`(`Server/GameServerTests/`)가 아래 세 가지를 검사한다. 메시지를 추가하면 이 테스트가 먼저 알려준다.

- `PROTOCOL_MESSAGES(X)` 매크로 목록과 실제 메시지 목록이 일치
- 패킷 ID가 연속이고 유일
- 전 메시지 왕복 (`DescriptorPool` 리플렉션으로 자동 확장)

**메시지가 늘어도 이 테스트를 고치지 않는다.** 고쳐야 한다면 뭔가 잘못된 것이다.

---

## 게임 데이터

**원본**: `DesignData/Original_*.xlsx` (Item, Map, Monster, Quest, Warrior_Level_Data)

**원본에는 수치만 둔다. 에셋 경로 열을 만들지 않는다.** 아이콘, 메시, 몬스터 클래스는
`DT_ItemAssets`와 `DT_MonsterAssets`에 에디터로 넣는다. 두 테이블은 JSON에서 임포트하지 않으며,
행 이름이 `TemplateId`다. 근거는 `docs/adr/0009-keep-asset-references-out-of-design-data.md`에 있다.

### 재생성

`DesignData/GenJsonFile.bat` 실행. 각 워크북을 JSON으로 변환한 뒤 **MOVE**한다.

**산출물은 아래 표의 9개가 전부다.**
— 와일드카드로 적으면 생성기가 만들지 않는 파일이 같은 폴더에 있어도 문서와 모순되지 않는다.

| 목적지 | 산출물 |
|---|---|
| `Server/GameServer/Game/Data/Json/` | `S_Warrior_Level_Data.json` · `S_Monster.json` · `S_Item.json` · `S_Map.json` · `S_Quest.json` |
| `P1/Content/P1/Data/DataTables/` | `C_Monster.json` · `C_Item.json` · `C_Map.json` · `C_Quest.json` |

**이 목록에 없는 `C_` 또는 `S_` 접두사 JSON은 생성기의 산출물이 아니다.** 엑셀 원본을 고쳐도
갱신되지 않는다. 발견하면 쓰이는지 확인하고, 쓰이지 않으면 지운다.

장비 데이터는 `Original_Item.xlsx`의 첫 시트에 있다. 세 시트가 하나의 `C_Item.json`으로
합쳐지므로 장비만 담는 산출물은 없다.

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

서버는 부팅 시 `Gamedata::LoadAllGamedata()`(`Server/GameServer/Game/Data/Gamedata.h`)로 이 파일들을 읽어 템플릿 ID를 키로 하는 `nlohmann::json` 기반 `DataTable`에 올린다.

---

## 생성기 자체

두 생성기 exe는 `Tools/`의 파이썬 소스를 각 툴의 `MakeExe.bat`으로 PyInstaller 번들링해서 만든다.

- `Tools/PacketHandlerGenerator/`
- `Tools/ExcelToJsonConverter/`

결과 exe는 각 툴 폴더 안에 놓이고, 호출하는 `.bat`이 `..\Tools\<툴>\<툴>.exe`로 부른다. `.bat`도 cp949다.