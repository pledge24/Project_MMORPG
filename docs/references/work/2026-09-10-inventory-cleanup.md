---
작업: inventory-cleanup
날짜: 2026-09-10
상태: 진행중
관련: tech-debt 「`Inventory`가 검증 없는 인덱싱으로 널 역참조에 열려 있다」,
  「인벤토리 매핑 3종이 손으로 유지된다」
---

# Inventory 매핑 표 통합과 슬롯 입력 검증

## 작업 배경

1. **클라가 보낸 값 하나로 게임 서버가 죽는다.** `Inventory`의 `removeItem`과 `GetSlot`은
   슬롯 타입을 `unordered_map::operator[]`로 조회한다. 표에 없는 키가 오면 `operator[]`가
   기본값을 삽입하므로 `ITEM_TYPE_NONE`이 나오고, 그 값으로 다시 조회한 포인터가 `nullptr`이다.
   바로 다음 줄의 `->Mutable(slotId)`가 널 역참조다. `slotId`에는 범위 검사가 없어
   `MAX_SLOTS`(32) 밖의 값도 그대로 들어간다. 둘 다 신뢰 경계 바깥에서 온 값이다.
2. **`Inventory`가 손으로 맞춰야 하는 표를 셋 들고 있다.** `itemTypeMappings`,
   `slotTypeToItemTypeMappings`, `inventorylookupMappings`를 전부 생성자에서 채운다.
   넣을 때(`addItem`)와 꺼낼 때(`removeItem`, `GetSlot`)가 서로 다른 표를 보므로, 한 표가
   어긋나면 아이템이 다른 인벤토리로 샌다. 세 값 모두 유효한 enum이라 컴파일러가 잡지 않는다.
3. **이 구조는 이미 한 번 터졌다.** `SLOT_TYPE_INVENTORY_MISC`가 `ITEM_TYPE_GEAR`로
   적혀 있어 기타 아이템을 지우면 장비 인벤토리를 건드린 사고였다. 값 하나를 고쳐 막았을 뿐
   같은 사고가 다시 나는 구조는 그대로다.

### 원하는 결과

**같은 실수를 구조상 할 수 없게 만든다.** 표가 하나로 줄어 `SlotType`과 `ItemType`의 대응이
한 곳에서 유도되고, 알 수 없는 `SlotType`과 범위 밖 `slotId`는 인덱싱에 닿기 전에 거부된다.
방어 코드를 덧대는 데서 멈추지 않는다 — 다음 사람이 표를 손으로 맞춰야 하는 상황 자체를 없앤다.

### 변경에 의한 영향 범위

- 게임 서버의 `Inventory` 클래스
- `InventoryTests`

### 반드시 지킬 것

- **기존 14개 테스트를 고쳐서 통과시키지 않는다.** 새 테스트 추가만 허용한다.
- **`.proto`를 건드리지 않는다.** 코드젠과 클라 재빌드가 없고, `NEXT.md` 2번과 겹치지 않는다.
- `Room`이 `JobQueue`를 상속하는 규약을 깨지 않는다. 인벤토리 상태는 룸 잡 큐 위에서 직렬화된다.
- 주석과 로그는 한국어로 쓴다.

### 미결정 사항

1. **반환 계약을 바꿀지.** `removeItem`과 `GetSlot`이 실패를 호출자에게 알릴지
   (`bool`·`std::optional`·`nullptr` 반환), 아니면 조용히 무시하고 로그만 남길지.
   바꾸면 호출자까지 열어야 하므로 「변경에 의한 영향 범위」가 넓어진다.
2. **표 셋을 어떤 형태로 합칠지.** `constexpr` 변환 함수인지 단일 테이블 상수인지.
   새 `SlotType`이 추가됐을 때 빌드가 깨지게 하는 컴파일 타임 검증을 어디에 걸지도 함께 정한다.
3. **거부된 요청을 어떻게 다룰지.** 프로토콜을 안 바꾸므로 클라에 실패 응답을 보낼 길이 없다.
   남는 것은 로그 레벨과 문구, 그리고 세션을 끊을지 여부다.
4. **테스트를 먼저 빨간색으로 둘지.** `tdd-slice` 스킬대로 실패 테스트를 먼저 커밋할지,
   수정과 테스트를 한 커밋에 넣을지.

## 결정

### 실패를 알리는 방법 — 기존 반환 타입을 그대로 쓴다

