---
작업: design-data-pipeline · 시작: 2026-10-27
브랜치: 통합 브랜치 `refactor/312-design-data-pipeline` · 맵 이슈: #312
---

# 기획 데이터를 DataBuilder로 생성하고 표 구성 다시 짜기

## 목표

- 엑셀 원본을 고친 뒤 DataBuilder 명령 하나로 검증과 생성이 끝나고, 서버와 클라이언트가 생성된 타입으로 기획 데이터를 읽는다
- 생성된 JSON이 한 행 한 줄로 정렬되어, 원본을 고친 결과가 diff 한 줄로 드러난다
- 표 구성이 `CONTEXT.md`의 용어와 맞는다. 맵과 지역이 나뉘고, 직업 공통 값과 직업별 값이 나뉘고, 기획 데이터가 아닌 값은 서버 상수로 돌아간다
- 에디터 검증기가 기획 데이터 테이블과 표현 데이터 테이블의 짝을 확인한다

결정과 근거는 `docs/adr/0022-generate-design-data-schema-with-databuilder.md`와 #312 본문에 있다.

## 세션 시작 절차

1. 통합 브랜치를 체크아웃한다. 없으면 `dev`에서 딴다. `dev`에 새 커밋이 있으면 merge한다(rebase하지 않는다)
2. 진행 표에서 차단이 모두 「머지」인 첫 「대기」 티켓을 고르고, 그 이슈 본문을 읽는다. 「진행」인 티켓이 있으면 「기록」에서 어디까지 했는지 먼저 읽는다
3. 티켓 브랜치를 통합 브랜치에서 따고 PR은 통합 브랜치로 보낸다. 그 PR에 진행 표의 해당 줄을 「머지」로 고치는 변경을 함께 넣는다.
   머지되면 `close-merged-issues` 워크플로가 PR 본문의 `Closes #N` 이슈를 닫는다. 닫히지 않았으면 손으로 닫는다

## 지킬 것

- 티켓 PR을 머지한 시점에 서버가 뜨고 게임이 동작한다. 서버를 고친 티켓은 서버 솔루션 빌드(DummyClient 포함)와 `GameServerTests`가 통과하고, DAO나 입장 경로를 건드린 티켓은 실제 DB 테스트도 통과한다. 클라이언트를 고친 티켓은 클라이언트 빌드와 `Run-UeTests.ps1`이 통과한다. DataBuilder를 고친 티켓은 DataBuilder 테스트가 통과한다
- 표는 확장, 이전, 축소 순서로 옮긴다. 옮기지 않은 표는 옛 변환기와 옛 파서로 계속 동작한다. 옛 경로는 #322가 없앤다
- 표를 옮긴 티켓은 그 표의 `Original_` 워크북과 옛 서버 JSON을 지운다. 옛 클라이언트 JSON과 데이터 테이블은 #320까지 그 시점의 값으로 남는다. 그 사이에 아이템, 몬스터, 맵 수치를 바꾸면 클라이언트에는 반영되지 않는다
- 워크북은 Claude가 `openpyxl`로 만들고 고친다(`docs/codegen.md`). 새 시트 규칙의 워크북에는 옛 워크북의 값을 그대로 옮기고, 옮긴 값은 DataBuilder 산출물과 옛 JSON을 대조해 확인한다
- 기획 데이터 자체의 규칙은 DataBuilder에, 코드가 정한 제약은 서버 기동에 둔다(ADR-0022). 같은 규칙을 두 곳에 쓰지 않는다
- 산출물 폴더에는 사람이 쓴 파일을 두지 않는다
- 로비와 게임은 서로의 기획 데이터를 읽지 않는다(ADR-0021)
- 스키마 변경은 예상하지 않는다. 생기면 SQL 초안을 사람에게 넘긴다
- 에디터 작업(데이터 테이블 생성과 삭제, 메뉴 실행)은 Claude가 그 직전까지 진행하고 절차를 사람에게 인계한다. 언리얼 MCP로 에디터를 바꾸기 전에는 커밋 상태를 확인한다
- 옮기다가 새로 찾은 결함은 그 자리에서 고치지 않는다. 「기록」에 적고 `docs/tech-debt.md`에도 적는다
- 진행 표의 「검증」에는 실제로 돌린 것만 적는다
- 통합 브랜치를 `dev`에 넣을 때는 merge commit으로 한다

