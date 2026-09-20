# Tech Debt

지금 틀린 것만 담는다. 해결이 확정되면 항목을 지운다 — 수정 완료 표기를 남기지 않는다.
무엇을 어떻게 고쳤는지는 커밋이 갖는다.

항목 24개 (높음 4 · 중간 15 · 낮음 5)

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

## Rider MCP의 DB 연결이 읽기 전용 계정을 쓰지 않는다
> **심각도:** 높음 · **난이도:** 낮음 · **범위:** 프로젝트 · ops
> 위치: `P1/.idea/.idea.P1.dir/.idea/dataSources.xml` ·
> `Server/.idea/.idea.Server/.idea/dataSources.xml`
> 등록일: 2026년 9월 16일

`CLAUDE.md` 17줄과 `docs/build.md` 117줄은 Rider의 DB 연결이 읽기 전용 계정(`claude_ro`)을
쓴다고 적는다. 2026년 9월 16일 실측 결과는 다르다.

| 확인 방법 | 결과 |
|---|---|
| `execute_sql_query`로 `SELECT SYSTEM_USER` | Windows 로그인 계정 |
| `list_database_connections` | `UserDB`와 `GameDB` 모두 `readOnly: false` |
| `preview_table_data`로 `Characters` 조회 | 행 2건 반환 |

`dataSources.xml`은 `.gitignore` 대상이라 저장소에 없다. 이 설정은 컴퓨터마다 다를 수 있다.

### 영향

**버그 발생 가능성 증가** · **부채의 연쇄 증가** — 에이전트가 MCP로 여는 DB 세션이 쓰기 권한을
갖는다. 지금 막는 것은 `.claude/hooks/guard_dangerous_cmd.py`의 패턴 검사뿐이고, 그 패턴에 걸리지
않는 쓰기는 통과한다. `execute_sql_query`는 ADR-0002가 남긴 36종에 들어 있고 승인 절차도 없다.
문서가 있지도 않은 방어선을 약속하고 있어서, 문서를 읽은 다음 세션이 그 방어선을 믿고 판단한다.

### 선행 조건

**`claude_ro` 계정이 두 LocalDB 인스턴스에 실제로 있는지 먼저 확인한다.** 계정을 만들어야 하면
`CLAUDE.md` 「안전」이 정한 대로 사람 승인을 먼저 받는다. Rider의 데이터소스 설정은 IDE 화면에서만
바꿀 수 있으므로 에이전트가 할 수 없다. 계정을 바꾼 뒤에는 두 문서의 서술이 사실이 되므로 함께
확인한다.

## 인게임 진입 직후 캐릭터가 스스로 죽는다
> **심각도:** 높음 · **난이도:** 중간 · **범위:** 기능 · client
> 위치: `P1/Content/Blueprints/Creatures/Monster/` · `P1/Content/Blueprints/UI/InGame/WBP_DeathScreen.uasset`
> 등록일: 2026년 9월 16일

인게임 맵에 진입하고 약 3초 뒤에 사망 화면이 뜬다. **서버는 `S_DIE`(1034)도 `S_HIT`(1023)도
보내지 않는다.** 2026년 9월 16일 실측에서 그 세션이 받은 패킷은 아래가 전부다.

| 패킷 | ID | 건수 |
|---|---|---|
| `S_MOVE` | 1020 | 38 |
| `S_SPAWN` | 1017 | 2 |
| `S_LOGIN` | 1003 | 2 |
| `S_ENTER_ROOM` `S_ENTER_GAME` `S_CREATE_CHARACTER` | 1016 1009 1005 | 각 1 |

그럼에도 `WBP_DeathScreen_C_0`이 생성됐다. 즉 클라이언트가 서버 판정 없이 혼자 사망으로
결론지었다. 전투 판정과 사망 화면이 모두 블루프린트에 있어 C++에서 추적할 수 없다.

S_MOVE 38건이 전부 페이로드 0바이트인 점도 같이 본다. 프로토버프가 기본값 필드를 생략하므로,
이는 좌표가 전부 0인 이동이 오갔다는 뜻이다.

### 영향

**버그 발생 가능성 증가** · **테스트 어려움** — 인게임을 3초 넘게 유지할 수 없어 전투·인벤토리·
상점 등 인게임 기능 전체를 손으로 확인할 수 없다. 서버 로그와 대조해도 원인이 클라 블루프린트
안에 있어 좁혀지지 않는다.

## `Room` / `DBRequestFunctions` 갓 클래스
> **심각도:** 높음 · **난이도:** 높음 · **범위:** 모듈 · server
> 위치: `Server/GameServer/Game/Room/Room.cpp` (1,161줄) · `Server/GameServer/DB/DBRequestFunctions.cpp` (1,531줄)
> 등록일: 2026년 8월 19일

`Room` 하나가 입장·퇴장·이동·전투·피격·처치·사망·보상·리스폰·채팅·셀 행렬·몬스터 스폰을 전부
들고 있다(`Room.h` 32~83줄). `DBRequestFunctions`는 캐릭터·상태·인벤토리·장비의 모든 쿼리를 한
파일에 담는다.

