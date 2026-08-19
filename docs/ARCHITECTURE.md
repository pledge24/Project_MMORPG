# ARCHITECTURE (as-is)

세션 1(2026-08-19)에서 실제 코드·라이브 DB·UE 에디터와 대조해 검증했다.
`/init` 초안에 있던 서버 경로 오류를 바로잡고 "미작성" 3개 항목을 채웠다.

## 로그인은 3티어를 모두 거친다

1. 클라의 `ULoginManager`(`P1/Source/P1/Login/LoginManager.cpp:29`)가 인증 서버로
   `POST http://127.0.0.1:5000/Login`. 회원가입은 `POST /Account/Register`
   (`Server/AuthServer/src/routes/account.router.js`)로 별도 라우터다.
2. `Server/AuthServer/src/routes/login.router.js`가 `UserDB.dbo.Users`의 bcrypt 해시를 검증하고,
   `last_login`을 갱신한 뒤 UUID 액세스 토큰을 발급해
   `accessToken:<uuid>` → `{userId, username}`을 `ACCESS_TOKEN_TTL`과 함께 Redis에 저장.
3. 클라는 토큰을 `UP1GameInstance::_token`에 보관하고, `LoginManager.cpp:122`에서
   `ConnectToGameServer()`를 호출해 `127.0.0.1:7777`로 TCP 소켓을 연 뒤 `C_LOGIN { access_token }` 전송.
4. `Handle_C_LOGIN`(`Server/GameServer/Main/ServerPacketHandler.cpp:33`)이 **DBQueue 위에서**
   Redis를 되읽고, 맞으면 `GameSession::userId`를 채운 뒤
   `DBRequestFunctions::LoadUserCharactersData()`로 `GameDB`에서 캐릭터를 로드.

즉 인증 티어와 게임 티어를 잇는 건 Redis뿐이다. 게임 서버는 `UserDB`를 직접 건드리지 않는다.

## 게임 서버 스레딩

`Server/GameServer/Main/GameServer.cpp`의 `main()`은 패킷 핸들러 테이블 초기화 → 게임 데이터 로드 →
맵 템플릿마다 `Room` 생성 → 리스너 시작 → DB/Redis 연결 순으로 부팅한 뒤, 워커 스레드 5개와
DB 스레드 5개를 띄운다 (메인 스레드가 6번째 워커가 된다).

각 워커는 64ms 틱 예산(`WORKER_TICK`)으로 `IocpCore::Dispatch(10)` →
`ThreadManager::DistributeReservedJobs()` → `ThreadManager::DoGlobalQueueWork()`를 반복한다.
따라서 패킷 핸들러는 IOCP 워커 스레드에서 돈다.

동시성 모델: `Room`이 `JobQueue`를 상속한다(`Server/GameServer/Game/Room/Room.h:15`). 핸들러는
인라인으로 일을 거의 하지 않고 잡만 밀어넣고 리턴한다. 예: `room->DoAsync(&Room::C_HandleMove, pkt)`.
룸/오브젝트 상태 변경은 전부 해당 룸의 큐에서 직렬화되므로, 큐 위에 머무는 한 룸 소유 상태에는
락이 필요 없다. DB 작업도 같은 식으로 분리한다 — 핸들러가 `DBQueue`에 `Job`을 push하고
(유저 친화도가 필요하면 `GDBManager->GetDBQueueFromId(userId)`, 아니면 랜덤) 전용 DB 스레드가 소비한다.

> **예외 2건 (부채로 기록됨)** — `Handle_C_ENTER_MAP`은 `player->OnEnterMap()`과 `SEND_PACKET`을
> 룸 큐 밖에서 직접 실행하고(`ServerPacketHandler.cpp:216-225`), `Handle_C_ENTER_GAME`은
> `ObjectUtils::CreatePlayer()`를 인라인 호출한다(`:125`).

`Room`은 공간 분할용 셀 행렬(`CELL_SIZE = 1000.f`)도 유지하며 `FindClosestPlayer`와 브로드캐스트
범위 계산에 쓴다. 룸 갱신 주기는 `ROOM_UPDATE_INTERVAL_MS = 200`.

## 클라이언트 스레딩