## 진행

상태: 대기 · 진행 · 머지 · 막힘

| # | 티켓 | 차단 | 상태 | PR | 검증 |
|---|---|---|---|---|---|
| 1 | #313 DataBuilder 뼈대를 만들고 재화 표를 새 경로로 서버에 싣기 | — | 머지 | #323 | DataBuilder 테스트 67개 통과 · 서버 솔루션 빌드 0(DummyClient 포함) · `GameServerTests` 491개 통과 · 서버 기동 로그에 데이터 버전 · 규범 검사와 문서 검사 통과 |
| 2 | #314 아이템 표를 공통, 장비 능력치, 소모 효과 표로 나눠 옮기기 | 1 | 머지 | #324 | DataBuilder 테스트 79개 통과 · 서버 솔루션 빌드 0(DummyClient 포함) · `GameServerTests` 490개 통과 · 실제 DB 테스트 25개 통과 · 서버 기동 로그에 데이터 버전 · 새 산출물과 옛 `S_Item.json` 39행 대조 |
| 3 | #315 몬스터 표를 기본 보상 열로 옮기고 드롭 그룹과 드롭 항목 표 더하기 | 2 | 머지 | #325 | DataBuilder 테스트 107개 통과 · 서버 솔루션 빌드 0(DummyClient 포함) · `GameServerTests` 491개 통과 · 서버 기동 로그에 데이터 버전 · 새 산출물과 옛 `S_Monster.json` 9행 대조 |
| 4 | #316 맵 표를 맵, 지역, 포털 표로 나누기 | 3 | 머지 | #327 | DataBuilder 테스트 132개 통과 · 서버 솔루션 빌드 0(DummyClient 포함) · `GameServerTests` 491개 통과 · 실제 DB 테스트 25개 통과 · 서버 기동 로그에 데이터 버전 · 새 산출물과 옛 `S_Map.json` 4행 대조. PIE는 확인하지 않았다 |
| 5 | #317 설정 시트로 초기 진행과 로비 데이터를 옮기고 칸 수와 퀵슬롯 개수를 서버 상수로 되돌리기 | 4 | 머지 | #329 | DataBuilder 테스트 144개 통과 · 서버 솔루션 빌드 0(DummyClient 포함) · `GameServerTests` 484개 통과 · 실제 DB 테스트 25개 통과 · 서버 기동 로그에 데이터 버전 · 새 산출물과 옛 `S_Initial_Progress.json`, `S_Lobby.json` 대조 · 규범 검사와 문서 검사 통과. PIE는 확인하지 않았다 |
| 6 | #318 레벨 표를 공통 레벨 표와 직업별 능력치 표로 나누기 | 1 | 머지 | #326 | #315를 merge한 뒤 DataBuilder 테스트 117개 통과 · 서버 솔루션 빌드 0(DummyClient 포함. merge 전 첫 빌드는 TD-077로 1) · `GameServerTests` 493개 통과 · 실제 DB 테스트 25개 통과 · 서버 기동 로그에 데이터 버전 · 새 산출물과 옛 `S_Warrior_Level_Data.json` 50행 대조 |
| 7 | #319 클라이언트 행 구조체를 생성하고 새 데이터 테이블 채우기 | 2, 3, 4 | 머지 | #328 | #317을 merge한 뒤 DataBuilder 테스트 161개 통과 · 클라이언트 빌드 0 · `Run-UeTests.ps1` 19개 통과 · 규범 검사와 문서 검사 통과. 에디터 메뉴는 실행하지 않았다(사람의 확인) |
| 8 | #320 클라이언트 코드를 새 데이터 테이블로 옮기고 손으로 쓴 행 구조체 지우기 | 7 | 머지 | #330 | 클라이언트 빌드 0 · `Run-UeTests.ps1` 20개 통과 · 규범 검사와 문서 검사 통과 · 사람이 에디터 메뉴로 새 테이블 6개를 채움(임포트 문제 0건) · 옛 테이블 셋 삭제. PIE는 확인하지 않았다 |
| 9 | #321 P1Editor 모듈과 수치 테이블과 에셋 테이블의 짝 검증기 만들기 | 8 | 대기 | | |
| 10 | #322 옛 변환기, 퀘스트 표, 시험용 파일 걷어내기 | 5, 6, 8 | 대기 | | |