`Room`에는 테스트가 없다. 현재 테스트 그물은 Inventory와 프로토콜에만 있다.

### 영향

**테스트 어려움** · **변경 영향 범위 확대** — 두 파일이 서버 코드 10,009줄의 27%다. 어느 기능을
고쳐도 같은 파일을 만지므로 변경이 서로 부딪히고, 테스트 대상을 잘라내기가 불가능하다.

## 몬스터 전체 계층과 전투 로직이 블루프린트에 있다
> **심각도:** 높음 · **난이도:** 높음 · **범위:** 기능 · client
> 위치: `P1/Content/Blueprints/`
> 등록일: 2026년 8월 19일

UE 에디터로 실측한 결과는 아래 두 가지다.

`BP_MonsterBase`(부모 C++ `Monster`)가 `ReceiveBeginPlay`·`ReceiveTick`·`ReceiveActorBeginOverlap`·
`OnPostDie`를 BP로 구현한다. 그 아래 `BP_{Melee,Ranged,Super}MonsterBase` 3개와 미니언 9개가
각자 다시 같은 3개 이벤트를 구현한다. BP 클래스 13개가 몬스터 틱 로직을 나눠 갖고 있다.

`BPC_MonsterAttackSystem`과 `BPC_WarriorAttackSystem`(부모 C++ `AttackSystemComponent`)이
`S_PerformNormalAttack`·`PerformNormalAttack`·`Reset Attack Combo`·`TestAttack`을 BP로 구현하고
`NormalAttacks` 배열을 들고 있다. 콤보 상태 머신과 몽타주 선택이 전부 BP에 있다. C++
`AttackSystemComponent`는 56+52줄뿐이다.

### 영향

**버그 발생 가능성 증가** · **유지보수 어려움** — 서버가 전투를 판정하는데
(`Room::HandleNormalAttack`) 클라 판정 로직은 BP라, 양쪽 규칙이 갈라져도 컴파일러도 테스트도
잡지 못한다. 13개 BP에 흩어진 틱은 호출 순서를 추적할 수 없어 디버깅이 불가능하다.

