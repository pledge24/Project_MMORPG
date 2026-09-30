# Architecture

이 문서는 코드를 읽어서 알 수 있는 내용을 반복하지 않는다.
**왜 이렇게 되어 있는지**, 그리고 **깨면 안 되는 것**만 적는다.

최종 수정: 2026-09-XX
검증: 세션 1(2026-08-19)에서 실코드·라이브 DB·UE 에디터와 대조. 이후 구조 변경 없음.

불변식은 `**Architecture Invariant:**`로 표시한다. 대부분 **무언가의 부재**를 가리킨다 —
코드를 읽어서 알아내기 가장 어려운 종류다.

---

## Bird's Eye View

UE5 클라이언트 · 외부 IOCP C++ 게임 서버 · Node 인증 서버의 3티어 MMORPG.
세 티어를 반드시 함께 띄워야 동작한다.

**ground state**는 `UserDB`·`GameDB`의 행과 Redis의 액세스 토큰이다.
**derived state**는 룸 안 오브젝트의 런타임 상태다 — 접속 종료 시 `CharactersLastState`로
내려가고, 재접속으로 재생성된다. 진실의 원천으로 삼지 않는다.

세 티어를 잇는 선은 두 개뿐이다.

- 인증 티어 ↔ 게임 티어: **Redis의 토큰 키 하나**
- 클라 ↔ 게임 서버: **`.proto`에서 생성된 패킷**

---

## Code Map

### Server/AuthServer

계정 생성과 로그인. bcrypt 해시를 `UserDB`에서 검증하고, `last_login`을 갱신한 뒤
UUID 액세스 토큰을 발급해 TTL과 함께 Redis에 넣는다.
로그인과 회원가입은 서로 다른 라우터다(`login.router` / `account.router`).

**Architecture Invariant:** 게임 상태를 모른다. `GameDB`에 접근하지 않는다.

**API Boundary:** HTTP. 게임 티어와의 유일한 접점은 Redis의 토큰 키이며,
이 키의 형태와 TTL이 사실상의 티어 간 계약이다. 게임 서버는 토큰을 한 번 쓰면 지운다. 그래서 TTL은
인증 서버 로그인에서 게임 서버 접속까지의 유효 시간이고, 게임 세션의 길이와 무관하다.

### Server/GameServer

**Architecture Invariant:** `UserDB`를 모른다. 로그인 검증은 Redis의 토큰을 되읽어서 하고,
계정 정보를 DB에서 직접 조회하지 않는다.

**Architecture Invariant:** 룸 소유 상태 변경은 그 룸의 큐 위에서만 일어난다. 락이 없다.
`Room`이 `JobQueue`를 상속하고, 패킷 핸들러는 인라인으로 일하지 않고 `DoAsync`로 잡을
밀어넣고 리턴한다. **다른 룸의 오브젝트에 직접 손대지 않는다.**
DB 작업도 같은 형태다 — 핸들러가 `DBQueue`에 push하고 전용 DB 스레드가 소비한다
(유저 친화도가 필요한 작업은 id 기반 큐로, 그 외는 랜덤 큐로 보낸다).

**Architecture Invariant:** 패킷 핸들러는 전용 스레드가 아니라 **IOCP 워커 스레드에서 돈다.**
워커는 `WORKER_TICK` 예산 안에서 IOCP 디스패치 → 예약 잡 분배 → 글로벌 큐 소비를 반복한다.
워커 풀과 DB 풀은 별개이고, **메인 스레드가 워커 풀의 마지막 하나로 합류한다.**
핸들러 안에서 블로킹하면 IOCP 처리량이 그만큼 줄어든다.

**Architecture Invariant:** DB 접근은 `DB/`의 DAO가 데이터별로 맡는다. SQL을 실행하는 클래스만 DAO라고
부른다. 여러 DAO를 차례로 부르는 `ProgressStorage`는 DAO가 아니다. DAO는 연결을 `DBConnectionGuard`로
빌려 함수가 어떻게 끝나든 풀로 돌려준다. 접속 종료 때 저장할 아이템 행은 DB를 모르는 `ItemSaveRows`가
고른다.
— 저장할 행을 잘못 고르면 진행이 사라지는데, SQL 실행은 테스트할 수 없어 이 판정만 떼어 테스트한다.

