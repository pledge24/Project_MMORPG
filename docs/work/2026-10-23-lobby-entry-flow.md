---
작업: lobby-entry-flow · 시작: 2026-10-23
브랜치: 단계 A `refactor/285-lobby-entry-prep` · 단계 B `feature/285-lobby-entry-flow` · 단계 C `refactor/285-protocol-migration` · 맵 이슈: #285
---

# 로비 분리, 게임 입장 흐름, 프로토콜 재작성

## 목표

- 게임 서버에서 로비가 하는 일(로비 접속, 캐릭터 목록, 생성, 삭제, 선택)이 로비 모듈 하나에 있고, 로비와 게임은 창구 둘로만 잇는다
- 플레이어가 로그인 → 월드 선택 → 로비 접속 → 게임 입장 → 맵 로드 → 룸 입장 순서로 들어가고, 게임에서 로비와 월드 선택으로 돌아갈 수 있다
- `.proto`가 도메인 파일 일곱 개로 다시 짜여 있고, 응답이 실패를 결과 enum으로 싣는다
- 클라이언트의 인증, 로비, 연결, 접속 흐름이 각자 다른 클래스에 있고, 위젯은 protobuf를 모른다

## 세션 시작 절차

1. 지금 단계의 통합 브랜치를 체크아웃한다. 단계가 끝나 `dev`에 들어갔으면 다음 단계의 통합 브랜치를 `dev`에서 딴다. `dev`에 새 커밋이 있으면 merge한다(rebase하지 않는다)
2. 진행 표에서 차단이 모두 「머지」인 첫 「대기」 티켓을 고르고, 그 이슈 본문을 읽는다. 「진행」인 티켓이 있으면 「기록」에서 어디까지 했는지 먼저 읽는다
3. 티켓 브랜치를 통합 브랜치에서 따고 PR은 통합 브랜치로 보낸다. 그 PR에 진행 표의 해당 줄을 「머지」로
   고치는 변경을 함께 넣는다. 머지되면 `close-merged-issues` 워크플로가 PR 본문의 `Closes #N` 이슈를 닫는다.
   닫히지 않았으면 손으로 닫는다

## 지킬 것