`PacketSession`(`P1/Source/P1/Network/PacketSession.h`)이 `RecvWorker`와 `SendWorker`를 소유하고,
둘 다 `FRunnable` 스레드다. 수신 바이트는 큐에 쌓이고 게임 스레드의
`UP1GameInstance::HandleRecvPackets()`(`P1GameInstance.cpp:97`)가 이를 비운다 —
네트워크 스레드는 UObject를 절대 만지지 않는다.

**이 펌프를 호출하는 C++ 코드는 없다.** `HandleRecvPackets()`는 `BlueprintCallable`이고,
호출부는 **레벨 블루프린트의 `ReceiveTick`** 안에 있다. UE 에디터로 실측한 결과:

| 레벨 | 레벨 스크립트 BP | 펌프 호출 |
|---|---|---|
| `Maps/InGameMap.umap` | 있음 (`ReceiveTick`, `ReceiveBeginPlay`, var `MyPlayer`) | ✅ |
| `Maps/LoginMap.umap` | 있음 (`ReceiveTick`) | ✅ |
| `Maps/TestMap.umap` | 있음 (`ReceiveTick`) | ✅ |
| `Maps/TownMap.umap` | 없음 | ❌ |
| `Maps/CrashTestMap.umap` | 없음 | ❌ |

새 레벨을 만들면 레벨 BP에 이 호출을 손으로 넣어야 네트워킹이 산다. 잊으면 조용히 죽는다.
(게임플레이 룸 4개는 전부 `MapId: 1111` = `InGameMap` 하나 안에 있는 논리 분할이라 지금은 사고가 안 났다.)

`UP1GameInstance`가 허브 역할로 소켓과 세션을 소유하고 모든 `HandleXxx(const Protocol::S_XXX&)`
핸들러를 구현하며, 게임플레이 액터와 UMG 위젯으로 멀티캐스트 델리게이트
(`OnRecvBuyItemPkt`, `OnRecvSellItemPkt`, `OnRecvUseItemPkt`, `OnRecvEquipGearPkt`,
`OnRecvUnequipGearPkt`)를 통해 전파한다.

## 클라/서버 클래스 계층이 대칭이다

양쪽 모두 `Object → Creature → { Player, Monster }` 형태를 공유한다.

- 서버: `Server/GameServer/Game/Object/{Object,Creature,Player,Monster}.h`
- 클라: `P1/Source/P1/Game/Objects/{Creature,P1Player,P1MyPlayer,Monster}.h`

서버 오브젝트는 상태를 protobuf 메시지(`Protocol::ObjectInfo`, `PosInfo`, `StatInfo`)로 직접 들고
있어서 복제가 변환이 아니라 복사다. 게임플레이를 바꾸면 보통 양쪽을 대칭으로 고치고 프로토콜도
같이 손대야 한다 — 3곳 수정을 기본으로 생각할 것.

---

## DB 스키마

두 DB는 **서로 다른 LocalDB 인스턴스**에 있다. 접속 문자열도 출처가 다르다.

| DB | 인스턴스 | 설정 위치 | 소유 티어 |
|---|---|---|---|
| `UserDB` | `(localdb)\MSSQLLocalDB` | `Server/AuthServer/.env` (`DB_CONNECTION_STRING`) | 인증 서버 전용 |
| `GameDB` | `(localdb)\ProjectModels` | `Server/GameServer/config.h` (`ENV_DB_CONNECTION_STRING`, 컴파일 타임 상수) | 게임 서버 전용 |

### UserDB — 테이블 1개

`Server/Queries/UserDB_CreateUsersTable.sql`

| 테이블 | 용도 | 키 컬럼 |
|---|---|---|
| `Users` | 계정. bcrypt 해시 보관 | `user_id INT IDENTITY` PK · `username NVARCHAR(50)` UNIQUE · `password_hash NVARCHAR(255)` · `created_at` · `last_login` |

### GameDB — 테이블 5개 + 저장 프로시저 1개

`Server/GameServer/Queries/GameDB_CreateAllTables.sql` (+ `AlterTable.sql` 적용 완료)