**Architecture Invariant:** 로그인 핸들러만 예외적으로 `DBQueue` 위에서 시작한다.
Redis 재검증과 캐릭터 로드가 이어져야 하기 때문이다. 다른 진입점을 여기에 얹지 않는다.

**Architecture Invariant:** 한 계정은 세션 하나만 갖고, 나중에 온 로그인이 이긴다. `Handle_C_LOGIN`은
Redis의 토큰 키를 읽고 지우며, 키를 지운 쪽만 통과한다. 통과하면 `GSessionManager.RegisterUser`가
계정에 세션을 묶고 기존 세션을 돌려준다. 확인과 교체는 관리자의 락 하나 안에서 한다. 로그인 잡은
랜덤 DB 큐에서 돌아서 같은 계정의 로그인이 동시에 올 수 있기 때문이다. 밀려난 세션에는
`S_LEAVE_GAME(DUPLICATE_LOGIN)`을 보내고 끊는다. 그 세션의 저장은 아래의 접속 종료 경로를 탄다.

**Architecture Invariant:** 캐릭터를 다루는 요청(입장, 삭제)은 그 계정이 가진 캐릭터에만 동작한다.
클라이언트가 보낸 `character_id`를 믿지 않고, SQL이 세션의 `user_id`를 함께 대조한다. 생성 요청은
`Handle_C_CREATE_CHARACTER`가 `CharacterCreation::Validate`로 먼저 거른다.

**Architecture Invariant:** 룸 퇴장과 진행 저장은 `GameSession::OnDisconnected`에서만 시작한다.
`C_LEAVE_GAME`도 연결을 끊어서 이 경로로 온다. 그래서 정상 종료와 크래시, 네트워크 단절이 같은
처리를 받는다. 저장은 룸 큐 위에서 뜬 `PlayerSaveData` 사본으로 하고, DB 스레드는 살아 있는
`Player`를 읽지 않는다. 저장 잡은 입장 불러오기와 같은 `userId` 큐에 넣는다. 그래야 곧바로 다시
접속해도 저장이 끝난 뒤에 불러온다.

**Architecture Invariant:** 몬스터의 `S_DIE`는 디스폰을 겸한다. 서버는 사망한 몬스터를 룸에서 곧바로
지우고 `S_DESPAWN`을 보내지 않는다. 클라이언트의 `AP1Monster`가 사망 애니메이션을 보여 준 뒤 스스로
지운다. 플레이어의 `S_DIE`는 디스폰을 겸하지 않는다. 사망한 플레이어는 리스폰할 때까지 룸에 남는다.
— 서버가 몬스터에 `S_DESPAWN`을 보태면 사망 애니메이션이 끝나기 전에 액터가 사라진다.

**Architecture Invariant:** 피격과 처치는 `Game/Combat/`이 판정한다. 판정은 룸과 세션을 모른다.
룸은 공격자가 아직 룸에 있는지 대조하고, 대상을 찾고, 결과를 패킷으로 보낸다.
— 판정이 룸에 묶여 있으면 룸 큐와 세션 없이는 테스트할 수 없다.

**Architecture Invariant:** 소모품 사용은 요청이 아니라 인벤토리 슬롯에 든 아이템으로 판정한다. 요청의
`template_id`를 믿지 않는다. 재사용 대기도 게임 서버가 템플릿마다 판정하고, `Player`의 메모리에만 두어
접속이 끊기면 사라진다. 클라이언트의 재사용 대기 표시는 보여 주기만 한다.

**Architecture Invariant:** 서버 오브젝트는 상태를 protobuf 메시지로 직접 들고 있다.
**복제가 변환이 아니라 복사다.** 새 상태 필드를 서버 클래스에 추가하는 것은
곧 프로토콜 변경이다.