## 로그인 라우터만 에러를 로그 없이 삼킨다
> **심각도:** 중간 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/AuthServer/src/routes/login.router.js` 64~66줄
> 등록일: 2026년 9월 16일

`POST /login`의 `catch`가 500만 돌려주고 원인을 어디에도 남기지 않는다. 같은 저장소의 다른 두
곳은 남긴다(2026년 9월 16일 실측).

| 위치 | 에러 처리 |
|---|---|
| `account.router.js` 58~60줄 | `console.log`로 원문을 남긴 뒤 500 |
| `app.js` 29~31줄 | `console.error`로 원문을 남김 |
| `login.router.js` 64~66줄 | **남기지 않고** 500 |

이 핸들러가 부르는 것은 bcrypt 비교와 MSSQL 조회와 Redis 기록 셋이다. 셋 중 어디서 터져도
클라이언트가 받는 응답과 서버에 남는 흔적이 똑같다.

ESLint를 붙이자 `no-unused-vars`가 이 자리를 잡았다. 그때는 `catch (err)`의 `err`를 지워 경고만
없앴고, 삼킴 자체는 그대로다.

### 영향

**유지보수 어려움** · **테스트 어려움** — 로그인은 가장 자주 도는 경로인데 500이 나면 원인을
좁힐 방법이 없다. 세 의존성 중 무엇이 죽었는지 알려면 서버에 붙어 재현해야 한다.

## 패킷 핸들러 템플릿이 두 곳에 같은 내용으로 추적된다
> **심각도:** 중간 · **난이도:** 낮음 · **범위:** 모듈 · protocol
> 위치: `Tools/PacketHandlerGenerator/Templates/PacketHandler.h` ·
> `Protocol/Templates/PacketHandler.h`
> 등록일: 2026년 9월 20일

두 파일은 2,561바이트로 내용이 같고 git이 둘 다 추적한다.

생성에 쓰이는 것은 `Protocol/Templates/PacketHandler.h` 하나다. `PacketHandlerGenerator.py`
30줄이 `jinja2.FileSystemLoader('Templates')`로 상대 경로를 읽고, `Protocol/GenPackets.bat`이
`pushd %~dp0`로 `Protocol/`에 들어간 뒤 생성기를 부르기 때문이다.
`Tools/PacketHandlerGenerator/Templates/`는 읽히지 않는다.

### 영향

**동일한 문제의 반복** · **변경 비용 증가** — 생성기 폴더 안에 있는 사본이 더 그럴듯해 보인다.
그쪽을 고치면 생성 결과가 바뀌지 않고, 바뀌지 않는 까닭이 파일 위치가 아니라 실행 시점의 작업
디렉터리에 있어서 원인을 찾는 데 시간이 걸린다.

## `docs/codegen.md`가 재배치 이전 경로를 가리킨다
> **심각도:** 중간 · **난이도:** 낮음 · **범위:** 파일 · protocol
> 위치: `docs/codegen.md` 11·17·57·61·85·87·88줄
> 등록일: 2026년 9월 20일

티켓 #43(PR #61)이 스키마 원본과 기획 수치와 생성기를 저장소 최상위로 올렸다.
`docs/codegen.md`는 그 이동을 반영하지 않았다. 이 문서가 적은 경로 여섯 곳이 지금은 모두
존재하지 않는다.

| 문서의 기재 | 실제 위치 |
|---|---|
| `Server/Common/Protobuf/bin/{Enum,Struct,Protocol}.proto` | `Protocol/Schema/` |
| `Server/Common/Protobuf/bin/GenPackets.bat` | `Protocol/GenPackets.bat` |
| `Server/Common/GameDatasheet/Original_*.xlsx` | `DesignData/Original_*.xlsx` |
| `Server/Common/GameDatasheet/GenJsonFile.bat` | `DesignData/GenJsonFile.bat` |
| `Server/Tools/`의 파이썬 소스 | `Tools/`의 파이썬 소스 |
| `Server/Tools/PacketHandlerGenerator/` · `Server/Tools/ExcelToJsonConverter/` | `Tools/` 아래 같은 이름의 두 폴더 |

`Server/Common/`은 지금 존재하지 않는다. #46이 클라이언트 목적지 두 줄만 고쳤고 나머지는
그대로 두었다.

### 영향

**변경 비용 증가** · **새 기능 개발 지연** — 이 문서는 「프로토콜·데이터 파일을 고치기 전에 반드시
읽는다」로 시작한다. 패킷을 추가하려는 다음 세션이 이 문서를 먼저 읽고 없는 경로를 찾게 된다.
생성기를 돌리는 자리는 두 티어가 공유하는 유일한 계약을 다시 만드는 자리여서, 경로를 못 찾으면
거기서 막힌다.

## 생성기가 만든 패킷 핸들러가 클라 모듈에서 컴파일되지 않는다
> **심각도:** 중간 · **난이도:** 낮음 · **범위:** 프로젝트 · protocol
> 위치: `Protocol/Templates/PacketHandler.h` 5~6줄
> 등록일: 2026년 9월 20일

템플릿 5~6줄이 `#include "SendBuffer.h"`와 `#include "Types.h"`를 쓴다. 두 줄 모두 경로가 없다.
클라이언트 쪽에서 이 include가 해결되는지는 `P1.Build.cs`의 `PrivateIncludePaths`에 달려 있는데,
지금 남아 있는 것은 모듈 루트 `P1/`과 생성물 폴더 `P1/Network` 둘뿐이다.

| 템플릿이 적는 것 | 실제 위치 | 찾히는가 |
|---|---|---|
| `#include "SendBuffer.h"` | `P1/Source/P1/Network/SendBuffer.h` | 찾힌다. `P1/Network`가 경로에 있다 |
| `#include "Types.h"` | `P1/Source/P1/Utils/Types.h` | **찾히지 않는다** |

#46이 `Types.h`를 `Utils/`로 옮겼고 #47이 include 평탄화를 없앴다. 둘을 합치면 이 상태가 된다.
지금 저장소에 커밋된 클라 사본은 `#include "Utils/Types.h"`로 손이 가 있어서 빌드가 통과하지만,
**`Protocol/GenPackets.bat`을 한 번 돌리면 그 사본이 템플릿 출력으로 덮어써지고 클라 빌드가 깨진다.**
`docs/codegen.md`가 「생성기를 다시 돌리는 순간 덮어써진다」고 적은 그대로다.

고치는 방법은 템플릿의 두 줄을 경로 한정으로 바꾸는 것인데, 같은 템플릿이 DummyClient용 출력도
만들고 그쪽 트리는 include 경로가 다르다. 그래서 템플릿 한 벌로는 양쪽을 동시에 만족시킬 수 없다.
생성기(`Tools/PacketHandlerGenerator/PacketHandlerGenerator.py`)는 템플릿 경로를 인자로 받지 않고
`Templates/PacketHandler.h`로 고정한다.

### 영향

**버그 발생 가능성 증가** · **새 기능 개발 지연** — 패킷을 하나 추가하려면 생성기를 돌려야 하는데,
돌리는 순간 클라가 빌드되지 않는다. 깨진 사실이 생성 시점에 드러나지 않고 다음 빌드에서 드러나서,
원인을 생성기가 아니라 방금 추가한 패킷에서 찾게 된다.

## 패킷 핸들러 20개가 `GWorld` 전역에 묶여 있다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 모듈 · client
> 위치: `P1/Source/P1/Network/ClientPacketHandler.cpp` (핸들러 23개 중 20개)
> 등록일: 2026년 9월 16일