| 테이블 | 용도 | 키 / 주요 컬럼 |
|---|---|---|
| `Characters` | 캐릭터 기본 정보 | PK `character_id BIGINT IDENTITY` · `user_id BIGINT` · `class_id` · `character_name NVARCHAR(50)` UNIQUE · `level SMALLINT` · `last_login` · `created_at` |
| `CharactersLastState` | 접속 종료 시점의 상태 (1:1) | PK/FK `character_id` · `cur_hp` `cur_mp` `cur_physical_attack` `cur_magical_attack` · `room_id`(기본 10) `map_id`(기본 1111) · `pos_x/y/z` `rot_yaw` · `exp` · `gold`(기본 10000) |
| `CharactersGearItems` | 장비 아이템 (인벤토리 + 착용) | PK `(character_id, slot_id, is_equipped)` · `item_uid BIGINT` · `template_id` · `enhance` · `durability` · `additional_physical_attack` · `additional_magical_attack` |
| `CharactersConsumableItems` | 소비 아이템 | PK `(character_id, slot_id)` · `template_id` · `count` |
| `CharactersMiscItems` | 기타 아이템 | PK `(character_id, slot_id)` · `template_id` · `count` |
| `GetMaxItemUID` (routine) | 서버 부팅 시 `GNextItemUID` 시드 | `DBRequestFunctions::GetMaxItemUID()`가 `GameServer.cpp:102`에서 1회 호출 |

아이템 4개 테이블 모두 `character_id`에 FK `ON DELETE CASCADE`.
`sysdiagrams`는 SSMS 다이어그램용 자동 생성 테이블로 애플리케이션과 무관하다.

> **교차 티어 타입 불일치** — `Users.user_id`는 `INT`인데 `Characters.user_id`와
> `GameSession::userId`는 `BIGINT`/`int64`다. 지금은 값이 작아 문제가 안 드러난다.

---

## 클라 폴더 현황 (레이어 분리 관점)

`P1/Source/`에는 모듈이 **2개** 있다.

| 모듈 | 내용 |
|---|---|
| `P1/` | 게임 모듈. `.pb.*` 제외 6,546줄 |
| `ProtobufCore/` | protobuf 벤더링. `Include/google/**` + `Lib/Win64/libprotobuf.lib`(16MB, `*.lib` ignore를 뚫고 강제 추적) |

`P1` 모듈 구조:

```
P1/Source/P1/
├── P1GameInstance.{h,cpp}     620줄 — 소켓·세션 소유 + S_* 핸들러 16개 + 스폰/디스폰 + 델리게이트
├── ClientPacketHandler.{h,cpp} 생성 헤더 + 손코딩 cpp (287줄)
├── BP_Structs.h / Types.h / Macro.h / P1GameModeBase
├── Network/                    PacketSession, NetworkWorker, SendBuffer, *.pb.*, *.proto
├── Login/                      LoginManager, LoginMenuMode, LoginMenuPlayerController
├── Log/                        LogCategory
└── Game/
    ├── Objects/                Creature(258) · P1Player · P1MyPlayer(257) · Monster
    ├── Widgets/                12개 위젯 (Inventory 225 · Login 190 · HUD 121 · StatusWindow 113 …)
    ├── Components/             AttackSystemComponent (유일한 컴포넌트)
    ├── Subsystem/              MyPlayerData · StatefulObjectManager
    ├── Structs/                ItemData · MapData · MonsterData · QuestData · SlotData …
    ├── Props/                  Portal · FieldBoundaryWall
    ├── Enums/                  EObject
    ├── InGamePlayerController.{h,cpp}  위젯 7종의 클래스/인스턴스 쌍 + Z-order 관리
    ├── Inventory / EquippedGear / ObjectSpawner
```

**진단**: 폴더가 도메인(전투·인벤토리·이동·UI)이 아니라 **UE 타입**(액터/위젯/컴포넌트/구조체)으로
갈려 있다. `Inventory`와 `EquippedGear`가 `Game/` 바로 아래에 있고 그 데이터 구조체는
`Game/Structs/`에 있는 식이라, 한 기능을 고치려면 3~4개 폴더를 오간다.
`P1.Build.cs`의 `PrivateIncludePaths`가 이 하위 폴더를 전부 등록해 include를 평탄하게 만들어 두어서
구조적 결합이 컴파일러에 드러나지 않는다 — 폴더를 옮겨도 빌드가 깨지지 않는 대신,
경계 위반도 빌드가 잡아주지 않는다.

---

## 프로토콜 ↔ 핸들러 매핑

원본 `Server/Common/Protobuf/bin/Protocol.proto` — 메시지 **40개** = `PKT_* 1000~1039`.
접두사가 방향을 정한다: `C_*` 18개(클라→서버), `S_*` 22개(서버→클라).