- 티켓 PR을 머지한 시점에 게임이 동작한다. 서버를 고친 티켓은 서버 솔루션 빌드(DummyClient 포함)와 `GameServerTests`가 통과하고, DAO를 고친 티켓은 실제 DB 테스트도 통과한다. 클라이언트를 고친 티켓은 클라이언트 빌드와 `Run-UeTests.ps1`이, 인증 서버를 고친 티켓은 `npm test`와 `npm start`가 통과한다
- 단계 A(#286~#289)는 계약을 바꾸지 않는다
- 계약은 확장, 이전, 축소 순서로 바꾼다. #286이 새 스키마 파일 구성을 옛 파일 셋 옆에 더하고, 계약 티켓이 도메인별로 메시지를 옮기면서 옛 파일에서 지우며, #296이 옛 파일을 없앤다
- 새 메시지는 아래 「목표 정의」를 따른다. 목표 정의를 바꿔야 하면 사람에게 먼저 묻고, 승인을 받은 뒤 이 파일의 목표 정의를 고친다
- 로비 모듈은 배선과 게임 입장 창구만 부르고, 게임은 로비 복귀 창구로만 로비를 부른다. 로비와 게임은 서로의 기획 데이터를 읽지 않는다(ADR-0021)
- 인증 서버와 게임 서버 사이의 Redis 계약은 ADR-0020의 표를 따른다
- 스키마 변경은 예상하지 않는다. 생기면 SQL 초안을 사람에게 넘기고, 승인을 받은 뒤 마이그레이션 파일과 함께 진행한다. 마이그레이션의 적용은 사람이 한다
- 위젯 BP는 언리얼 MCP가 연결되면 `ue-mcp` 스킬의 절차로 Claude가 만들고 사람이 확인한다. 고치기 전에 커밋 상태를 확인한다. 연결되지 않으면 바꿀 내용을 지시서로 써서 사람에게 넘긴다. 시안(#291)을 승인받기 전에는 새 화면의 BP를 만들지 않는다
- 클라이언트에서 실제로 해 보는 확인은 사람이 한다. 확인할 항목은 아래 「PIE 시나리오」에서 그 티켓까지의 줄이다
- DummyClient는 계약을 바꾸는 티켓마다 빌드만 통과하게 맞춘다. 실제 인증 티켓을 쓰게 하는 일은 백로그 3번에 남긴다
- 옮기다가 새로 찾은 결함은 그 자리에서 고치지 않는다. 「기록」에 적고 `docs/tech-debt.md`에도 적는다. 이 작업의 티켓이 그 코드를 다시 쓰면 그 티켓 본문에 더한다
- 진행 표의 「검증」에는 실제로 돌린 것만 적는다. 빌드 종료 코드, 단위 테스트, 실제 DB 테스트, UE 테스트, 인증 서버 테스트, 사람의 확인을 구분한다
- 통합 브랜치를 `dev`에 넣을 때는 merge commit으로 한다. 티켓별 커밋이 `dev` 이력에 남아야 한다

## 진행

상태: 대기 · 진행 · 머지 · 막힘

| # | 단계 | 티켓 | 차단 | 상태 | PR | 검증 |
|---|---|---|---|---|---|---|
| 1 | A | #286 스키마를 도메인 파일 구성으로 바꾸고 목표 스키마를 확정하기 | — | 머지 | #297 | 서버 솔루션 빌드 0(DummyClient 포함) · `GameServerTests` 390 통과 · 클라이언트 빌드 0 · `Run-UeTests.ps1` 10 통과 · 생성 스크립트: 문법 오류, 범위 없음, 범위 겹침에서 1, 정상 0 · 규범 검사와 문서 검사 통과 |
| 2 | A | #287 서버의 로비를 로비 모듈로 옮기고 세션 단계를 두기 | — | 머지 | #298 | 서버 솔루션 빌드 0(DummyClient 포함) · `GameServerTests` 407 통과 · 실제 DB 테스트 23 통과 · 규범 검사와 문서 검사 통과 · PIE 확인은 사람을 기다린다 |
| 3 | A | #288 진행의 저장 계층을 로비와 게임의 경계에 맞게 정리하기 | 2 | 진행 | | 서버 솔루션 빌드 0(DummyClient 포함) · `GameServerTests` 421 통과 · 실제 DB 테스트 24 통과 · PIE 확인은 사람을 기다린다 |
| 4 | A | #289 클라이언트 로그인 구조를 지금의 계약 그대로 나누기 | — | 대기 | | |
| 5 | B | #290 로비 계약으로 로비에 접속하고 캐릭터 다루기 | 1, 2, 3, 4 | 대기 | | |
| 6 | B | #291 화면 시안을 확정하고 로그인과 로비 화면 만들기 | 5 | 대기 | | |
| 7 | B | #292 맵이 바뀌는 입장을 맵 로드 절차 하나로 처리하기 | 3, 5 | 대기 | | |
| 8 | B | #293 월드 목록을 보고 월드를 골라 접속하기 | 5, 6 | 대기 | | |
| 9 | B | #294 게임에서 로비와 월드 선택으로 돌아가기 | 7, 8 | 대기 | | |
| 10 | C | #295 엔티티와 전투 계약을 새 규칙으로 옮기기 | 7 | 대기 | | |
| 11 | C | #296 아이템, 퀵슬롯, 채팅 계약을 옮기고 옛 스키마 파일 지우기 | 9, 10 | 대기 | | |

단계 A는 계약을 그대로 둔 채 구조만 바꾸므로, 티켓마다 지금의 흐름으로 PIE에서 확인할 수 있다. 단계 B는 플레이어가 겪는
순서와 달리 로비부터 한다. 로비와 입장이 이 작업의 중심이고, 월드 선택은 그 앞에 끼워 넣는 일이라 나중에 해도 앞의 티켓을
다시 고치지 않는다. #293 전까지 클라이언트는 로그인이 준 인증 티켓으로 설정의 주소에 접속한다.

계약 티켓에서 화면 작업을 떼어 낸 것은 크기 때문이다. 로비 계약에 위젯 BP 다섯 개까지 넣으면 한 세션에 들어가지 않는다.
월드 선택 화면과 게임 메뉴는 그 화면이 없으면 동작을 보일 수 없으므로 #293과 #294에 남겼다.

## PIE 시나리오

사람이 확인하는 항목이다. 티켓마다 그 티켓까지의 줄을 확인한다.

| 줄 | 시작하는 티켓 | 확인할 것 |
|---|---|---|
| 1 | #287 | 로그인 → 캐릭터 목록 → 캐릭터 생성과 삭제 → 게임 입장 → 이동 |
| 2 | #287 | 다른 창에서 같은 계정으로 접속해 앞의 연결 밀어내기 |
| 3 | #288 | 새 캐릭터가 시작 맵의 시작 위치에 서고, 다시 접속하면 마지막 위치에 선다 |
| 4 | #290 | 캐릭터 생성 실패의 문구(이름 중복, 빈 슬롯 없음) |
| 5 | #291 | 다섯 화면이 시안과 같다 |
| 6 | #292 | 같은 맵 안의 포털 이동, 사망과 마을 리스폰 |
| 7 | #293 | 로그인 → 월드 목록과 혼잡도 → 월드 선택 → 로비. 게임 서버를 끄면 15초 안에 그 월드가 목록에서 빠진다 |
| 8 | #294 | 사냥 → 로비 복귀 → 레벨이 반영된 목록 → 다른 캐릭터로 입장. 사망한 채 복귀하면 마을. 월드 선택 복귀 뒤 다시 로그인하지 않고 입장 |
| 9 | #295 | 다른 플레이어 보이기, 몬스터의 추적과 공격, 리스폰 뒤 내 플레이어가 저절로 움직이지 않음 |
| 10 | #296 | 상점 구매와 판매, 착용과 해제, 소모품 사용, 퀵슬롯 등록과 사용, 다시 입장한 뒤의 소지품 |

## 목표 정의

#286이 이 절에 도메인 파일 일곱 개(Common, Lobby, Entry, Entity, Combat, Item, Chat)의 메시지 정의 초안을 쓴다. 사람이
승인하면 승인한 날짜를 이 줄 아래에 적는다. 승인 전에는 계약 티켓을 시작하지 않는다.

승인: 2026년 10월 23일. 「기록」에 적은, 「결정」 표에 없던 일곱 가지도 함께 승인했다.

계약 티켓은 아래 정의를 그 도메인 파일에 옮겨 적는다. 파일 첫머리의 `syntax`, `package`, 번호 범위 줄은 #286이 만든 것을
그대로 두고, `.proto`는 cp949로 저장한다.

**자료형을 새 파일로 옮기는 티켓은 옛 파일의 같은 자료형을 같은 커밋에서 지운다.**
— 옮기는 동안에는 새 파일과 옛 파일이 한 패키지에 함께 있다. 같은 이름의 자료형이 두 파일에 있으면 protoc가 거절한다.

아직 옮기지 않은 자료형이 필요하면 새 파일이 옛 파일(`Enum.proto`, `Struct.proto`)을 import해서 쓴다. 그 자료형을 옮기는 티켓이
import를 바꾼다. 아래 일곱 파일은 2026년 10월 23일에 protoc와 핸들러 생성기를 함께 통과했다.

### 정의 규칙

- 패킷 이름: `C_`와 `S_` 접두사만 생성기에 의미가 있고, 요청과 응답이 같은 이름을 쓰는 것은 관례
- 서버가 먼저 보내는 통지(`S_SPAWN`, `S_CHARACTER_LIST`, `S_GEAR_APPEARANCE`): 짝이 되는 요청 없음
- enum 값: enum 이름을 대문자 snake_case로 바꾼 접두사로 시작. 0번은 `_NONE`, 결과 enum만 0번이 `_SUCCESS`
- 필드 이름: snake_case. C++ 예약어 금지(직업 필드는 `class`가 아니라 `character_class`)
- 응답: 그 도메인의 결과 enum 필드 하나를 1번 필드로 둠. 서버는 화면 문구를 보내지 않음
- 응답하지 않는 요청: `C_MOVE` 하나. 세션 단계에 맞는 나머지 요청에는 언제나 응답
- 수신 대상: 응답과 개인 정보는 요청한 플레이어에게만 감. 다른 클라이언트에도 가는 공개 정보(`EntityInfo`와 통지)에는 맵 번호와 룸 번호를 싣지 않음
- 엔티티 번호: 메시지마다 한 곳에만 실음
- 번호를 바꾸지 않는 enum: DB에 숫자로 저장되는 `ItemType`, `CharacterClass`, `GearPart`, `QuickSlotRefType`
- 이름을 바꾸지 않는 enum: 기획 데이터가 값 이름으로 읽는 `ItemType`
- 자료형의 자리: 그것을 쓰는 도메인 파일. 두 도메인 이상의 패킷이 쓰거나 로비와 게임의 코드가 함께 쓰면 Common. 스탯 자료형 셋(`StatType`, `Stat`, `StatInfo`)은 함께 둔다
- import 방향: Common은 아무것도 import하지 않음. Common이 아닌 도메인 파일을 import하는 것은 Entry 하나(Entity와 Item)

### 흐름

| 흐름 | 패킷 순서 |
|---|---|
| 로비 접속 | `C_CONNECT_LOBBY` → `S_CONNECT_LOBBY` → `S_CHARACTER_LIST`. 실패하면 서버가 `S_CONNECT_LOBBY`를 보낸 뒤 연결을 끊는다 |
| 게임 입장 | `C_ENTER_GAME` → `S_ENTER_GAME` → 맵 로드 → `C_MAP_LOAD_COMPLETE` → `S_ENTER_ROOM`, `S_SPAWN` |
| 같은 맵 안의 포털 이동 | `C_USE_PORTAL` → `S_USE_PORTAL` → `S_ENTER_ROOM`, `S_SPAWN` |
| 맵 간 포털 이동 | `C_USE_PORTAL` → `S_USE_PORTAL`(`map_id` 있음) → 맵 로드 → `C_MAP_LOAD_COMPLETE` → `S_ENTER_ROOM`, `S_SPAWN` |
| 리스폰 | `C_RESPAWN` → `S_RESPAWN` → 맵이 바뀌면 맵 로드와 `C_MAP_LOAD_COMPLETE` → `S_ENTER_ROOM`, `S_SPAWN`. 사망한 룸으로 다시 리스폰해도 룸 퇴장과 룸 입장을 거치므로, 다른 플레이어는 `S_DESPAWN`과 `S_SPAWN`을 받는다 |
| 로비 복귀 | `C_RETURN_TO_LOBBY` → 룸 퇴장과 저장 → `S_RETURN_TO_LOBBY` → `S_CHARACTER_LIST` |
| 같은 계정의 새 로비 접속 | 앞 연결에 `S_DISCONNECT` → 앞 연결 종료 |

`S_ENTER_ROOM`을 받은 클라이언트는 앞 룸의 엔티티를 모두 지운다. 같은 맵 안의 이동과 리스폰을 입장 유형으로 가르지 않는다.

### Common.proto

```protobuf
// 직업. DB의 Character.ClassId에 숫자로 저장된다. 로비의 캐릭터 생성과 게임의 레벨 표, 착용 조건이 함께 쓴다
enum CharacterClass
{
    CHARACTER_CLASS_NONE = 0;
    CHARACTER_CLASS_WARRIOR = 1;
    CHARACTER_CLASS_MAGE = 2;
}

// 장비 부위. 착용 장비의 칸 번호(Item)이자 외형의 부위(Entity)이고, DB의 Item.SlotIndex에 숫자로 저장된다
enum GearPart
{
    GEAR_PART_NONE = 0;
    GEAR_PART_HELMET = 1;
    GEAR_PART_CHEST = 2;
    GEAR_PART_LEGS = 3;
    GEAR_PART_ARMS = 4;
    GEAR_PART_BOOTS = 5;
    GEAR_PART_WEAPON = 6;
}

enum StatType
{
    STAT_TYPE_NONE = 0;
    STAT_TYPE_MAX_HP = 1;
    STAT_TYPE_HP = 2;
    STAT_TYPE_MAX_MP = 3;
    STAT_TYPE_MP = 4;
    STAT_TYPE_PHYSICAL_ATTACK = 5;
    STAT_TYPE_MAGICAL_ATTACK = 6;
    STAT_TYPE_MAX_EXP = 7;          // 지금 레벨의 요구 경험치
    STAT_TYPE_EXP = 8;              // 지금 레벨 안에서 쌓은 경험치
}

// 바뀐 스탯 하나
message Stat
{
    StatType type = 1;
    int64 value = 2;
}

// 크리처의 모든 스탯
message StatInfo
{
    map<int32, int64> values = 1;   // <StatType 값, 스탯 값>
}

// 서버가 연결을 끊는 사유
enum DisconnectReason
{
    DISCONNECT_REASON_NONE = 0;
    DISCONNECT_REASON_DUPLICATE_CONNECTION = 1;     // 같은 계정의 새 로비 접속에 밀려났다
}

// 서버가 연결을 끊기 직전에 보낸다. 세션 단계와 무관하게 온다
message S_DISCONNECT
{
    DisconnectReason reason = 1;
}
```

### Lobby.proto

```protobuf
import "Common.proto";

enum LobbyResult
{
    LOBBY_RESULT_SUCCESS = 0;
    LOBBY_RESULT_INVALID_AUTH_TICKET = 1;   // 인증 티켓이 없거나, 이미 쓰였거나, 다른 월드의 것이다
    LOBBY_RESULT_INVALID_NAME = 2;          // 이름이 비었거나 길이 제한을 넘는다
    LOBBY_RESULT_NAME_TAKEN = 3;            // 이 월드에 같은 이름의 캐릭터가 있다
    LOBBY_RESULT_UNAVAILABLE_CLASS = 4;     // 생성할 수 있는 직업이 아니다
    LOBBY_RESULT_NO_EMPTY_SLOT = 5;         // 빈 캐릭터 슬롯이 없다
    LOBBY_RESULT_CHARACTER_NOT_FOUND = 6;   // 이 계정의 캐릭터가 아니거나 이미 삭제됐다
    LOBBY_RESULT_INTERNAL_ERROR = 7;        // DB 오류처럼 요청을 바꿔도 풀리지 않는 실패
}

// 캐릭터 목록의 항목
message LobbyCharacter
{
    int64 character_id = 1;
    string name = 2;
    CharacterClass character_class = 3;
    int32 level = 4;                        // 마지막으로 저장한 레벨
}

message C_CONNECT_LOBBY
{
    string auth_ticket = 1;
}

// 성공하면 S_CHARACTER_LIST가 뒤따른다. 실패하면 서버가 이 응답을 보낸 뒤 연결을 끊는다
message S_CONNECT_LOBBY
{
    LobbyResult result = 1;
}

// 세션이 로비에 들어갈 때마다 온다. 로비 접속에 성공했을 때와 로비 복귀의 저장이 끝났을 때다
message S_CHARACTER_LIST
{
    repeated LobbyCharacter characters = 1;
    int32 character_slot_count = 2;
}

message C_CREATE_CHARACTER
{
    string name = 1;
    CharacterClass character_class = 2;
}

message S_CREATE_CHARACTER
{
    LobbyResult result = 1;
    LobbyCharacter character = 2;           // 성공했을 때만 있다
}

message C_DELETE_CHARACTER
{
    int64 character_id = 1;
}

message S_DELETE_CHARACTER
{
    LobbyResult result = 1;
    int64 character_id = 2;                 // 요청한 캐릭터 번호
}
```

### Entry.proto

```protobuf
import "Common.proto";
import "Entity.proto";
import "Item.proto";

enum EntryResult
{
    ENTRY_RESULT_SUCCESS = 0;
    ENTRY_RESULT_CHARACTER_NOT_FOUND = 1;       // 이 계정의 캐릭터가 아니거나 삭제됐다
    ENTRY_RESULT_PREVIOUS_SAVE_PENDING = 2;     // 앞 연결의 진행 저장이 정해진 시간 안에 끝나지 않았다
    ENTRY_RESULT_INVALID_PORTAL = 3;            // 플레이어가 있는 룸의 포털이 아니다
    ENTRY_RESULT_TOO_FAR_FROM_PORTAL = 4;
    ENTRY_RESULT_NOT_DEAD = 5;                  // 사망하지 않은 플레이어는 리스폰할 수 없다
    ENTRY_RESULT_UNSUPPORTED_RESPAWN_TYPE = 6;
    ENTRY_RESULT_INTERNAL_ERROR = 7;            // 진행을 불러오거나 저장하지 못했거나, 기획 데이터가 틀려 위치를 정하지 못했다
}

// 리스폰 유형. 지금은 마을 리스폰 하나다
enum RespawnType
{
    RESPAWN_TYPE_NONE = 0;
    RESPAWN_TYPE_TOWN = 1;
}

// 로비에서 고른 캐릭터로 게임에 입장한다. 로비가 받아 캐릭터의 소유를 확인한다
message C_ENTER_GAME
{
    int64 character_id = 1;
}

// 성공하면 클라이언트는 map_id의 맵을 다 연 뒤 C_MAP_LOAD_COMPLETE를 보낸다
message S_ENTER_GAME
{
    EntryResult result = 1;
    int32 map_id = 2;

    // 내 캐릭터의 개인 정보
    StatInfo stats = 3;
    Possession possession = 4;
    repeated QuickSlot quick_slots = 5;     // 모든 퀵슬롯. 빈 퀵슬롯도 싣는다
}

// 서버가 알린 맵을 다 열었다
message C_MAP_LOAD_COMPLETE
{
}

// 내 플레이어가 룸에 들어갔다. C_MAP_LOAD_COMPLETE의 응답이고, 맵이 바뀌지 않는 포털 이동과 리스폰 뒤에도 온다
message S_ENTER_ROOM
{
    EntryResult result = 1;
    EntityInfo my_entity = 2;
}

message C_USE_PORTAL
{
    int32 portal_id = 1;
}

// map_id가 있으면 맵이 바뀐다. 클라이언트는 그 맵을 다 연 뒤 C_MAP_LOAD_COMPLETE를 보낸다.
// 없으면 서버가 바로 S_ENTER_ROOM을 보낸다
message S_USE_PORTAL
{
    EntryResult result = 1;
    optional int32 map_id = 2;
}

message C_RESPAWN
{
    RespawnType respawn_type = 1;
}

// map_id의 뜻은 S_USE_PORTAL과 같다
message S_RESPAWN
{
    EntryResult result = 1;
    optional int32 map_id = 2;
    repeated Stat changed_stats = 3;        // 리스폰으로 회복한 스탯
}

// 서버는 플레이어를 바로 룸에서 퇴장시키고, 접속 종료와 같은 절차로 진행을 저장한다
message C_RETURN_TO_LOBBY
{
}

// 저장이 끝난 뒤에 온다. 성공하면 S_CHARACTER_LIST가 뒤따른다
message S_RETURN_TO_LOBBY
{
    EntryResult result = 1;
}
```

### Entity.proto

```protobuf
import "Common.proto";

enum EntityType
{
    ENTITY_TYPE_NONE = 0;
    ENTITY_TYPE_PLAYER = 1;
    ENTITY_TYPE_MONSTER = 2;
}

enum MoveState
{
    MOVE_STATE_NONE = 0;
    MOVE_STATE_IDLE = 1;
    MOVE_STATE_RUN = 2;
    MOVE_STATE_ACTION = 3;                  // 루트 모션이 들어간 애니메이션 실행
}

message Vector
{
    float x = 1;
    float y = 2;
    float z = 3;
}

message PosInfo
{
    Vector pos = 1;
    float yaw = 2;
    Vector move_direction = 3;
    MoveState state = 4;
}

// 플레이어의 공개 정보
message PlayerInfo
{
    string name = 1;
    int32 level = 2;
    map<int32, int32> gear_appearance = 3;  // <GearPart 값, 착용한 아이템 번호>
}

message MonsterInfo
{
    int32 template_id = 1;
    int64 hp = 2;
}

message EntityInfo
{
    int64 entity_id = 1;
    EntityType entity_type = 2;
    PosInfo pos_info = 3;

    oneof details
    {
        PlayerInfo player_info = 4;
        MonsterInfo monster_info = 5;
    }
}

// 엔티티 하나의 이동
message EntityMove
{
    int64 entity_id = 1;
    PosInfo pos_info = 2;
}

// 룸에 다른 엔티티가 나타났다. 내 엔티티는 싣지 않는다
message S_SPAWN
{
    repeated EntityInfo entities = 1;
}

message S_DESPAWN
{
    repeated int64 entity_ids = 1;
}

// 내 플레이어의 이동. 응답이 없다
message C_MOVE
{
    PosInfo pos_info = 1;
}

// 룸의 플레이어에게 간다. 이동한 엔티티가 플레이어면 그 플레이어는 받지 않는다
message S_MOVE
{
    repeated EntityMove moves = 1;
}

// 플레이어의 착용 장비가 바뀌었다. 착용한 플레이어를 포함한 룸의 모든 플레이어에게 간다.
// template_id가 0이면 그 부위를 비웠다
message S_GEAR_APPEARANCE
{
    int64 entity_id = 1;
    GearPart gear_part = 2;
    int32 template_id = 3;
}
```

### Combat.proto

```protobuf
import "Common.proto";

enum CombatResult
{
    COMBAT_RESULT_SUCCESS = 0;
    COMBAT_RESULT_DEAD = 1;                 // 사망한 플레이어는 공격할 수 없다
}

message C_NORMAL_ATTACK
{
    int32 combo = 1;
    float yaw = 2;
}

message S_NORMAL_ATTACK
{
    CombatResult result = 1;
}

// 크리처가 일반 공격을 했다. 룸의 플레이어에게 가고, 공격한 크리처가 플레이어면 그 플레이어는 받지 않는다
message S_ATTACK
{
    int64 entity_id = 1;
    int32 combo = 2;
    float yaw = 3;
}

// 크리처가 피격됐다
message S_HIT
{
    int64 entity_id = 1;
    int64 damage = 2;
    int64 hp = 3;                           // 피격 뒤 남은 체력
}

message S_DIE
{
    int64 entity_id = 1;
}

message LevelUp
{
    int32 level = 1;                        // 오른 뒤의 레벨
    repeated Stat changed_stats = 2;
}

// 처치 보상을 받았다. 받은 플레이어에게만 간다
message S_REWARD
{
    int64 exp = 1;                          // 보상을 더한 뒤 지금 레벨 안의 경험치
    int64 gold = 2;                         // 보상을 더한 뒤의 골드
    LevelUp level_up = 3;                   // 레벨이 오르지 않았으면 없다
}
```

### Item.proto

```protobuf
import "Common.proto";

enum ItemResult
{
    ITEM_RESULT_SUCCESS = 0;
    ITEM_RESULT_INVALID_REQUEST = 1;        // 정상 클라이언트가 보내지 않는 요청. 없는 아이템 번호, 아이템과 맞지 않는 컨테이너, 범위 밖의 퀵슬롯 번호
    ITEM_RESULT_SLOT_CHANGED = 2;           // 칸의 아이템이 요청의 item_uid와 다르다. 클라이언트의 칸이 서버보다 늦었다
    ITEM_RESULT_NOT_ENOUGH_GOLD = 3;
    ITEM_RESULT_INVENTORY_FULL = 4;         // 아이템이 들어갈 구역에 빈 칸이 없다
    ITEM_RESULT_NOT_SELLABLE = 5;
    ITEM_RESULT_REQUIREMENT_NOT_MET = 6;    // 레벨이나 직업이 착용 조건에 못 미친다
    ITEM_RESULT_GEAR_PART_OCCUPIED = 7;     // 그 부위에 이미 장비를 착용하고 있다
    ITEM_RESULT_COOLDOWN = 8;               // 재사용 대기 중이다
    ITEM_RESULT_DEAD = 9;                   // 사망한 플레이어는 소모품을 쓸 수 없다
    ITEM_RESULT_NOT_OWNED = 10;             // 퀵슬롯이 가리키는 소모품을 갖고 있지 않다
    ITEM_RESULT_INTERNAL_ERROR = 11;        // 검사를 통과한 뒤 칸을 바꾸지 못했다
}

// 아이템 종류. DB의 Item.ItemType에 숫자로 저장되고, 기획 데이터가 이름으로 읽는다
enum ItemType
{
    ITEM_TYPE_NONE = 0;
    ITEM_TYPE_GEAR = 1;
    ITEM_TYPE_CONSUMABLE = 2;
    ITEM_TYPE_MISCELLANEOUS = 3;
}

// 아이템을 담는 컨테이너의 종류. 인벤토리의 구역 셋과 착용 장비다
enum ContainerType
{
    CONTAINER_TYPE_NONE = 0;
    CONTAINER_TYPE_INVENTORY_GEAR = 1;
    CONTAINER_TYPE_INVENTORY_CONSUMABLE = 2;
    CONTAINER_TYPE_INVENTORY_MISC = 3;
    CONTAINER_TYPE_EQUIPPED = 4;            // 착용 장비. 칸 번호가 GearPart 값이다
}

// 퀵슬롯이 가리키는 대상의 종류. DB의 QuickSlot.RefType에 숫자로 저장된다
enum QuickSlotRefType
{
    QUICK_SLOT_REF_TYPE_NONE = 0;
    QUICK_SLOT_REF_TYPE_ITEM = 1;           // 아이템 정의. ref_id가 아이템 번호다
}

message Item
{
    int32 template_id = 1;
    int64 item_uid = 2;
    int32 count = 3;
}

// 칸 하나를 가리키는 주소
message SlotAddress
{
    ContainerType container = 1;
    int32 slot_index = 2;                   // 컨테이너 안의 칸 번호. 착용 장비에서는 GearPart 값이다
}

// 이 주소의 칸은 이제 이 내용이다. item이 없으면 빈 칸이다
message SlotChange
{
    SlotAddress address = 1;
    Item item = 2;
}

// 컨테이너 안의 칸 하나. item이 없으면 빈 칸이다
message Slot
{
    int32 slot_index = 1;
    Item item = 2;
}

// 컨테이너 하나의 모든 칸. 빈 칸도 싣는다
message ContainerSlots
{
    ContainerType container = 1;
    repeated Slot slots = 2;
}

message Possession
{
    int64 gold = 1;
    repeated ContainerSlots containers = 2; // 인벤토리의 구역 셋과 착용 장비
}

// 퀵슬롯이 가리키는 대상. 아이템 하나가 아니라 아이템 정의를 가리킨다
message QuickSlotRef
{
    QuickSlotRefType ref_type = 1;
    int32 ref_id = 2;
}

// 퀵슬롯 하나. ref가 없으면 빈 퀵슬롯이다
message QuickSlot
{
    int32 quick_slot_index = 1;             // 0부터 퀵슬롯 개수 미만
    QuickSlotRef ref = 2;
}

message C_BUY_ITEM
{
    int32 template_id = 1;
}

message S_BUY_ITEM
{
    ItemResult result = 1;
    repeated SlotChange changed_slots = 2;
    int64 gold = 3;
}

// 칸을 가리키는 요청은 칸 주소와, 클라이언트가 그 칸에서 본 아이템의 uid를 싣는다.
// 그 칸의 아이템 uid가 다르면 서버는 ITEM_RESULT_SLOT_CHANGED로 거절한다
message C_SELL_ITEM
{
    SlotAddress address = 1;
    int64 item_uid = 2;
}

message S_SELL_ITEM
{
    ItemResult result = 1;
    repeated SlotChange changed_slots = 2;
    int64 gold = 3;
}

message C_EQUIP_GEAR
{
    SlotAddress address = 1;
    int64 item_uid = 2;
}

// 외형은 S_GEAR_APPEARANCE가 룸에 알린다
message S_EQUIP_GEAR
{
    ItemResult result = 1;
    repeated SlotChange changed_slots = 2;
    repeated Stat changed_stats = 3;
}

message C_UNEQUIP_GEAR
{
    SlotAddress address = 1;
    int64 item_uid = 2;
}

message S_UNEQUIP_GEAR
{
    ItemResult result = 1;
    repeated SlotChange changed_slots = 2;
    repeated Stat changed_stats = 3;
}

message C_USE_ITEM
{
    SlotAddress address = 1;
    int64 item_uid = 2;
}

// C_USE_ITEM과 C_USE_QUICK_SLOT의 응답
message S_USE_ITEM
{
    ItemResult result = 1;
    repeated SlotChange changed_slots = 2;
    repeated Stat changed_stats = 3;
}

// 퀵슬롯 하나에 대상을 등록한다. ref가 없으면 해제한다. 다른 퀵슬롯에 있는 대상이면 두 퀵슬롯이 바뀐다
message C_SET_QUICK_SLOT
{
    int32 quick_slot_index = 1;
    QuickSlotRef ref = 2;
}

message S_SET_QUICK_SLOT
{
    ItemResult result = 1;
    repeated QuickSlot changed_quick_slots = 2;
}

// 퀵슬롯에 등록한 소모품을 쓴다. 쓸 칸은 서버가 고른다. 응답은 S_USE_ITEM이다
message C_USE_QUICK_SLOT
{
    int32 quick_slot_index = 1;
}
```

### Chat.proto

```protobuf
enum ChatResult
{
    CHAT_RESULT_SUCCESS = 0;
    CHAT_RESULT_INVALID_MESSAGE = 1;        // 비었거나 길이 제한을 넘는다
}

message C_CHAT
{
    string message = 1;
}

message S_CHAT
{
    ChatResult result = 1;
}

// 룸의 플레이어가 말했다. 말한 플레이어를 포함한 룸의 모든 플레이어에게 간다
message S_CHAT_MESSAGE
{
    int64 entity_id = 1;
    string message = 2;
}
```

### 이름이 바뀌는 것

| 옛 정의 | 새 정의 |
|---|---|
| `C_LOGIN.access_token` | `C_CONNECT_LOBBY.auth_ticket` |
| `S_LOGIN` | `S_CONNECT_LOBBY`와 `S_CHARACTER_LIST` |
| `CharacterOverview` | `LobbyCharacter`. 생성 요청은 이름과 직업 두 필드만 싣는다 |
| `CLASS_TYPE_*` | `CHARACTER_CLASS_*`. 번호는 그대로다 |
| `GearType`, `GEAR_TYPE_*` | `GearPart`, `GEAR_PART_*`. 번호는 그대로다 |
| `S_LEAVE_GAME`, `LeaveReason` | `S_DISCONNECT`, `DisconnectReason` |
| `LEAVE_REASON_DUPLICATE_LOGIN` | `DISCONNECT_REASON_DUPLICATE_CONNECTION`. 게임 서버에는 로그인이 없다 |
| `LEAVE_REASON_INVALID_TOKEN` | `S_CONNECT_LOBBY`의 `LOBBY_RESULT_INVALID_AUTH_TICKET` |
| `C_ENTER_ROOM`(맵을 연 뒤의 첫 입장) | `C_MAP_LOAD_COMPLETE` |
| `C_ENTER_ROOM`(포털) | `C_USE_PORTAL` |
| `S_ENTER_ROOM.enter_pos` | `S_ENTER_ROOM.my_entity`의 `pos_info` |
| `S_ENTER_GAME.stat_info` | `S_ENTER_GAME.stats` |
| `StatInfo.info` | `StatInfo.values` |
| 응답의 `updated_stat` | `changed_stats` |
| `C_MOVE.info`, `S_MOVE.info` | `C_MOVE.pos_info`, `S_MOVE.moves`(`EntityMove` 목록) |
| `S_HIT.updated_hp` | `S_HIT.hp`. 언제나 싣는다 |
| `LevelUpInfo.new_level`, `.updated_stat` | `LevelUp.level`, `.changed_stats` |
| `PlayerInfo.equipped_gear_summary` | `PlayerInfo.gear_appearance` |
| `S_EQUIP_GEAR`·`S_UNEQUIP_GEAR`의 외형 필드 | `S_GEAR_APPEARANCE` |
| `S_NORMAL_ATTACK`(다른 플레이어에게 가는 것) | `S_ATTACK`. `S_NORMAL_ATTACK`은 공격한 플레이어의 응답이 된다 |
| `S_REWARD_RESULT`, `LevelUpInfo` | `S_REWARD`, `LevelUp` |
| `QuickSlot.slot_index`, 퀵슬롯 요청의 `slot_index` | `quick_slot_index`. 퀵슬롯은 칸이 아니다 |
| `S_CHAT` | `S_CHAT_MESSAGE`. `S_CHAT`은 말한 플레이어의 응답이 된다 |
| `C_CHAT.msg` | `C_CHAT.message` |
| 응답의 `success`, `cause`, `error_message` | 도메인별 결과 enum |

### 옮기지 않는 것

| 옛 정의 | 이유 |
|---|---|
| `C_PING`, `S_PONG` | 보내는 코드가 없다 |
| `C_LEAVE_GAME` | 쓰이는 요청이지만 설계로 뺀다. 클라이언트는 게임을 끌 때 이것을 보내고 바로 소켓을 닫는데, 소켓을 닫기만 해도 서버가 같은 접속 종료 경로를 탄다. 남기면 응답 없는 요청이 하나 는다 |
| `C_ENTER_MAP`, `S_ENTER_MAP` | 클라이언트가 보내지 않는다. 맵 간 이동은 `C_USE_PORTAL`이 맡는다 |
| `EnterType` | 서버가 룸을 고르므로 클라이언트가 입장 유형과 룸 번호를 보내지 않는다 |
| `HitInfo`, `Effect`, `DamageType`, `EffectType` | 어디에서도 쓰지 않는다 |
| `AttackInfo`, `AttackType` | 서버 안에서만 쓰고 패킷에 실리지 않는다. 서버의 자기 타입으로 옮긴다 |
| `GearInfo`, `Item.gearInfo` | 클라이언트가 읽지 않는다. 강화 수치와 내구도는 서버 타입과 DB에 남는다 |
| `RewardType`, `Reward`, `LevelUpInfo.old_level` | 클라이언트가 읽지 않는다 |
| `Stat.is_gauge` | 어디에서도 쓰지 않는다 |
| `PlayerInfo`의 `character_id`, `class`, `map_id`, `room_id` | 다른 클라이언트가 읽지 않는다. 서버는 진행 저장에 쓰지만, DB 계층이 Protocol 타입을 모르게 되면서 서버 타입으로 옮긴다 |
| `PosInfo.entity_id` | `EntityInfo.entity_id`, `EntityMove.entity_id`와 겹친다 |
| `S_ENTER_GAME.player` | 내 엔티티 정보는 `S_ENTER_ROOM`이 싣는다 |
| `C_BUY_ITEM.count`, `C_SELL_ITEM.count` | 서버가 읽지 않는다 |
| `C_RESPAWN.respawn_data` | 아무도 채우지도 읽지도 않는다 |
| `S_RESPAWN`의 `respawn_type`, `room_id`, `entity_id`, `pos_info` | 앞의 둘은 클라이언트가 읽지 않는다. 엔티티와 위치는 `S_ENTER_ROOM`이 싣는다 |
| `S_USE_ITEM.entity_id` | 응답은 요청한 플레이어에게만 간다 |
| `RespawnType`의 `TOWN`이 아닌 값 일곱 개 | 서버의 빈 case뿐이고 클라이언트가 보내지 않는다 |
| `EntityType`의 `PROJECTILE`, `ENV` | 어디에서도 쓰지 않는다 |

## 결정

| 결정 | 틀렸다는 신호 |
|---|---|
| 로비는 게임 서버 프로젝트 안의 최상위 모듈 폴더다. 로비와 게임은 게임 입장 창구와 로비 복귀 창구로만 잇고, 오가는 값은 계정 번호와 캐릭터 번호다(ADR-0021) | 로비가 착용 장비처럼 게임의 진행을 읽어야 한다. 창구로 오가는 값이 늘어난다 |
| 캐릭터 선택은 로비가 받아 소유를 확인하고 게임 입장 창구로 넘긴다. 저장 대기, 불러오기, 룸 배정은 게임이 한다 | 프로세스를 나눌 때 입장 티켓만으로 넘길 수 없는 값이 생긴다 |
| 기획 데이터는 소비자별로 나눈다. 로비는 기본 슬롯 수와 생성할 수 있는 직업, 게임은 새 캐릭터의 초기 진행을 갖는다. 교차 검증은 기동할 때 배선이 한다 | 로비와 게임이 함께 읽어야 하는 표가 생긴다 |
| 캐릭터 생성은 캐릭터 행만 쓰고, 첫 게임 입장이 같은 트랜잭션에서 초기 진행 행을 만든다 | 게임에 입장한 적 없는 캐릭터의 진행을 로비가 보여 줘야 한다 |
| 세션에 단계를 두고 핸들러가 받는 단계를 표 하나로 정한다. 단계는 로비 접속 전, 로비 접속 중(#290이 더한다), 로비, 게임 입장 중, 게임이고, 로비 복귀 중은 #294가 더한다. 단계에 맞지 않는 패킷은 로그를 남기고 버린다 | 정상 클라이언트의 패킷이 단계가 바뀌는 순간에 자주 버려져 동작이 어긋난다 |
| 로그인은 로그인 세션(로그인 시각부터 1시간)과 월드 목록을 돌려주고, 월드 선택이 정지를 다시 확인한 뒤 그 월드에 묶인 인증 티켓을 발급한다(ADR-0020) | 월드 선택으로 돌아올 때마다 로그인 화면으로 간다는 불만이 나온다 |
| 게임 서버가 자기 월드 항목을 5초마다 TTL 15초로 갱신하고, 꺼진 월드는 목록에서 사라진다 | 꺼진 월드를 「점검」으로 보여 달라는 요구가 나온다(백로그 37번) |
| 혼잡도는 정원 대비 세 단계이고 월드 선택을 막지 않는다 | 정원을 넘는 접속이 생긴다(백로그 35번) |
| 맵이 바뀌는 모든 입장(게임 입장, 맵 간 이동, 다른 맵으로의 리스폰)이 같은 맵 로드 절차를 따른다. 맵 로드 시간 제한은 두지 않는다 | 로드 완료를 보내지 않는 연결이 세션을 붙든다(백로그 36번) |
| 게임 입장 응답은 맵 번호와 내 캐릭터의 개인 정보를, 룸 입장 응답은 내 엔티티를, 스폰은 다른 엔티티를 싣는다 | 룸에 들어가기 전에 엔티티 번호가 필요한 기능이 생긴다 |
| 로비 복귀는 바로 룸에서 퇴장하고 접속 종료와 같은 저장 절차를 탄다. 캐릭터 목록은 저장이 끝난 뒤에 보낸다. 월드 선택 복귀는 접속 종료다 | 전투 중에 복귀해 피해를 피한다(백로그 34번) |
| 프로토콜은 처음부터 다시 짠다. 칸과 퀵슬롯 메시지의 의미는 그대로 옮기고, DB에 숫자로 저장되는 enum 번호는 바꾸지 않는다 | - |
| 스키마는 도메인 파일 일곱 개이고, 패킷 번호는 파일별 범위 안에서 선언 순서로 매긴다. 생성 스크립트는 PowerShell이고 실패를 종료 코드로 알린다 | 서로 다른 빌드의 클라이언트와 서버가 붙어야 해서 한 파일 안의 번호 밀림이 문제가 된다 |
| 응답마다 도메인별 결과 enum 하나로 실패를 싣고, 서버는 화면 문구를 보내지 않는다. 세션 단계에 맞는 요청에는 언제나 응답하고, 예외는 이동 요청 하나다 | 서버만 아는 사유를 화면에 보여 줘야 한다 |
| DB 계층은 Protocol 타입을 모른다. 복제되는 엔티티 상태(엔티티 정보, 위치, 스탯)만 protobuf로 든다 | 엔티티 상태를 패킷으로 옮기는 자리에서 필드 누락이 반복된다 |
| 클라이언트는 인증 클라이언트, 로비 서브시스템, 연결 서브시스템, 접속 흐름 서브시스템으로 나눈다. 위젯은 protobuf와 송신 창구를 모른다 | 한 단계의 변경이 서브시스템 여럿을 함께 고치게 만든다 |
| 화면은 시안을 사람이 승인한 뒤 언리얼 MCP로 만든다 | - |

## 기록

- 2026-10-23: 설계 논의에서 위 결정을 정했다. 판단 과정과 고르지 않은 대안은 `docs/reports/2026-10-23-lobby-entry-flow-design.html`에 있다(git이 추적하지 않는다). 용어는 `CONTEXT.md`의 「접속 흐름」 절에, 되돌리기 어려운 결정은 ADR-0020과 ADR-0021에 있다. 설계 중에 서버 코드를 읽다가 찾은 결함 둘을 TD-071과 TD-072로 적었고 #287이 고친다. 백로그 26번과 32번, 7번의 B3과 B7을 이 작업이 가져갔고, 미룬 다섯 가지를 백로그 34~38번에 적었다
- 2026-10-23: #286이 스키마를 도메인 파일 일곱 개와 옛 파일 셋으로 구성하고 생성 경로를 바꿨다. 새 파일은 번호 범위만 있고 비어 있다. 옛 `Protocol.proto`는 1000~1999 임시 범위라 지금의 패킷 번호가 그대로다. 생성 스크립트는 `Protocol/Generate.ps1`이고 서버 빌드의 재생성 타깃도 이 스크립트를 부른다. 생성기가 읽은 스키마 파일과 범위를 핸들러 헤더에 적고, 계약 테스트가 자기 표와 대조한다. 「목표 정의」 초안을 썼고 일곱 파일이 protoc와 생성기를 통과했다. 초안 승인과 PIE 확인은 사람을 기다린다
- 2026-10-23: #286의 초안이 「결정」 표에 없던 일곱 가지를 정했다. 초안을 승인할 때 함께 본다. (1) 리스폰을 Entry 파일에 둔다. 맵이 바뀌는 리스폰은 #292의 맵 로드 절차를 따른다. (2) 사망한 룸으로 리스폰해도 룸 퇴장과 룸 입장을 거친다. (3) `S_ENTER_ROOM`을 받은 클라이언트는 앞 룸의 엔티티를 모두 지우고, 입장 유형을 쓰지 않는다. (4) 클라이언트가 게임을 끌 때 보내던 `C_LEAVE_GAME`을 옮기지 않는다. (5) 공격과 채팅은 요청한 플레이어의 응답(`S_NORMAL_ATTACK`, `S_CHAT`)과 룸에 가는 통지(`S_ATTACK`, `S_CHAT_MESSAGE`)를 나눈다. (6) 착용 외형은 Entity의 통지 `S_GEAR_APPEARANCE`가 착용한 플레이어를 포함한 룸 전체에 알린다. (7) 캐릭터 목록은 로비 접속과 로비 복귀 뒤에 통지 `S_CHARACTER_LIST`로 따로 보낸다
- 2026-10-23: #287이 로비 패킷의 핸들러, 캐릭터 생성 규칙, 계정 행과 캐릭터 행의 DAO를 `Server/GameServer/Lobby/`로 옮기고 세션에 네 단계(로비 접속 전, 로비, 게임 입장 중, 게임)와 단계별 허용표를 두었다. 캐릭터 선택은 로비가 소유를 확인한 뒤 게임 입장 창구 `GameEntry::RequestEnter`로 넘긴다. 게임 쪽 `CharacterStateDAO`가 이름 버퍼 크기 때문에 생성 규칙 헤더를 include하고 있어서, 열 크기 상수를 `DB/DAOCommon.h`로 뺐다. 단계를 두다가 찾은 결함 하나(로비 접속을 마치기 전의 두 번째 로비 접속)를 TD-073으로 적었다. 계약은 바꾸지 않았다
- 2026-10-23: #288이 기획 데이터를 로비 표(`S_Lobby.json`: 기본 슬롯 수, 생성할 수 있는 직업)와 게임의 초기 진행 표(`S_Initial_Progress.json`)로 나누고, 기동할 때 `Core/DataCrossCheck`가 생성할 수 있는 직업마다 레벨 표가 있는지 대조하게 했다. 캐릭터 생성은 `Character` 행만 쓰고, 게임 입장의 불러오기가 트랜잭션 하나 안에서 진행 행이 없으면 초기 진행 행을 만든다. 진행 사본은 서버 필드가 됐고 맵 번호와 룸 번호는 `Player`의 필드로 옮겼다. 입장 응답의 `player_info`에는 두 값을 채워 계약은 그대로다. 다른 클라이언트에 가는 엔티티 정보에는 두 값이 더 이상 실리지 않는데, 클라이언트가 읽지 않는 필드다. DB 계층에서 Protocol 타입을 빼면서 로비 DAO의 목록 행(`CharacterListRow`)과 아이템 행의 종류(`ItemRowType`)도 서버 타입으로 바꿨다. 아이템 행의 종류와 구역의 짝은 `InventorySections`와 따로 있고 `ItemRowsTest`가 둘을 대조한다
- 2026-10-23: #287의 리뷰에서 나온 판단 둘을 사람이 승인했고 #290 본문에 더했다. (1) 같은 계정의 앞 연결을 밀어내는 일(등록, 진행 조율자에 알림, 끊기)을 배선 함수 하나로 묶어 로비가 `ProgressCoordinator`와 플레이어를 모르게 한다. #287은 이 일을 로비 접속 잡이 직접 하도록 옮겨 두었고, #290이 `S_DISCONNECT`로 끊기를 다시 쓰므로 함께 한다. (2) 세션 단계에 「로비 접속 중」을 더해 TD-073을 막는다. 새 계약에서는 로비 접속에 실패하면 연결을 끊으므로 로비 접속 전으로 되돌리는 전이가 없다. 「결정」 표의 세션 단계 줄을 고쳤다