`Handle_S_*` 23개 중 20개가 `Cast<UP1GameInstance>(GWorld->GetGameInstance())`로 시작한다.
자기 자신의 월드를 인자로 받지 않고 전역에서 끌어온다.

그래서 핸들러는 **호출되는 시점의 `GWorld`가 게임 월드일 때만** 동작한다. 레벨 스크립트
블루프린트가 펌프를 부르던 동안에는 그 조건이 우연히 성립했다. 월드 틱 안에서 호출됐기
때문이다. 펌프를 코어 티커로 옮기자 조건이 깨졌고, 에디터에서 `GWorld`가 에디터 월드를 가리켜
20개 핸들러가 전부 첫 줄에서 탈락했다(2026년 9월 16일 실측).

지금은 `UP1GameInstance::TickRecvPump`가 펌프 호출 구간에만 `GWorld`를 게임 월드로 바꿔
우회한다. 전역을 직접 대입하는 코드라 그 자체가 부채다.

### 영향

**변경 영향 범위 확대** · **테스트 어려움** — 패킷 처리를 어디에서 부르느냐가 핸들러의 동작을
바꾼다. 호출 지점을 옮길 때마다 20곳이 함께 깨지고, 컴파일러는 아무것도 잡지 못한다. 월드를
인자로 받지 않으므로 핸들러 단위 테스트도 세울 수 없다.

## 접속 정보가 3곳에 컴파일 타임 상수로 흩어져 있다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 프로젝트 · build
> 위치: `Server/GameServer/config.h` · `P1/Source/P1/Core/P1GameInstance.h` 97~98줄 ·
> `P1/Source/P1/Online/P1LoginManager.h` 33~34줄
> 등록일: 2026년 8월 19일

| 위치 | 값 | 형태 |
|---|---|---|
| `Server/GameServer/config.h` | GameDB 접속 문자열, Redis URI | `#define` (gitignore됨) |
| `P1/Source/P1/Core/P1GameInstance.h` 97~98줄 | `127.0.0.1` / `7777` | `const` 멤버 |
| `P1/Source/P1/Online/P1LoginManager.h` 33~34줄 | `127.0.0.1` / `5000` | 멤버 초기값 |
| `Server/AuthServer/.env` | 나머지 전부 | 유일하게 런타임 설정 |

`config.h` 방식의 실질 이점은 두 가지다. 오타가 컴파일 에러로 잡히고, 배포물에 설정 파일을
딸려 보낼 필요가 없다.

### 영향

**새 기능 개발 지연** · **변경 비용 증가** — `config.h`가 gitignore돼 있고 예제 파일도 없어 새로
클론한 사람은 빌드 자체가 안 된다. 테스트용 DB를 가리키게 할 방법이 없어 L1 이상의 통합 테스트를
막는다. 접속처를 바꾸려면 재빌드해야 한다.

## C++ 베이스 없이 BP에만 사는 UI/액터
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 기능 · client
> 위치: `P1/Content/Blueprints/`
> 등록일: 2026년 8월 19일

UE 에디터로 41개 BP의 부모 클래스를 전수 확인한 결과, 위젯 12/15는 이미 C++ 클래스로
리페어런트되어 있다. 남은 것은 아래 넷이다.

| 에셋 | 부모 | BP에 있는 것 |
|---|---|---|
| `WBP_CharacterSlot` | `UserWidget` | 함수 그래프 `UpdateCharacterInfo`·`DisableHighlight`·`Clear`, 디스패처 `OnSlotButtonClicked`, 변수 `Characterid`·`ThisSlotId` |
| `BP_Shop` | `Actor` | 오버랩 상호작용 + `PlayerController` 참조. C++에 `UP1ShopWidget`은 있는데 상점 액터가 없다 |
| `WBP_NameTag` | `UserWidget` | `Tick`·`PreConstruct`·`Construct`. `UP1NameplateWidget`과 역할이 겹친다 |
| `WBP_Help` | `UserWidget` | `Tick`·`PreConstruct`·`Construct`. 순수 표시용 |

C++ 부모가 있는데도 BP 쪽 로직이 무거운 것은 아래 넷이다.

| 에셋 | 부모(C++) | BP에 남은 로직 |
|---|---|---|
| `WBP_Slot` | `SlotWidget` | 그래프 6개(`GetToolTipWidget`·`OnMouseButtonDown`·`OnMouseButtonDoubleClick` 외), 이벤트 `OnStartCooldown`·`OnUpdateCooldown`·`OnUse`, 변수 9개(`CooldownTimerHandle`·`ElapsedTime`·`IntervalTime` 외). 쿨다운 상태 머신 전체 |
| `WBP_LoginMenu` | `LoginWidget` | 그래프 4개(`CC_Init`·`DisableAllSlotsHighlight`·`ClearAllSlots`·`IsValidCharacter`) + `OnDisplayCharacterOverviews` |
| `WBP_DeathScreen` | `DeathWidget` | `Countdown`·`StartCountdown`·`ReturnToTown` + `ReturnCountdown`·`ElapsedTime`·`Timer`. 리스폰 카운트다운 |
| `BP_MyPlayer` / `BP_Player` | `P1MyPlayer` / `P1Player` | `Load and Set SK Mesh`·`Load And Set ST Mesh` + `ChangeMesh`. 장비 메시 교체 |