`Room`은 브로드캐스트 범위 계산과 근접 탐색을 위해 `CELL_SIZE` 단위 셀 행렬을 유지하고,
`ROOM_UPDATE_INTERVAL_MS` 주기로 갱신한다.

**API Boundary:** 패킷 핸들러. `C_*` 로만 진입한다. 처리 경로는 네 가지뿐이다 —
`DBQueue`(로그인·캐릭터 생성/삭제·게임 입장), 룸 큐(그 외 대부분), 혼합, 스텁.
새 핸들러는 이 넷 중 하나에 들어가야 한다. **다섯 번째를 만들지 않는다.**

일부 패킷은 의도적으로 스텁이다(핑/퐁, 맵 로드 완료). 채팅은 서버가 처리하지만
클라에 UI가 없어 로그만 남는다. **미구현이 아니라 미완성 기능이다.**

### P1 (UE 클라이언트)

**Architecture Invariant:** 네트워크 스레드는 UObject를 절대 만지지 않는다.
`PacketSession`이 소유한 두 `FRunnable` 워커는 바이트를 큐에 쌓기만 하고,
게임 스레드의 수신 펌프가 그것을 비운다.

**Architecture Invariant:** 수신 펌프는 `UP1GameInstance`가 코어 티커로 돌린다. 레벨과 무관하게 돈다.
월드가 `BeginPlay` 전이거나 해체 중이면 그 틱을 건너뛰고 큐를 비우지 않는다. 그래서 레벨 전환 중에 온
패킷은 새 월드가 준비된 뒤에 처리된다. 레벨 블루프린트에서 펌프를 부르지 않는다.

**Architecture Invariant:** 연결이 끊기면 수신 워커만 `PacketSession`에 끊김 표시를 세운다. 펌프는 표시를
먼저 읽고 큐를 비운 뒤, 표시가 서 있었으면 `UP1GameInstance::ReturnToLogin`으로 로그인 맵을 연다.
수신 워커는 마지막 패킷을 큐에 넣은 뒤 표시를 세우므로 끊기기 직전에 온 `S_LEAVE_GAME`을 놓치지
않는다. 서버가 `S_LEAVE_GAME`으로 끊어도 같은 함수로 간다. 사용자가 게임을 끄는 `Shutdown`은 이
경로를 타지 않는다.

**Architecture Invariant:** `UP1GameInstance`가 소켓·세션을 소유하는 유일한 허브다.
모든 `S_*` 핸들러가 여기 구현되고, 액터와 위젯에는 멀티캐스트 델리게이트로만 전파된다.
액터가 세션을 직접 잡지 않는다.

**Architecture Invariant:** 내 플레이어의 스탯은 `UP1MyPlayerData::ApplyStat`으로만 바꾼다. 이 함수가
사본에 쓰고 델리게이트로 알린다.
— 위젯은 생성될 때 사본을 읽는다. 알리기만 하고 사본을 두면 레벨을 옮긴 뒤 옛 값이 보인다.

**Architecture Invariant:** 폴더가 도메인으로 갈려 있고 `#include`는 경로를 한정한다.
`P1.Build.cs`의 `PrivateIncludePaths`에는 모듈 루트 `P1/`과 생성물 폴더 `P1/Network`만 남는다.
도메인을 넘는 참조가 `#include` 줄에 드러나므로 **검색 한 번으로 잡힌다.**
> 컴파일러는 경계 위반을 막지 않는다. UBT가 모듈 루트를 include 경로에 넣으므로, 경로만
> 한정하면 어느 도메인이든 부른다. 차단하려면 도메인을 별도 모듈로 나눠야 하고 지금 규모에서는
> 나누지 않는다. 근거는 `docs/adr/0005-drop-include-path-flattening.md`에 있다.

**API Boundary:** `P1/Source/`에는 모듈이 둘이고(`P1`, `ProtobufCore`), protobuf를 아는
경계는 `ProtobufCore`다. 상세는 `docs/build.md`.

### 생성물