**채택**: `removeItem`은 `false`, `GetSlot`은 `nullptr`를 돌려준다. 새 오류 타입을 만들지 않는다.
**기각**: `SlotError` enum이나 `std::optional`로 실패 이유까지 돌려주는 안. 호출자 3곳과 기존
테스트를 함께 열어야 해서 「변경에 의한 영향 범위」가 `Player`까지 넓어진다. 지금 호출자는
실패 이유로 분기하지 않는다 — 셋 다 `false`면 그대로 `false`를 반환할 뿐이다.
**기각**: 조용히 무시하고 성공을 반환하는 안. 판매·사용이 아이템 없이 성공한 것처럼 처리된다.
**근거**: `removeItem`은 이미 `bool`이고 `Player.cpp:77`, `Player.cpp:92`, `Player.cpp:156`
세 호출자가 전부 반환값을 검사한다. 검증을 넣고 `false`를 반환하면 호출자 수정이 0이다.
`GetSlot`은 프로덕션 호출자가 없다 — `InventoryTests.cpp`만 부르므로 계약을 바꿔도 밖으로
번지지 않는다.
**틀렸다는 신호**: 호출자가 "알 수 없는 슬롯 타입"과 "빈 슬롯"을 구분해 다르게 응답해야 하는
요구가 생긴다. 그때는 `bool`로 부족하다.
**승격**: 없음

### 표 통합 — 단일 `constexpr` 카테고리 테이블

**채택**: `SlotType`·`ItemType`·아이템 데이터의 `itemType` 문자열을 한 배열(`CATEGORIES`)에
묶고, 조회 함수 셋을 전부 이 배열에서 유도한다. 런타임 표(슬롯 배열 포인터, 더티 플래그)는
`ItemType`을 인덱스로 쓰는 고정 배열로 바꾼다. 누락은 `static_assert`가 빌드에서 막는다.
**기각**: `constexpr switch` 변환 함수. `default` 레이블이 있으면 MSVC의 C4062가 뜨지 않고,
`default` 없이 두면 알 수 없는 값이 컴파일 경고 없이 빠져나간다. 문자열 → `ItemType` 대응은
`switch`로 쓸 수도 없다.
**기각**: 표 3종을 유지하고 생성자에서 `assert`로 교차 검증하는 안. 키 존재는 확인하지만
`SLOT_TYPE_INVENTORY_MISC`에 `ITEM_TYPE_GEAR`가 적힌 **값이 뒤바뀐 경우**를 못 잡는다.
이미 한 번 터진 사고가 정확히 그 형태다.
**근거**: 대응 관계가 한 곳에만 있으면 어긋날 자리가 없다. 방어 코드를 더하는 대신 손으로
맞추는 상황 자체를 없앤다는 「원하는 결과」와 맞는다.
**틀렸다는 신호**: 인벤토리 탭마다 조회 규칙이 갈라져 테이블 한 줄로 표현되지 않는다
(`findFirstAvailableSlotId`가 이미 `ITEM_TYPE_GEAR`만 다르게 다룬다). 갈라짐이 둘을 넘으면
테이블이 아니라 카테고리별 정책 객체가 맞다.
**승격**: 없음

### 거부된 요청 — `false` 반환과 한국어 경고 로그, 세션은 유지

**채택**: `Inventory`가 한국어 경고 로그를 남기고 `false`를 반환한다. 클라 응답은 핸들러가
이미 하던 `set_success(false)`가 그대로 처리한다. 프로토콜과 핸들러를 고치지 않는다.
**기각**: 세션 강제 종료. 조작된 패킷뿐 아니라 서버 버그나 클라 구버전도 같은 값을 만들 수
있어 정상 플레이어를 끊는다.
**기각**: 로그 없이 `false`만. 공격이든 버그든 서버 쪽에 흔적이 남지 않는다.
**근거**: 「미결정 사항」 3번이 깔고 있던 "프로토콜을 안 바꾸므로 클라에 실패 응답을 보낼 길이
없다"가 사실과 다르다. `S_SELL_ITEM`과 `S_USE_ITEM`에 `success` 필드가 이미 있고
`Room::C_HandleSellItem`(`Room.cpp:438`)이 실패 시 `set_success(false)`로 이미 답한다.
**틀렸다는 신호**: 같은 세션에서 거부 로그가 반복해 쌓인다. 정상 클라라면 나오지 않을 값이
계속 온다는 뜻이므로, 그때 차단 정책을 따로 정한다.
**승격**: 없음. 다른 핸들러에서 같은 판단이 또 필요해지면 서버 공통 규약으로 ADR에 올린다.

