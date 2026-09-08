# 코드 생성

**프로토콜·데이터 파일을 고치기 전에 반드시 읽는다.**

생성 결과물을 클라이언트/서버 트리로 **복사**하는 파이프라인이 두 개 있다. 복사본이 git에 커밋되어 있어서 직접 수정해도 되는 파일처럼 보이지만 아니다. **생성기를 다시 돌리는 순간 덮어써진다.**

---

## 와이어 프로토콜

**원본**: `Server/Common/Protobuf/bin/{Enum,Struct,Protocol}.proto`

`.proto`는 cp949다. UTF-8로 저장하지 않는다.

### 재생성

`Server/Common/Protobuf/bin/GenPackets.bat` 실행. 이 스크립트는 `protoc.exe`를 돌리고, `PacketHandlerGenerator.exe`를 방향별로 두 번 실행한 뒤, 결과를 `XCOPY`로 아래에 뿌린다.

| 목적지 | 산출물 |
|---|---|
| `Server/GameServer/Protocol/` | `*.pb.{h,cc}` |
| `Server/GameServer/Main/` | `ServerPacketHandler.h` |
| `Server/DummyClient/Protocol/` | `*.pb.{h,cc}` |
| `Server/DummyClient/Main/` | `ClientPacketHandler.h` |
| `P1/Source/P1/Network/` | `*.pb.{h,cc}` 및 `.proto` 3개 |
| `P1/Source/P1/` | `ClientPacketHandler.h` (`Network/`가 아니라 **모듈 루트**) |

### 손으로 쓰는 것

생성되는 건 패킷 핸들러 **헤더뿐**이다. `.cpp` 두 개는 손으로 작성한 파일이고, 새 패킷의 실제 동작은 여기에 구현한다.

- `Server/GameServer/Main/ServerPacketHandler.cpp`
- `P1/Source/P1/ClientPacketHandler.cpp`

### 이름 규칙

**기능적으로 중요하다.** 생성기가 접두사로 핸들러를 나눈다.

- `C_*` — 클라 → 서버
- `S_*` — 서버 → 클라
- 요청/응답 쌍은 이름을 공유한다 (`C_MOVE` / `S_MOVE`)

### 불변식 검사

`ProtocolContractTests`(`Server/GameServerTests/`)가 다음을 검사한다. 메시지를 추가하면 이 테스트가 먼저 알려준다.

- `PROTOCOL_MESSAGES(X)` 매크로 목록과 실제 메시지 목록이 일치
- 패킷 ID가 연속이고 유일
- 전 메시지 왕복 (`DescriptorPool` 리플렉션으로 자동 확장)

**메시지가 늘어도 이 테스트를 고치지 않는다.** 고쳐야 한다면 뭔가 잘못된 것이다.

---

## 게임 데이터

**원본**: `Server/Common/GameDatasheet/Original_*.xlsx` (Item, Map, Monster, Quest, Warrior_Level_Data)

### 재생성

`Server/Common/GameDatasheet/GenJsonFile.bat` 실행. 각 워크북을 JSON으로 변환한 뒤 **MOVE**한다.

| 산출물 | 목적지 |
|---|---|
| `S_*.json` | `Server/GameServer/Game/Data/Json/` |
| `C_*.json` | `P1/Content/Gamedata/` |

### 로딩

서버는 부팅 시 `Gamedata::LoadAllGamedata()`(`Server/GameServer/Game/Data/Gamedata.h`)로 이 파일들을 읽어 템플릿 ID를 키로 하는 `nlohmann::json` 기반 `DataTable`에 올린다.

---

## 생성기 자체

두 생성기 exe는 `Server/Tools/`의 파이썬 소스를 각 툴의 `MakeExe.bat`으로 PyInstaller 번들링해서 만든다.

- `Server/Tools/PacketHandlerGenerator/`
- `Server/Tools/ExcelToJsonConverter/`

결과 exe는 호출하는 `.bat` 옆에 놓인다. `.bat`도 cp949다.