2 → 3 → 4 → 5의 순서는 참조 방향 때문이다. 드롭 항목은 아이템을, 지역은 몬스터를, 시작 맵은 맵을 참조하므로 참조되는 표가
먼저 DataBuilder에 있어야 한다. 6번은 다른 표를 참조하지 않으므로 1번 뒤에 언제든 할 수 있다.

## PIE 시나리오

사람이 확인하는 항목이다. 티켓마다 그 티켓까지의 줄을 확인한다.

| 줄 | 시작하는 티켓 | 확인할 것 |
|---|---|---|
| 1 | #316 | 포털 이동, 사망과 마을 리스폰, 다시 접속한 뒤 마지막 위치 |
| 2 | #317 | 새 캐릭터가 시작 맵의 시작 위치에 시작 골드를 들고 선다. 인벤토리 구역과 퀵슬롯이 지금처럼 보인다 |
| 3 | #319 | 에디터 메뉴로 새 데이터 테이블이 만들어지고 값이 엑셀과 같다. 원본에서 지운 행이 테이블에서도 사라진다 |
| 4 | #320 | 인벤토리의 아이콘과 이름, 상점 목록, 몬스터 스폰, 포털 이동 |
| 5 | #321 | 에셋 테이블의 행 하나를 비우고 저장하면 오류가 보인다 |

## 기록

(티켓을 진행하면서 어디까지 했는지, 막힌 것과 새로 찾은 결함을 적는다)

**#313 (2026년 10월 27일)**

- DataBuilder의 공통 규칙을 모두 만들었다. 시트 규칙, 타입, 열거형 표기, 세로형 설정 시트, 산출물 형식, 데이터 버전은
  `docs/codegen.md` 「기획 데이터 (DataBuilder)」에 있다. 뒤 티켓은 `databuilder.toml`에 표를 더하고 워크북을 옮긴다
- 서버 산출물은 모듈마다 헤더 하나(`DesignData.h`)다. 표를 더해도 `.vcxproj`에는 그 표의 JSON만 더 등록한다
- 표 하나의 의미 규칙(최솟값과 최댓값의 순서, 드롭 그룹의 판정 방식과 항목 열의 짝)은 `databuilder/rules.py`의 `TABLE_RULES`에 넣는다.
  재화 표의 「GOLD 행이 정확히 하나」를 여기로 옮겼다. 서버는 GOLD 행이 없을 때만 부팅을 멈춘다(재화 번호를 정할 수 없어서)
- 정한 것: 열 이름은 PascalCase이고 생성되는 필드 이름이 된다. `none` 열은 빈 셀을 허용한다. 정수 타입은 `int32`와 `int64` 둘이다.
  열거형은 산출물 JSON에 번호가 아니라 이름으로 싣는다
- 클라이언트 산출물은 만들지 않았다. `client` 대상 열도 검사는 한다. 클라이언트 생성은 #319가 정한다
- 새로 찾은 결함: TD-077 병렬 솔루션 빌드에서 DummyClient의 프로토콜 생성 단계가 한 번 실패했다
- DataBuilder 테스트는 CI에 붙이지 않았다