### 커밋 순서 — 실패 테스트를 먼저 커밋한다

**채택**: 죽는 것을 보여주는 테스트를 먼저 커밋해 빨강을 기록에 남기고, 다음 커밋에서 고친다.
**기각**: 수정과 테스트를 한 커밋에 넣는 안. 테스트가 실제로 빨강이었다는 증거가 남지 않는다.
**근거**: 「그물이 이미 있다」가 이 작업을 첫 대상으로 고른 이유다.
`DISABLED_StackDoesNotExceedMaxStack`도 같은 방식으로 빨강임을 먼저 확인한 전례가 있다.
**틀렸다는 신호**: 빨강 커밋의 테스트가 프로세스 크래시로 죽어 `GameServerTests.exe`가
나머지 테스트 결과까지 못 내놓는다. 그러면 빨강 커밋을 `DISABLED_` 접두어와 함께 넣는다.
**승격**: 없음

## 위험 요소

- **Phase C가 가장 위험하다.** A·B는 코드를 더하기만 하지만, C는 `addItem`,
  `findFirstAvailableSlotId`, 생성자, DB 저장 경로가 함께 도는 자료구조를 바꾼다.
- **기존 그물이 대칭 오류를 못 잡는다.** 왕복 테스트(넣기 → 조회 → 지우기)는 넣을 때와 꺼낼 때가
  **같이** 틀리면 초록으로 통과한다. 이미 한 번 터진 사고가 바로 그 형태였다. 그래서 Phase C에
  대응 방향을 직접 단언하는 테스트를 따로 넣는다.
- **빈 `dataKeys` 칸.** `ITEM_TYPE_GEAR`만 문자열이 둘이라 나머지 카테고리는 빈 칸이 남는다.
  조회에서 빈 문자열을 건너뛰지 않으면 `itemType` 없는 아이템이 엉뚱한 탭으로 분류된다.
- **`static_assert`가 `.proto`의 enum 번호에 기댄다.** `ItemType_ARRAYSIZE`를 쓰는 이유가
  이것이지만, 번호를 띄엄띄엄 매기면 여전히 어긋난다.
- **`GetDirtyFlags`에도 같은 구멍이 있다.** `dirtyFlagsMappings[itemType]`가 `operator[]`라
  `ITEM_TYPE_NONE`을 주면 빈 `vector`를 삽입해 반환하고, 호출자가 `[0]`을 읽으면 UB다.
  호출자 3곳이 전부 `ITEM_TYPE_GEAR` 리터럴이라 실피해는 없다. 배열로 바꾸면 함께 닫힌다.
- **확인한 것** — `player->possession`은 `Player` 생성자에서 한 번 만들어지고 교체되지 않는다
  (`Player.cpp:14`). `mutable_inventory()`를 부르는 곳은 `Inventory` 생성자뿐이다
  (`Inventory.cpp:8`). 캐시한 슬롯 배열 포인터가 낡을 경로가 없다.

### 고려했지만 하지 않는 것

- **`EquippedGear`까지 정리.** `SLOT_TYPE_EQUIPPED`가 같은 enum에 있어 묶고 싶지만
  「변경에 의한 영향 범위」 밖이다. tech-debt로 넘긴다.
- **`addItem`의 미검증 `templateId`.** `Gamedata::ItemDataTable[id]`가 `operator[]`라 없는 키를
  삽입한다. `addItem` 맨 위에 주석 처리된 `template_id() == 0` 검사가 그 흔적이다.
  같은 종류의 버그지만 이번 영향 범위에 없다. tech-debt로 넘긴다.
- **범위 검사를 `Room` 핸들러로 올리기.** 핸들러 5곳에 같은 코드가 복제된다.
  `Inventory`가 자기 불변식을 지키는 쪽이 맞다.

## 증명

빌드는 `build_solution_start(rootFolder="D:\Unreal\Projects\Project_MMORPG\Server",
filesToRebuild=["Inventory.cpp", "InventoryTests.cpp"])` → `build_solution_state` 폴링으로 한다.
`rebuild=true`를 쓰지 않고 UBT·MSBuild를 터미널로 돌리지 않는다. 솔루션 전체 빌드는
`AuthServer.esproj` 때문에 항상 빨강이므로 범위를 좁힌다.

실행은 셸에서 한다. `Server/Binary/`는 gitignore이므로 실행 전 빌드가 필수다.