### 영향

**테스트 어려움** · **유지보수 어려움** — 쿨다운과 카운트다운처럼 시간과 상태를 다루는 로직이
BP에 있으면 단위 테스트가 불가능하고 Live Coding으로도 검증할 수 없다. `WBP_Slot`의 쿨다운
상태 머신과 `WBP_DeathScreen`의 리스폰 카운트다운이 여기 해당한다.

## 캐릭터 클래스와 컨트롤러에 관심사가 뭉쳐 있다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 모듈 · client
> 위치: `P1/Source/P1/Characters/` · `P1/Source/P1/Core/`
> 등록일: 2026년 8월 19일

| 클래스 | 뭉쳐 있는 것 |
|---|---|
| `AP1Creature` (`Characters/P1Creature.h`, 258줄) | 이동 보간(`MoveQueue`·`CorrectionMaxThreshold`·`CORR_INTERP_SPEED`) + 어택 컴포넌트 + 네임플레이트 위젯 + 사망 상태 + `S_*` 수신 처리 |
| `AP1MyPlayer` (`Characters/P1MyPlayer.h`, 126+257줄) | 카메라 붐 + Enhanced Input 액션 5종 + 이동 패킷 스로틀(`MOVE_PACKET_SEND_DELAY`·`YAW_TOLERANCE`·더티 플래그) + 전투 모드 + 디버그 카운터 |
| `AP1InGamePlayerController` (`Core/P1InGamePlayerController.h`, 124+219줄) | 위젯 7종의 `TSubclassOf`/인스턴스 쌍 + `WidgetMappings` + `WidgetFlag` 비트마스크 + `CurrentMaxZOrder` 관리 |

이동 동기화 로직이 수신(`AP1Creature`)과 송신(`AP1MyPlayer`) 양쪽에 갈라져 있다. 보간 상수와
스로틀 상수도 두 파일에 따로 산다.

### 영향

**변경 영향 범위 확대** · **버그 발생 가능성 증가** — 이동 동기화를 고칠 때 한쪽만 고치는 사고가
나기 쉽다. 두 파일의 상수가 어긋나도 컴파일러가 잡지 않고, 증상은 특정 지연 구간에서만
드러난다.

## 소켓과 세션이 해제되지 않는다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 기능 · client
> 위치: `P1/Source/P1/Core/P1GameInstance.cpp` 52~100줄 · `P1/Source/P1/Network/ClientPacketHandler.cpp` 100~107줄
> 등록일: 2026년 9월 16일

`Socket`과 `GameServerSession` 두 멤버에 `nullptr`을 대입하는 코드가 모듈 전체에 하나도 없다
(2026년 9월 16일 실측). 연결을 끊는 경로가 아래처럼 어디에서도 멤버를 비우지 않는다.

| 위치 | 하는 일 | 비우는가 |
|---|---|---|
| `DisconnectFromGameServer()` | `C_LEAVE_GAME` 패킷 전송 | 아니다 |
| `Handle_S_LEAVE_GAME` | 지역 변수로 받아 `Socket->Close()` | 아니다 |
| `PacketSession::Disconnect()` | 송수신 워커 스레드만 정리 | 소켓을 건드리지 않는다 |

`ClientPacketHandler.cpp` 102~104줄에 `DestroySocket`과 `Socket = nullptr`이 주석으로 남아
있다. 주석을 풀어도 지역 변수에 대입하는 형태라 멤버는 그대로다.

`ConnectToGameServer()`는 기존 소켓을 검사하지 않고 매번 새로 만든다. 연결에 실패하면 방금 만든
소켓을 닫지도 파괴하지도 않는다.

### 영향

**버그 발생 가능성 증가** · **변경 영향 범위 확대** — 재접속할 때마다 소켓이 샌다. 끊긴 뒤에도
`Socket`이 유효한 포인터로 남아 `Socket == nullptr` 가드가 통과하므로, 닫힌 소켓에 계속 쓰기를
시도한다. 수명을 고치려면 `P1GameInstance`와 `ClientPacketHandler`와 `PacketSession` 셋을 함께
봐야 한다.

## DummyClient로는 로그인과 이동을 확인할 수 없다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 기능 · build
> 위치: `Server/DummyClient/Main/DummyClient.cpp` ·
> `Server/DummyClient/Main/ClientPacketHandler.cpp`
> 등록일: 2026년 9월 20일

`CLAUDE.md` 「완료 기준」과 `docs/testing.md` 56줄은 테스트가 없는 서버 경로의 검증 하한으로
DummyClient 스모크를 지목한다. 정작 DummyClient가 보내는 패킷은 빈 `C_LOGIN`과 `C_CHAT` 둘뿐이다.