### 서버 수신 (`C_*` 18개) — `Server/GameServer/Main/ServerPacketHandler.cpp`

| 처리 경로 | 패킷 |
|---|---|
| **DBQueue** (`GDBManager`) | `C_LOGIN` `C_CREATE_CHARACTER` `C_DELETE_CHARACTER` `C_ENTER_GAME` |
| **Room 큐** (`room->DoAsync`) | `C_ENTER_ROOM` `C_MOVE` `C_CHAT` `C_NORMAL_ATTACK` `C_BUY_ITEM` `C_SELL_ITEM` `C_EQUIP_GEAR` `C_UNEQUIP_GEAR` `C_USE_ITEM` `C_RESPAWN` |
| **혼합/인라인** | `C_LEAVE_GAME`(Room 큐 + DBQueue + `Disconnect`) · `C_ENTER_MAP`(**큐 밖 인라인**) |
| **스텁** (`return false`) | `C_PING` `C_MAP_LOAD_COMPLETE` |

`Room`의 대응 진입점은 `Room::C_Handle*`(`Room.h:38-47`)이고, 그 아래에 큐 내부 전용
`HandleNormalAttack` / `HandleHit` / `HandleMonsterKill` / `HandleDie` / `HandleRespawn`이 있다.

### 클라 수신 (`S_*` 22개) — `P1/Source/P1/ClientPacketHandler.cpp`

| 처리 경로 | 패킷 |
|---|---|
| **`UP1GameInstance::HandleXxx()`** (16개) | `S_ENTER_GAME` `S_ENTER_MAP` `S_ENTER_ROOM` `S_SPAWN` `S_DESPAWN` `S_MOVE` `S_NORMAL_ATTACK` `S_HIT` `S_BUY_ITEM` `S_SELL_ITEM` `S_EQUIP_GEAR` `S_UNEQUIP_GEAR` `S_USE_ITEM` `S_DIE` `S_REWARD_RESULT` `S_RESPAWN` |
| **`ULoginWidget`** (컨트롤러→`ULoginManager`→위젯 4단 캐스트) | `S_LOGIN` `S_CREATE_CHARACTER` `S_DELETE_CHARACTER` |
| **소켓 직접 닫기** | `S_LEAVE_GAME` |
| **로그만** | `S_CHAT` (채팅 UI 없음) |
| **스텁** | `S_PONG` |

### 생성 파이프라인이 결과물을 뿌리는 곳

`Server/Common/Protobuf/bin/GenPackets.bat`의 `XCOPY` 목적지 (직접 수정하면 덮어써진다):

| 산출물 | 목적지 |
|---|---|
| `*.pb.{h,cc}` | `Server/GameServer/Protocol/` · `Server/DummyClient/Protocol/` · `P1/Source/P1/Network/` |
| `ServerPacketHandler.h` | `Server/GameServer/Main/` |
| `ClientPacketHandler.h` | `Server/DummyClient/Main/` · `P1/Source/P1/` (**모듈 루트**) |
| `.proto` 3개 | `P1/Source/P1/Network/` |

`.cpp`(`Server/GameServer/Main/ServerPacketHandler.cpp`, `P1/Source/P1/ClientPacketHandler.cpp`)는
손으로 작성한 파일이고 생성 대상이 아니다.

### 게임 데이터 파이프라인

`Server/Common/GameDatasheet/GenJsonFile.bat`의 `MOVE` 목적지:

| 산출물 | 목적지 |
|---|---|
| `S_{Item,Map,Monster,Quest,Warrior_Level_Data}.json` | `Server/GameServer/Game/Data/Json/` |
| `C_{Item,Map,Monster,Quest}.json` | `P1/Content/Gamedata/` |

서버는 부팅 시 `Gamedata::LoadAllGamedata()`(`Server/GameServer/Game/Data/Gamedata.h:14`)로 읽어
`using DataTable = unordered_map<int32, Json>` 형태의 정적 테이블
(`ItemDataTable` `MapDataTable` `MonsterDataTable` `QuestDataTable` `WarriorLevelDataTable`)에 올린다.

> `P1/Content/Gamedata/`에는 생성기 목록에 없는 `C_Equipment.json` · `C_Gear.json`도 있다 (출처 불명).