| Phase | 검사 | 통과 기준 |
| --- | --- | --- |
| A | `Server/Binary/Debug/GameServerTests.exe` | 종료 코드 0, 14개 유지 (`DISABLED_`는 안 돈다) |
| A | `GameServerTests.exe --gtest_also_run_disabled_tests --gtest_filter=*Rejected*:*ReturnsNull*` | 크래시 또는 실패. **여기서 초록이면 테스트가 구멍을 못 짚은 것이다** |
| B | `GameServerTests.exe` | 종료 코드 0, 18개 (14 + 4) |
| C | `GameServerTests.exe` | 종료 코드 0, 19개 (18 + 대응표 1) |
| C | `CATEGORIES`에서 한 줄을 주석 처리하고 빌드 | `static_assert` 실패로 빌드가 멈춘다. 확인 후 되돌린다 |

전 Phase 공통: `git diff`에 기존 `TEST_F`·`TEST_P` 본문 변경이 있으면 「반드시 지킬 것」 위반이다.

## Phase A — 지금 죽는 것을 테스트로 고정한다 (빨강)

`DISABLED_` 접두어를 붙인다. 널 역참조가 gtest 프로세스를 통째로 죽여 나머지 13개 결과까지
못 내놓기 때문이다. `DISABLED_StackDoesNotExceedMaxStack`과 같은 처리다.

- [ ] **A-1 `DISABLED_RemoveWithUnknownSlotTypeIsRejected`** — `SLOT_TYPE_EQUIPPED`로
  `removeItem`을 부르면 `false` — `Server/GameServerTests/InventoryTests.cpp`
- [ ] **A-2 `DISABLED_RemoveWithOutOfRangeSlotIdIsRejected`** — `slot_id`에 `MAX_SLOTS`와 `-1` —
  `Server/GameServerTests/InventoryTests.cpp`
- [ ] **A-3 `DISABLED_GetSlotWithUnknownSlotTypeReturnsNull`** — `SLOT_TYPE_QUICK`으로 `GetSlot` —
  `Server/GameServerTests/InventoryTests.cpp`
- [ ] **A-4 `DISABLED_GetSlotWithOutOfRangeSlotIdReturnsNull`** — `slot_id`에 `MAX_SLOTS` —
  `Server/GameServerTests/InventoryTests.cpp`
- [ ] **A-5 빨강 확인 후 커밋** — `🚧 wip: 잘못된 슬롯 입력이 Inventory를 죽이는 테스트 추가 (빨강)`

## Phase B — 슬롯 입력 검증 (초록)

- [ ] **B-1 판정 두 개를 선언한다** — `ToItemType(Protocol::SlotType)`은
  `optional<Protocol::ItemType>`, `IsValidSlotId(int32)`는 `0 <= slotId && slotId < MAX_SLOTS` —
  `Server/GameServer/Game/System/Inventory.h`
- [ ] **B-2 `removeItem`이 인덱싱 전에 거른다** — 실패 시 한국어 경고를 `cout`으로 남기고
  `false`. 로그 문투는 `findFirstAvailableSlotId`를 따른다 —
  `Server/GameServer/Game/System/Inventory.cpp`
- [ ] **B-3 `GetSlot`이 인덱싱 전에 거른다** — 실패 시 `nullptr`. `at(slot_id)`를
  `Mutable(slot_id)`로 바꾼다 — `at()`은 범위를 벗어나면 `CHECK` 실패로 프로세스를 죽인다 —
  `Server/GameServer/Game/System/Inventory.cpp`
- [ ] **B-4 `DISABLED_` 접두어 4개를 뗀다** — `Server/GameServerTests/InventoryTests.cpp`
- [ ] **B-5 초록 확인 후 커밋** — `✅ done: Inventory 슬롯 입력 검증 추가 (초록)`

호출자는 고치지 않는다. `Player.cpp:77`, `Player.cpp:92`, `Player.cpp:156`이 이미 반환값을
검사한다.

## Phase C — 표 3종을 단일 카테고리 테이블로 합친다

- [ ] **C-1 `InventoryCategory` 테이블과 `static_assert`** — `SlotType`·`ItemType`·아이템 데이터
  문자열을 한 배열에 묶는다. `static_assert(std::size(CATEGORIES) == Protocol::ItemType_ARRAYSIZE - 1)`
  — `ITEM_TYPE_MISCELLANEOUS`가 마지막이라는 가정을 코드에 박지 않는다 —
  `Server/GameServer/Game/System/Inventory.h`