**#314 (2026년 10월 27일)**

- 아이템을 `Item`, `ItemEquipStat`, `ItemConsumeEffect` 세 워크북으로 나눴다. 종류별 표의 키는 `key:parent:Item`이다.
  표 구성과 DataBuilder 규칙은 `docs/codegen.md` 「아이템 표」에 있다
- 옛 값을 옮기고 새 산출물을 옛 `S_Item.json`과 대조했다. 39행의 서버 값이 모두 같다. 이름과 설명은 옛 워크북과 새 워크북끼리 대조했다
- 정한 것: 착용 부위는 옛 `item_subtype`(helmet, sword 등) 대신 `ItemEquipStat.Part`(`enum:GearPart`)다. 착용 직업은 `enum[]:CharacterClass`이고
  빈 셀이 옛 `all`이다. 재사용 대기는 초 대신 밀리초 정수(`CooldownMs`)이고 소모 효과 표로 옮겼다. 장비의 대기는 옛 표에서도 -1(없음)이었다.
  대기가 없으면 0을 적고, 음수는 DataBuilder가 막는다. 서버가 대기를 부호 없는 시각에 더하므로 음수는 끝나지 않는 대기가 된다
- 옮기지 않은 열: `stackable`(`MaxStack`으로 정해진다), `hp_regenerate`와 `mp_regenerate`(39행 모두 0이고 서버도 클라이언트도 읽지 않는다),
  소모품의 `item_subtype`(`potion`). 클라이언트의 `FP1ItemData`에는 이 필드가 남아 있다. #319와 #320이 클라이언트 열을 정할 때 다시 본다
- 회복 비율이 옛 `double`에서 `float`가 됐다. DataBuilder의 실수 타입이 `float` 하나라서다. 지금 값(0.3, 0.4)은 최대치 1000 기준으로 결과가 같다.
  0.7처럼 `float`로 조금 작게 저장되는 값은 최대치를 곱한 뒤 버림하면 1이 모자랄 수 있다
- 표 이름으로 규칙을 거는 `TABLE_RULES` 때문에, 일반 규칙 테스트가 표 이름으로 쓰던 `Item`을 `Thing`으로 바꿨다
- 서버 테스트는 `Server/GameServerTests/TestItemTables.h`로 아이템 행을 넣는다. 생성된 행의 `MaxStack` 기본값이 0이라 손으로 채우면 아이템이 인벤토리에 들어가지 않는다
- `GenJsonFile.bat`에서 아이템 줄을 지웠다. `C_Item.json`과 `DT_Item`은 #320까지 지금 값으로 남는다

**#315 (2026년 10월 27일)**

- 몬스터를 `Monster`, `DropGroup`, `DropEntry` 세 워크북으로 옮겼다. 표 구성과 규칙은 `docs/codegen.md` 「몬스터와 드롭 표」에 있다
- 옛 값을 옮기고 새 산출물을 옛 `S_Monster.json`과 대조했다. 9행의 서버 값이 모두 같다
- DataBuilder에 세 가지를 더했다. 선택 수 타입(`int32?`, `int64?`, `float?`), 자식 표를 부모 행 아래에 중첩하는 설정(`nest`), 산출물을 막지 않는 경고(`TABLE_WARNINGS`)다.
  중첩한 자식 행도 한 줄씩 써서 diff가 한 줄로 남는다
- 정한 것: 드롭 그룹 키는 다른 표처럼 `int32`다. 드롭 항목의 키는 그룹과 아이템의 조합이라 한 그룹에 같은 아이템을 두 번 둘 수 없다.
  판정 방식은 설정 파일의 열거형 `DropMethod`(`INDEPENDENT`, `WEIGHTED`), 공격 방식은 `MonsterAttackType`(`MELEE`, `RANGED`)이다