2026년 9월 20일 실측 결과는 아래와 같다.

| 확인 항목 | 결과 |
|---|---|
| 세션 수립 | 100건 전부 성공 |
| `C_LOGIN`이 핸들러에 도달 | 도달함 |
| 로그인 성공(`userId:` 로그) | 0건 |
| `Not Found AccessToken` | 95건 |
| 이동 패킷 전송 | 코드 없음 |

`Handle_C_LOGIN`은 `pkt.access_token()`으로 Redis를 조회한다. DummyClient는 이 필드를 채우지
않으므로 조회가 전부 빗나간다. 유효한 토큰을 얻으려면 인증 서버에 먼저 로그인해야 한다.

### 영향

**테스트 어려움** · **변경 영향 범위 확대** — 스모크로 닿는 곳은 연결과 패킷 프레이밍과 핸들러
디스패치까지다. 로그인 이후의 경로인 캐릭터 로드와 룸 입장과 이동 동기화는 닿지 않는데, 문서는 이
도구를 검증 하한으로 약속한다. 「DummyClient로 확인했다」는 보고가 실제로 확인한 범위보다 넓게 읽힌다.

## CI가 없다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 프로젝트 · build
> 위치: `.github/workflows/` (빈 폴더)
> 등록일: 2026년 8월 19일

`GameServerTests`는 비밀 없이 빌드된다. gitignore된 `config.h`의 유일한 소비자인
`Main/GameServer.cpp`를 제외하므로 판정은 종료 코드 하나다. AuthServer에도 `npm test`가 생겼다.

### 영향

**버그 발생 가능성 증가** · **테스트 어려움** — 3티어 중 어느 하나만 깨져도 손으로 띄워보기
전에는 모른다. 빌드와 테스트가 이미 비밀 없이 도는데도 자동으로 돌지 않으므로, 깨진 커밋이
병합된 뒤에야 발견된다.

## 인벤토리 매핑 3종이 손으로 유지된다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 파일 · server
> 위치: `Server/GameServer/Game/System/Inventory.cpp` (생성자)
> 등록일: 2026년 8월 27일

`Inventory`는 서로 정합해야 하는 표를 넷 들고 있고, 넷 다 생성자에서 손으로 채운다. 제목의
"3종"은 서로 변환하는 표만 센 것이다.

| 표 | 방향 | 쓰는 곳 |
|---|---|---|
| `itemTypeMappings` | 아이템 데이터의 `"itemType"` 문자열 → `ItemType` | `addItem` |
| `slotTypeToItemTypeMappings` | `SlotType` → `ItemType` | `removeItem`, `GetSlot` |
| `inventorylookupMappings` | `ItemType` → 실제 슬롯 배열 | 전부 |
| `dirtyFlagsMappings` | `ItemType` → 슬롯별 더티 플래그 | `addItem`, `removeItem`, `GetDirtyFlags` |

**검사한 표와 인덱싱하는 표가 다른 자리가 둘 있다.**
— `Inventory.cpp:83`은 `addItem`이 69~71줄에서 `inventorylookupMappings`를 `find`로 확인한 뒤
`dirtyFlagsMappings`를 `operator[]`로 인덱싱한다. `Inventory.cpp:162`는 `removeItem`이 135줄에서
`slotTypeToItemTypeMappings`를 확인한 뒤 같은 일을 한다. 지금 터지지 않는 것은 생성자가 네 표를
같은 세 키로 채우기 때문이고, 코드가 그 사실을 보장하지는 않는다.

`Server/GameServerTests/InventoryTests.cpp`가 네 표 중 셋의 키 집합을 기대 집합에 고정하므로,
표가 다시 어긋나면 테스트가 먼저 잡는다. `itemTypeMappings`는 키가 문자열이라 열거형 리플렉션
대조가 닿지 않고, 관측 경로도 없다. 2026년 9월에 검토했다가 폐기한 설계가
`docs/references/work/2026-09-10-inventory-cleanup.md`에 있다.

### 영향

**버그 발생 가능성 증가** · **유지보수 어려움** — 넣을 때와 꺼낼 때가 다른 표를 본다. 두 표가 한
글자만 어긋나도 아이템이 다른 인벤토리로 샌다. 두 값 모두 유효한 enum이라 컴파일러가 아무 말도
하지 않는다.

## 에이전트의 파일 편집을 가로채는 층이 없다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 프로젝트 · build
> 위치: `.claude/settings.json` · `.claude/hooks/guard_dangerous_cmd.py`
> 등록일: 2026년 9월 15일

`guard_dangerous_cmd.py`가 위험한 명령을 차단한다. 이 훅을 부르는 `PreToolUse` 매처는 둘이다.

| 매처 | 덮는 것 |
|---|---|
| `Bash\|PowerShell` | 셸 명령 |
| Rider MCP 툴 21종 | `apply_patch`, `create_new_file`, `rename_refactoring`, `safe_delete` 등 |