- [ ] **C-2 `ToItemType` 둘을 테이블에서 유도한다** — `SlotType` 판을 테이블 순회로 바꾸고
  `string_view` 판을 새로 만든다. 빈 `dataKeys` 칸을 건너뛴다 —
  `Server/GameServer/Game/System/Inventory.h`
- [ ] **C-3 런타임 표 둘을 고정 배열로 바꾼다** — `inventorylookupMappings`와 `dirtyFlagsMappings`를
  `ItemType` 인덱스 배열(`ItemType_ARRAYSIZE` 크기, 0번은 빈 칸)로. `GetDirtyFlags`의
  `vector<bool>&` 반환 계약은 유지해 `DBRequestFunctions.cpp:1141`·`:1308`·`:1427`을 건드리지
  않는다 — `Server/GameServer/Game/System/Inventory.{h,cpp}`
- [ ] **C-4 생성자의 표 초기화 3종을 지운다** — `Server/GameServer/Game/System/Inventory.cpp`
- [ ] **C-5 `addItem`의 문자열 조회를 바꾼다** — `nlohmann::json`의 암묵 변환 대신
  `itemData[JsonProperty::Item::ItemType].get_ref<const std::string&>()`로 꺼내 `string_view`
  비교에 넘긴다 — `Server/GameServer/Game/System/Inventory.cpp`
- [ ] **C-6 `SlotTypeMapsToItemTypeAsProtoDefines`** — `ToItemType`의 세 대응을 직접 단언하고
  `SLOT_TYPE_NONE`·`SLOT_TYPE_EQUIPPED`·`SLOT_TYPE_QUICK`이 `nullopt`인지 본다.
  **왕복 테스트로는 대칭 오류를 못 잡으므로 이 항목이 Phase C의 핵심이다** —
  `Server/GameServerTests/InventoryTests.cpp`
- [ ] **C-7 `static_assert`가 실제로 무는지 손으로 1회 확인** — `CATEGORIES` 한 줄을 주석 처리해
  빌드가 멈추는지 보고 되돌린다
- [ ] **C-8 초록 확인 후 커밋** — `♻️ refactor: 인벤토리 매핑 표 3종을 단일 카테고리 테이블로 통합`

## 작업 중 기록

<!-- 계획에서 벗어난 이유, 막힌 지점, tech-debt로 보낸 것.
     TDD 슬라이스의 실패 로그도 이 절에 적는다. -->

- 2026-09-10: 1단계 마감. 「미결정 사항」 4개를 2단계에서 결정한다.
- 2026-09-12: 2단계 마감. 「미결정 사항」 4개를 모두 결정했다.
- 2026-09-12: 「미결정 사항」 3번의 전제가 틀렸다. 프로토콜을 안 바꿔도 클라에 실패를 알릴 수
  있다 — `S_SELL_ITEM`과 `S_USE_ITEM`에 `success` 필드가 이미 있고 `Room.cpp:438`이 쓰고 있다.
- 2026-09-12: 「미결정 사항」 4번이 부른 `tdd-slice` 스킬은 이 저장소에 없다.
  `.claude/skills/`에는 `end-session`, `grill-me`, `grilling`, `korean-md-style`, `new-adr`,
  `new-work`만 있다. 커밋 순서는 스킬 없이 손으로 지킨다.
- 2026-09-12: `SlotType`에 `SLOT_TYPE_EQUIPPED`(4)와 `SLOT_TYPE_QUICK`(5)이 있는데
  `slotTypeToItemTypeMappings`에는 없다. 널 역참조가 가설이 아니라 실재하는 입력 경로다.
- 2026-09-12: Rider의 `analyze_calls`가 이 C++ 솔루션에서 심볼을 못 찾는다
  (`No callable symbol found for symbolFqn`). 호출자 확인은 `search_text`로 대신했다.

<!--
완료 항목 옆에 요약 한 줄을 적는다 (파일 경로와 핵심 결정).
계획에 없던 항목에는 *(계획에 없던 항목)*을 표기한다.
막힌 항목은 지우지 않는다. 형식: `[B] blocked` 무엇이 어디서 막혔는가 (에러 원문과 근거 `file:line`).
판정이 틀렸던 것도 지우지 않는다.

「다음에 할 것」을 이 절에 적지 않는다. `docs/NEXT.md` 하나만 미래를 말한다.
-->