- 이슈에 없던 규칙을 더했다: 기본 보상 최솟값 0 이상, 항목 `Rate` 0 초과 1 이하, `Weight` 1 이상, `MinCount` 1 이상
- 드롭 데이터는 옛 표에 없었다. 경로가 실제 산출물로 돌도록 견본 그룹 둘을 넣었다(초급 미니언의 물약, 초보자 장비). 판정과 지급이 없으므로 게임에는 영향이 없다
- 옮기지 않은 열: `is_targeting`(서버가 파싱만 하고 읽지 않았다)
- `GenJsonFile.bat`에서 몬스터 줄을 지웠다. `C_Monster.json`과 클라이언트 몬스터 데이터 테이블은 #320까지 지금 값으로 남는다

**#316 (2026년 10월 27일)**

- 옛 맵 워크북을 `Map`, `Area`, `Portal` 세 워크북으로 나눴다. 포털은 `Area.json`의 지역 아래 `Portals`에 중첩한다.
  표 구성과 규칙은 `docs/codegen.md` 「맵, 지역, 포털 표」에 있다
- 옛 값을 옮기고 새 산출물을 옛 `S_Map.json`과 대조했다. 지역 4행과 포털 6개의 서버 값이 모두 같다. 포털 번호는 클라이언트가 보내는 값이라 그대로 옮겼다
- 서버의 이름을 바로잡았다. `MapTemplate`과 `PortalTemplate`을 지우고 생성된 `AreaRow`와 `PortalRow`를 쓴다.
  `FindMap`, `GetMaps`, `townRoomId`가 `FindArea`, `GetAreas`, `townAreaId`가 됐다. 지역 번호를 룸 번호로 쓰는 규칙은 `RoomTransfer::SelectRoomForArea` 한 곳에 모았다.
  플레이어의 마지막 위치와 시작 맵은 맵 번호를 그대로 쓴다
- 정한 것: 포털 반경은 옛 표에서 지역마다 하나였지만 포털마다의 `Radius`가 됐다(이슈의 포털 열 구성). 지금 값은 모두 500이다.
  옛 포털 유형 `room`은 `AREA`로 바꿨다. 기획 데이터는 룸을 모르기 때문이다. 옛 표에 없던 맵 1111의 이름은 「시작의 섬」으로 정했다.
  몬스터 리스폰 시간은 단위를 열 이름에 넣어 `MonsterRespawnSec`로 했다
- 이슈에 없던 규칙을 더했다: 리스폰 지점 유무와 리스폰 좌표의 짝, `MaxMonsterCount`와 `MonsterRespawnSec` 0 이상
- 서버의 맵 검사 가운데 포털 목적지, 스폰 몬스터, 반경, 마을 수는 DataBuilder로 옮기고 서버에서 지웠다. 서버는 마을이 없을 때와 시작 맵이 맵 표에 없을 때만 부팅을 멈춘다.
  시작 맵 대조는 초기 진행 표를 옮기는 #317이 참조 열로 바꾼다
- `GenJsonFile.bat`에서 맵 줄을 지웠다. `C_Map.json`과 클라이언트 맵 데이터 테이블은 #320까지 지금 값으로 남는다
- 사람의 PIE 확인(포털 이동, 사망과 마을 리스폰, 다시 접속한 뒤 마지막 위치)이 남았다

**#318 (2026년 10월 27일)**

- 전사 레벨 표를 `Level`(레벨, 필요 경험치)과 `ClassLevelStat`(직업, 레벨, 능력치 넷)으로 나눴다. 표 구성과 규칙은 `docs/codegen.md` 「레벨 표」에 있다.
  #315와 병렬로 진행했다. 두 티켓 모두 `databuilder.toml`, `rules.py`, `Templates.h`, `GamedataParser.cpp`, `Player.cpp`의 보상 처리, 이 파일을 고친다
- 옛 값을 옮기고 새 산출물을 옛 `S_Warrior_Level_Data.json`과 대조했다. 50행의 값이 모두 같다. 옛 증가분 열은 2~50레벨에서 인접한 두 행의 차이와 같았다.
  그래서 레벨업 결과는 바뀌지 않는다