두 매처 어디에도 내장 편집 도구인 `Edit`, `Write`, `NotebookEdit`가 없다(2026년 9월 15일 실측).
Rider를 거친 편집은 걸리지만 에이전트가 파일을 직접 고치는 경로는 걸리지 않는다.

CLAUDE.md 「안전」이 "파일 편집에는 셸을 거치지 않는 편집 도구를 쓴다"고 지시하므로, 권장 경로가
곧 검사받지 않는 경로다.

### 영향

**버그 발생 가능성 증가** · **동일한 문제의 반복** — 파일을 어디까지 고쳐도 되는지가 모델의 준수에만
달려 있다. 훅이 덮는 셸 경로와 덮지 않는 편집 경로 사이에 안전 수준의 단차가 생기고, 그 단차는
에이전트가 권장 경로를 쓸수록 커진다.

## `UP1GameInstance`가 클라 측 갓 클래스
> **심각도:** 중간 · **난이도:** 높음 · **범위:** 모듈 · client
> 위치: `P1/Source/P1/Core/P1GameInstance.cpp` (620줄)
> 등록일: 2026년 8월 19일

소켓 소유 + 세션 관리 + `S_*` 핸들러 16개 + 스폰/디스폰 + 델리게이트 5종 브로드캐스트 + 토큰
보관을 한 클래스가 들고 있다.

### 영향

**변경 영향 범위 확대** · **테스트 어려움** — 게임 인스턴스는 레벨 전환에 살아남는 싱글턴이라
여기 붙은 모든 것이 전역 상태가 된다. 핸들러 하나를 고치려 해도 소켓 수명과 델리게이트 구독을
함께 따져야 한다.

## 스택 상한 없는 아이템 누적
> **심각도:** 중간 · **난이도:** 높음 · **범위:** 함수 · protocol
> 위치: `Server/GameServer/Game/System/Inventory.cpp` 166줄
> 등록일: 2026년 8월 19일

`findFirstAvailableSlotId`는 비장비 아이템에서 같은 `template_id` 슬롯을 찾으면 무조건 거기
합친다. 아이템 데이터의 최대 스택 수를 보지 않는다. `JsonProperty::Item::MaxStack`은 선언만
돼 있고 서버 코드 어디에서도 한 번도 읽히지 않는다(실측).

난이도가 높은 이유는 수정 범위가 함수 밖으로 나가기 때문이다. 초과분을 다음 슬롯으로 넘기면
한 번의 구매가 슬롯 두 개를 바꾸는데, `S_BUY_ITEM`은 `Slot updated_slot` 하나만 나른다
(`P1/Source/P1/Network/Protocol.proto` 168~173줄). 서버가 두 번째 슬롯을 클라에 알릴 방법이
없다.

재현 테스트는 `InventoryTest.DISABLED_StackDoesNotExceedMaxStack`에 있다. `DISABLED_`로 둔
이유는 이 범위 문제 때문이지 버그가 아니어서가 아니다.

### 영향

**새 기능 개발 지연** · **버그 발생 가능성 증가** — `maxStack: 10`인 소모품을 15개 구매하면 한
슬롯에 15개가 쌓인다. `--gtest_also_run_disabled_tests`로 실행해 빨강임을 확인했다. 스택 상한을
전제하는 기능(거래, 창고, 제작)은 이 상태 위에 올릴 수 없다.

## `Users.user_id INT` vs `Characters.user_id BIGINT`
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 모듈 · ops
> 위치: `Server/Queries/UserDB_CreateUsersTable.sql` 7줄 ·
> `Server/GameServer/Queries/GameDB_CreateAllTables.sql` 11줄
> 등록일: 2026년 8월 19일

두 티어가 같은 개념을 다른 폭으로 저장한다. `GameSession::userId`는 `int64`다.

### 영향

**버그 발생 가능성 증가** — 사용자 수가 `INT` 상한(2,147,483,647)에 닿으면 두 DB가 같은 사용자를
다른 값으로 보게 된다. 지금은 값이 작아 드러나지 않는다.

### 선행 조건

**스키마 변경이라 사람 승인이 먼저다.** CLAUDE.md 「안전」이 스키마 변경을 사람 승인 후에,
저장소의 SQL 스크립트 갱신과 함께만 실행하도록 정한다. 대상이 서로 다른 두 LocalDB 인스턴스에
걸쳐 있다는 점도 함께 본다. UserDB는 `(localdb)\MSSQLLocalDB`, GameDB는 `(localdb)\ProjectModels`다.
난이도가 낮다고 적혀 있지만 그것은 수정 범위의 크기이지 착수 조건의 무게가 아니다.

## `SendBuffer::Append`와 `Copy`를 부르는 곳이 없다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · client
> 위치: `P1/Source/P1/Network/SendBuffer.cpp` 20~43줄
> 등록일: 2026년 9월 17일

모든 송신이 `ClientPacketHandler::MakeSerializedPacket`을 거치는데, 이 함수는 `Buffer()`로 받은
포인터에 직접 쓰고 `Close()`로 기록 위치만 맞춘다. `Append`와 `Copy`는 거치지 않는다. 저장소
전체에서 두 함수를 부르는 곳은 없다.