**Architecture Invariant:** 패킷 정의와 게임 데이터는 생성물이다.
저장소에 커밋되어 있어 직접 고쳐도 되는 파일처럼 보이지만, 생성기를 다시 돌리면
덮어써진다. **손으로 쓰는 것은 핸들러 `.cpp` 둘뿐이다.**
절차와 목적지 목록은 `docs/codegen.md`.

**Architecture Invariant:** 패킷 접두사가 방향을 정한다. `C_*`는 클라→서버,
`S_*`는 서버→클라이며, 생성기가 이 접두사로 핸들러 테이블을 나눈다.
요청/응답 쌍은 이름을 공유한다. **이름 규칙이 장식이 아니라 기능이다.**

---

## Layering Rules

**클라와 서버의 클래스 계층이 대칭이다.** `Object → Creature → { Player, Monster }`.
서버는 상태를 protobuf로 들고 클라는 그것을 액터에 반영한다.

그래서 게임플레이 변경은 **클라 + 서버 + 프로토콜 3곳을 기본으로 잡는다.**
한쪽만 고치면 어긋나고, 어긋남을 빌드가 잡아주지 않는다.

**하나의 동작을 세 곳에 걸쳐 세로로 자른다.** 프로토콜을 전부 먼저 하고 서버를 전부 하는
방식은 중간 상태가 검증 불가능해진다.

**계층을 자르는 쪽이 이 저장소의 최대 위험이다.** 3곳을 건드리는 구조라 계층별로 자르고
싶어지는데, 그렇게 쓴 테스트는 러너가 잡지 못한다. 통과하면서 실질적으로 아무것도 주장하지 않는다.

### 티어 간 계약

| 계약 | 형태 | 깨지면 |
|---|---|---|
| 인증 ↔ 게임 | Redis 토큰 키와 TTL | 로그인이 통과해도 게임 입장이 실패 |
| 클라 ↔ 게임 서버 | 생성된 패킷 | 빌드는 통과하고 런타임에 어긋남 |
| 게임 서버 ↔ GameDB | SQL 스크립트 | 부팅 또는 첫 쿼리에서 터짐 |

**주의: 계정 id의 폭이 티어마다 다르다.** 인증 티어와 게임 티어가 같은 값을 다른 폭으로
들고 있다. 지금은 값이 작아 드러나지 않는다. (tech-debt 참조)

---

## Cross-Cutting Concerns

### 저장소

두 DB는 **서로 다른 LocalDB 인스턴스**에 있고 접속 문자열의 출처도 다르다 —
게임 쪽은 컴파일 타임 상수, 인증 쪽은 환경 파일.
**Architecture Invariant:** 각 DB는 소유 티어만 접근한다. 교차 접근 경로가 없다.
인스턴스·경로·스크립트 목록은 `docs/build.md`. 스키마의 원본은 SQL 스크립트다.

### 코드 생성

`docs/codegen.md`. 생성물을 직접 고치지 않는다.

### 테스트

판정은 종료 코드다. 로그 문자열로 성공을 판단하지 않는다.
계층별 실행 경로와 현재 커버리지는 `docs/testing.md`.

**Architecture Invariant:** 프로토콜 회귀 그물은 매크로 목록과 리플렉션으로 돈다.
메시지가 늘어도 테스트를 고칠 필요가 없다. **고쳐야 한다면 뭔가 잘못된 것이다.**

### 인코딩

`.proto`와 `.bat`은 cp949, 나머지는 UTF-8. UTF-8로 읽으면 깨져 보이는 것이 정상이고,
"고치면" 그 파일을 소비하는 툴이 깨진다.

### 빌드 환경

런처 설치본 엔진 고정. 엔진 소스 패치와 프로젝트 내 `TargetType.Program` 타깃이
불가능하다. 무언가를 계획하기 전에 이 제약부터 본다. 상세는 `docs/build.md`.

---

## 이렇게 안 한 이유

`docs/references/decisions/index.md` 참조. 하네스 v1과 함께 걷어낸 기록이라 현행 제약이 아니다.
현행 제약은 각 문서의 본문에 흡수했다.