- 정한 것: 게임 코드는 `ClassLevelTable`과 `LevelTemplate` 대신 생성된 두 표를 쓴다. 레벨 판정(`Leveling`)은 직업과 무관한 레벨 표를 받는다.
  「레벨 표가 없는 직업」은 「직업별 능력치 표에 없는 직업」(`Gamedata::HasClassLevelStats`)이 됐다. 그런 직업은 경험치를 쌓지 않고 최대 레벨로 본다
- 레벨 표의 「1부터 이어짐」 검사를 서버 파서에서 DataBuilder로 옮겼다. 최대 레벨이 아닌 레벨의 필요 경험치가 1 이상인지도 DataBuilder가 검사한다.
  명세에 없던 규칙이지만 같은 표의 의미 규칙이라 함께 넣었다. 이 규칙으로 TD-061이 풀려 `docs/tech-debt.md`에서 지웠다
- 레벨업은 새 레벨과 이전 레벨 능력치 행의 차이를 지금 스탯에 더한다. 장비 증감량을 지키려고 새 레벨 값으로 덮지 않는다
- 서버 테스트는 `Server/GameServerTests/TestLevelTables.h`로 레벨 행과 능력치 행을 함께 넣는다
- `.gitignore`의 `C_TestJsonfile.json` 주석이 지운 `Original_Warrior_Level_Data.xlsx`를 가리킨다. 옛 변환기와 함께 #322가 정리한다

**#317 (2026년 10월 27일)**

- 설정 워크북 `Config.xlsx`에 `Progress`(시작 맵, 시작 좌표 셋, 시작 방향, 시작 골드)와 `Lobby`(기본 슬롯 수, 생성할 수 있는 직업) 시트를 만들고 옛 값을 그대로 옮겼다.
  새 산출물을 옛 `S_Initial_Progress.json`, `S_Lobby.json`과 대조했다(맵 1111, 좌표 0, 방향 0, 골드 10000, 슬롯 4, 전사). 규칙은 `docs/codegen.md` 「설정 워크북」에 있다
- 로비 모듈(`lobby`)을 설정 파일에 더했다. 산출물 폴더는 `Server/GameServer/Lobby/Data/Generated/`다
- 정한 것: 설정 파일의 모듈에 `namespace`를 더했다. 게임은 `DesignData`, 로비는 `LobbyDesignData`다. 두 모듈이 같은 네임스페이스를 쓰면 DataBuilder가 멈춘다.
  pch가 게임 헤더를, `Core/DataCrossCheck`가 로비 헤더를 함께 include해서, 네임스페이스가 같으면 공통 함수와 열거형 이름 표가 두 번 정의된다
- DataBuilder에 설정의 의미 규칙(`SETTING_RULES`)을 더했다. 시작 골드 0 이상, 슬롯 수 1 이상, 직업 목록이 비지 않고 겹치지 않음이다.
  서버에는 슬롯 수의 `TINYINT` 상한과 직업별 능력치 대조만 남겼다. 시작 맵의 서버 대조(`CheckStartMap`)는 참조 열(`ref:Map`)로 바꾸고 지웠다
- 인벤토리 구역의 칸 수는 `InventorySections::SLOT_COUNT`(32), 퀵슬롯 개수는 `QUICK_SLOT_COUNT`(9)가 됐다. 퀵슬롯 상수 옆에
  「9를 넘기면 `WBP_QuickSlots`에도 위젯을 더한다」를 남겼다. 256을 넘기면 `static_assert`가 빌드를 막는다
- 옛 변환기의 표 넷이 빠지면서 `GamedataParser.cpp`의 행 단위 도우미와 `JsonProperty.h`를 쓰는 곳이 없어져 함께 지웠다. 옛 변환기는 퀘스트 표 하나만 만든다
- 구역마다 칸 수가 다른 경우를 보던 테스트(`GameEntryTest`의 「다른 구역에서는 같은 번호가 들어감」)는 칸 수가 하나가 되어 지웠다.
  퀵슬롯 테스트는 상수 개수 기준으로 기대값을 만든다