두 함수의 가드 조건도 서로 다르다. `Copy`는 `Len()`을 보고 `Append`는 `FreeSize()`를 본다.
쓰는 쪽이 없어서 이 차이가 의도인지 실수인지 확인할 수 없다.

`Append`에는 경계 결함도 있다. 버퍼가 가득 찬 상태(`_writePos == Len()`)에서 `len`이 0 이하면
가드를 통과한 뒤 `WritePos()`가 `_buffer[Num()]`을 인덱싱한다. 지금은 부르는 곳이 없어 드러나지
않는다.

### 영향

**유지보수 어려움** · **변경 비용 증가** — 서버 쪽 `SendBuffer`와 이름이 같아서 같은 역할이라고
읽히지만 클라에서는 동작하지 않는 코드다. 지우든 쓰든 정하기 전에는 이 클래스를 고칠 때마다
쓰이지 않는 절반을 함께 고려해야 한다.

## `GenJsonFile.bat`의 첫 줄이 빈 클라이언트 JSON을 부산물로 남긴다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · tools
> 위치: `DesignData/GenJsonFile.bat` 4줄
> 등록일: 2026년 9월 20일

그 줄은 `Original_Warrior_Level_Data.xlsx`를 변환하면서 `--s_output`만 주고 `--c_output`을 주지
않는다. `Tools/ExcelToJsonConverter/ExcelToJsonConverter.py` 25줄의 argparse 기본값이 그 자리를
메워서, 스크립트를 돌릴 때마다 `C_TestJsonfile.json`이 생성 폴더에 떨어진다. 내용은 빈 객체
50개뿐이고 읽는 곳이 없다. 나머지 네 줄은 `--c_output`을 명시해서 이 부산물을 만들지 않는다.

지금은 `.gitignore`가 이 파일을 가린다. 가리는 것과 만들지 않는 것은 다르다. 고치는 방법은 두
가지다. 그 줄에 `--c_output`을 주고 산출물을 목적지로 옮기거나, 클라이언트 출력이 필요 없음을
생성기에 알리는 인자를 만든다. 후자가 맞지만 생성기를 고치는 일이다.

### 영향

**혼란 유발** — 이슈 #40이 이 파일을 "생성기 산출물이 아님"으로 판정해 지웠다. 실제로는
생성기가 만드는 파일이었고, #43에서 스크립트를 새 위치에서 돌렸을 때 다시 나타났다. 이름에
`Test`가 들어 있고 내용이 비어 있어서 시험 삼아 만든 파일로 읽히는 것이 원인이다.

## `.gitignore`의 `[Ll]og/` 패턴이 클라이언트 소스 폴더까지 무시한다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · build
> 위치: `P1/.gitignore` 127줄
> 등록일: 2026년 9월 20일

127줄의 `[Ll]og/`는 언리얼이 만드는 로그 출력 폴더를 무시하려고 쓴 패턴이다. 경로를 한정하지
않아서 `P1/` 아래 어디에 있든 `Log`나 `log`라는 이름의 폴더를 전부 잡는다.

이 패턴 때문에 `P1/Source/P1/Log/LogCategory.h`와 `LogCategory.cpp` 두 파일이 저장소에 들어간
적이 없다. #46이 두 파일을 `Utils/`로 옮기자 git이 둘을 추적하기 시작했고, 지금 빠져 있는 파일은
없다. 패턴은 그대로 남아 있다.

### 영향

**버그 발생 가능성 증가** — 소스 폴더 이름을 `Log`로 지으면 그 폴더가 통째로 저장소에서 빠진다.
빠진 사실은 커밋할 때 드러나지 않고 새로 클론한 쪽에서 빌드가 깨질 때 드러난다. 고치려면
`/Saved/Logs/`처럼 경로를 한정한다.

## 같은 헤더를 두 번 include하는 파일이 둘 있다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · client
> 위치: `P1/Source/P1/Core/P1GameInstance.cpp` 13·18줄, `P1/Source/P1/UI/P1ShopWidget.cpp` 5·9줄
> 등록일: 2026년 9월 20일

`P1GameInstance.cpp`가 `Characters/P1MyPlayer.h`를, `ShopWidget.cpp`가 `Core/MyPlayerData.h`를
각각 두 번 부른다. 두 헤더 모두 `#pragma once`를 갖고 있어서 컴파일 결과는 달라지지 않는다.

#47이 include를 경로 한정으로 바꾸다가 드러났다. 그 티켓의 범위가 아니어서 줄을 남겨 두었다.

### 영향

**유지보수 어려움** — 드러난 손실은 없다. `#pragma once`가 중복 포함을 막고, 빌드 시간도
재어 볼 만큼 늘지 않는다. include 목록으로 그 파일의 의존 관계를 셀 때 같은 항목이 두 번
세어지는 것이 전부다.