- 일반 규칙 테스트가 설정 이름으로 쓰던 `Lobby`를 `Account`로 바꿨다. 설정 이름으로 규칙을 거는 `SETTING_RULES` 때문이다
- 사람의 PIE 확인(새 캐릭터가 시작 맵의 시작 위치에 시작 골드를 들고 선다. 인벤토리와 퀵슬롯이 지금처럼 보인다)이 남았다

**#319 (2026년 10월 27일)**

- DataBuilder가 클라이언트 산출물을 만든다. 설정 파일의 `[client]`가 JSON 폴더(`P1/DesignData/Generated/`)와 헤더 폴더(`P1/Source/P1/Game/Data/Generated/`)를 정한다.
  규칙과 파일 목록은 `docs/codegen.md` 「클라이언트 산출물」에 있다
- 클라이언트 산출물을 만드는 표는 `Item`, `ItemEquipStat`, `ItemConsumeEffect`, `Monster`, `Map`, `Area`(포털 중첩)다. `Level`은 키만 `both`라서 만들지 않는다
- 정한 것: 행 이름은 JSON 각 행의 첫 필드 `RowName`이다. 아이템 표의 `Name` 열이 언리얼 임포터의 기본 행 이름 필드(`Name`)와 겹쳐서, 에디터 스크립트가 `ImportKeyField`를 `RowName`으로 둔다.
  복합 키는 `_`로 잇는다
- 정한 것: `.proto` 열거형은 `UPROPERTY`가 될 수 없어서 값 이름(`FString`)으로 싣고, 행 구조체의 `Get<열>()`이 프로토콜 값을 돌려준다(ADR-0022의 「프로토콜 헤더의 열거형을 쓴다」).
  설정 파일의 열거형은 `UENUM`(`EP1<열거형>`)으로 만든다. #320은 옛 `FP1ItemData::GetItemType` 대신 `FP1ItemRow::GetType`을 쓴다
- 정한 것: 새 데이터 테이블은 `/Game/P1/Data/DesignData/DT_<표>`다. 옛 `DataTables/`의 같은 이름 테이블과 폴더로 나뉜다. 행 구조체의 속성은 `VisibleAnywhere`라 에디터에서 행 값을 고칠 수 없다
- 에디터 메뉴는 `P1/Scripts/Editor/p1_design_data_tables.py`다. `Content/`는 `Content/P1/`만 추적하므로 스크립트를 `Content/Python/`에 두지 않고, `DefaultEngine.ini`의 Python 시작 스크립트가 `sys.path`에 더해 불러온다.
  `PythonScriptPlugin`은 `ToolsetRegistry`가 이미 켜고 있었지만 `.uproject`에 에디터 전용으로 명시했다
- 데이터 버전은 생성된 `P1DesignDataVersion.h`의 값을 게임 인스턴스가 초기화할 때 남긴다(`기획 데이터 버전 ...`). 빌드에 들어간 헤더의 값이라, 메뉴를 돌리지 않으면 데이터 테이블과 어긋날 수 있다. #321의 검증기가 다룰지 본다
- 이슈에 없던 규칙을 더했다: 클라이언트로 가는 선택 값, 설정 시트의 클라이언트 필드, 부모 표가 클라이언트로 가지 않는 중첩, 클라이언트로 가는 설정 열거형의 `NONE` 값 이름과 255를 넘는 번호를 DataBuilder가 막는다
- `Tools/ConventionLint`가 `P1/Source/P1/Game/Data/Generated/` 아래 파일을 생성물로 보고 검사하지 않는다
- 사람의 확인이 남았다: 에디터 메뉴로 새 테이블 생성, 행 이름과 값, 원본에서 행을 지운 뒤 다시 돌렸을 때 테이블에서 사라지는지(PIE 시나리오 3줄). 메뉴는 실행하지 않았다
- 이미 있던 결함 TD-075(클라이언트 빌드 스크립트가 다른 워크트리의 에디터를 이 프로젝트의 에디터로 본다)에 막혔다. 다른 체크아웃의 Live Coding도 빌드를 막는다는 것을 덧붙였다. 메인 체크아웃의 에디터를 닫고 빌드했다

**#320 (2026년 10월 27일)**

- 게임 코드는 `UP1GameDataSettings`의 `FindItemRow`, `FindItemEquipStatRow`, `FindItemConsumeEffectRow`, `FindMonsterRow`로 생성된 행을 찾는다.
  프로젝트 설정의 테이블 경로는 `/Game/P1/Data/DesignData/DT_<표>`다. 읽는 법은 `docs/codegen.md` 「클라이언트가 읽는 법」에 있다
- 손으로 쓴 `FP1ItemData`, `FP1MonsterData`, `FP1MapData`와 그 리디렉터, 옛 `C_Item.json`, `C_Monster.json`, `C_Map.json`을 지웠다
- 정한 것: 몬스터 스포너는 몬스터 행을 블루프린트 속성(`MonsterDataTable`) 대신 프로젝트 설정에서 찾는다. 속성이 사라져 TD-026(몬스터 테이블이 비면 플레이어도 스폰되지 않음)이 풀렸다
- 정한 것: 툴팁은 장비 능력치와 소모 효과를 아이템 번호로 직접 찾는다. 재사용 대기는 `CooldownMs`를 1000으로 나눠 초로 쓴다
- 맵 행을 읽는 클라이언트 코드는 없었다(`FP1MapData`를 쓰는 곳이 없었다). 맵 번호로 레벨을 여는 `MapLevels`의 키가 이미 맵 표의 `Id`라서 고칠 곳이 없었다
- 퀘스트 보상의 아이템 목록이 `FP1ItemData`였다. JSON의 모양(`Id`, `Quantity`)에 맞춘 `FP1QuestRewardItemData`로 바꿨다. 퀘스트 표는 #322가 걷어낸다
- 아이템 요청 테스트(`P1.Progress.ItemActions`)는 실제 `DT_Item` 에셋을 읽었다. 이제 `Tests/P1DesignDataTestTables.h`가 저장소의 클라이언트 JSON으로 임시 테이블을 채워 설정에 끼운다
- 에셋 테이블의 행 이름을 `.uasset` 문자열에서 뽑아 생성된 JSON의 키와 대조했다. 아이템 39개, 몬스터 9개가 같다
- 사람이 에디터에서 한 것
  1. 에디터 메뉴 **Tools > P1 > 기획 데이터 테이블 채우기**로 `/Game/P1/Data/DesignData/`의 테이블 6개를 만들었다(#319부터 남은 일).
     출력 로그의 행 수가 JSON과 같고(아이템 39, 장비 능력치 36, 소모 효과 3, 몬스터 9, 맵 1, 지역 4), 임포트 문제는 0건이다
  2. `BP_ObjectSpawner`를 다시 저장했다. 지운 속성(`MonsterDataTable`)이 가리키던 옛 `DT_Monster` 참조가 사라졌다
  3. `DT_Quest`를 다시 임포트하고 저장했다. 보상 아이템의 구조체가 `FP1QuestRewardItemData`로 바뀌었다
  4. `Content/P1/Data/DataTables/`의 `DT_Item`, `DT_Monster`, `DT_Map`을 지웠다. 지울 때 행 구조체가 없다는 `LogDataTable` 오류가 나오는 것은 예상한 결과다
- 남은 것: PIE 시나리오 3줄의 「원본에서 지운 행이 테이블에서도 사라진다」와 4줄. 사람이 #312를 마친 뒤 확인한다
