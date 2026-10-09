# Tech Debt

지금 틀린 것만 담는다. 해결이 확정되면 항목을 지운다 — 수정 완료 표기를 남기지 않는다.
무엇을 어떻게 고쳤는지는 커밋이 갖는다.

항목 42개 (높음 3 · 중간 5 · 낮음 34) · 다음 번호 TD-045

## 작성 방법

항목마다 `TD-NNN` 번호를 붙인다. 번호는 항목을 만든 순서다. 새 항목에는 파일 맨 위의 「다음 번호」를 쓰고,
「다음 번호」를 하나 올린다. 항목을 지워도 그 번호를 다시 쓰지 않는다. 다른 문서에서 항목을 가리킬 때는 번호와
제목을 함께 적는다.

항목은 심각도 절(`# 높음`, `# 중간`, `# 낮음`)에 넣는다. 세 절은 `---`로 나누고, 항목이 없는 절도 지우지 않는다.
심각도를 바꾸면 항목을 그 절로 옮기고 번호는 그대로 둔다. 한 절 안에서는 난이도가 낮은 항목을 위에 두고,
난이도가 같으면 번호 순서로 둔다.

### 새 항목 양식

아래 블록을 복사해서 쓴다.

```markdown
## TD-NNN 무엇이 틀렸는가를 한 줄로
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
- 항목을 추가하거나 지우면 파일 맨 위의 개수 줄을 함께 고친다. 추가할 때는 「다음 번호」도 고친다
- 항목을 고칠 GitHub 이슈에는 `tech-debt` 라벨을 붙인다. 이슈를 먼저 열고 나중에 항목을 적었으면 그때 붙인다

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

# 높음

## TD-044 서버를 종료하면 접속 중인 플레이어의 진행이 사라진다
> **심각도:** 높음 · **난이도:** 높음 · **범위:** 기능 · server
> 위치: `Server/GameServer/GameServer.cpp` 25~38줄 (`DoWorkerJob`), 105~116줄
> 등록일: 2026년 10월 9일

워커 루프가 `while (true)`로 돌고, 콘솔 종료 처리기가 없다. `GThreadManager->Join()`과 `return 0`에 도달하지 않는다. 진행은
`GameSession::OnDisconnected`에서만 저장하고 주기 저장이 없으므로, 서버 창을 닫거나 Ctrl+C를 누르면 접속 중인 플레이어 전원의
진행이 로그인 이후 것까지 사라진다. 종료 경로를 만들면 GameServer와 ServerCore의 전역 객체가 서로 다른 번역 단위에 있어서
소멸 순서가 정해지지 않는 문제도 함께 드러난다. 코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 개발 중 서버를 다시 띄울 때마다 진행이 사라져서, 진행에 기대는 기능을 검증하기 어렵다.

---

# 중간

## TD-006 장비 착용·해제의 성공 응답이 끊긴 세션을 검사하지 않고 보낸다
> **심각도:** 중간 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/GameServer/Game/Room/Room.cpp` 531~536줄, 571~576줄 (`C_HandleEquipGear`, `C_HandleUnequipGear`)
> 등록일: 2026년 10월 5일

두 함수는 실패 응답을 보낼 때 `player->_session.lock()`의 결과를 `if`로 검사한다. 성공 응답을 보낼 때는 같은
결과를 검사하지 않고 `SEND_PACKET`으로 `session->Send`를 부른다. `Handle_C_EQUIP_GEAR`와 `Handle_C_UNEQUIP_GEAR`는
세션을 잡에 담지 않고 플레이어만 넘긴다. 그래서 잡이 룸 큐에서 기다리는 사이에 연결이 끊겨 세션이 소멸하면
`lock()`이 널을 돌려준다. 코드를 읽고 판단했고 실행해서 재현하지는 않았다.

### 영향

**버그 발생 가능성 증가** — 착용이나 해제를 요청한 직후에 연결이 끊기면 룸 잡이 널 포인터를 역참조해 게임 서버가 죽는다.

## TD-007 이동 요청이 보낸 사람이 아니라 패킷의 엔티티 번호로 플레이어를 찾는다
> **심각도:** 중간 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/GameServer/Game/Room/Room.cpp` 389~400줄 (`C_HandleMove`) · `Server/GameServer/Network/ServerPacketHandler.cpp` 336~351줄 (`Handle_C_MOVE`)
> 등록일: 2026년 10월 5일

`Handle_C_MOVE`는 보낸 세션의 플레이어를 룸 잡에 넘기지 않고 패킷만 넘긴다. `Room::C_HandleMove`는
`FindEntityAs<Player>(pkt.info().entity_id())`로 대상을 찾고 `_posInfo->CopyFrom(pkt.info())`로 위치를 덮어쓴다.
클라이언트는 `UP1MoveSyncComponent`가 캐시한 위치를 엔티티 번호와 함께 그대로 보낸다. 위치값 자체도 검증하지 않는다.
코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 조작한 클라이언트는 같은 룸에 있는 다른 플레이어의 위치를 바꿀 수 있다. 피해자가 그
상태로 접속을 끊으면 바뀐 위치가 `SaveLastState`로 저장되어 다른 계정의 진행이 손상된다.

## TD-008 클라이언트가 엔티티를 디스폰할 때 맵에서 지운 원소를 역참조한다
> **심각도:** 중간 · **난이도:** 낮음 · **범위:** 함수 · client
> 위치: `P1/Source/P1/Sync/P1StatefulEntityManager.cpp` 136~149줄 (`DespawnEntity`)
> 등록일: 2026년 10월 5일

`DespawnEntity`는 `Players.Find`로 얻은 원소 포인터를 쥔 채 `UnRegisterEntity`를 부른다. `UnRegisterEntity`가
그 원소를 맵에서 `Remove`한 뒤에 `(*FindPlayer)->Destroy()`로 포인터를 역참조한다. 몬스터 분기도 같은 순서다.
제거된 TMap 원소를 읽는 것은 정의되지 않은 동작이다. 지금은 원소가 있던 메모리가 바로 재사용되지 않아서
동작하는 것으로 보인다. 코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 엔진 버전이나 할당 패턴이 바뀌면 디스폰할 때 다른 액터를 지우거나 클라이언트가 죽는다.
원인이 디스폰과 떨어진 곳에서 드러나서 추적하기 어렵다.

## TD-042 몬스터의 틱이 다른 룸으로 떠난 플레이어의 위치를 읽는다
> **심각도:** 중간 · **난이도:** 중간 · **범위:** 기능 · server
> 위치: `Server/GameServer/Game/Entities/Monster.cpp` 378~386줄 (`ExecuteStateAttacking`), 399~423줄 (`ExecuteStateChasing`), 521~533줄 (`IsTargetLost`)
> 등록일: 2026년 10월 9일

대상이 룸에 남아 있는지는 200ms마다 도는 `UpdateState`의 `IsTargetLost`만 확인한다. 50ms마다 도는 틱은 `_target.lock()`만 하고
`target->_posInfo`를 읽는다. 플레이어가 포털로 다른 룸에 가면, 그 룸 큐의 `C_HandleMove`가 같은 `_posInfo`를 쓰는 동안 이 몬스터의
룸 큐가 읽는다. 「다른 룸의 오브젝트에 직접 손대지 않는다」는 불변식을 최대 200ms 동안 어긴다.

고칠 때 함정이 있다. 퇴장할 때 `_room`을 비우면 `Player::ApplyTownRespawnForSave`가 실패한다. 이 함수는 이미 떠난 룸이 `_room`에
남아 있어야 성공하므로, 사망한 채 끊긴 플레이어가 HP 0으로 저장된다. 코드를 읽고 판단했고 경합을 재현하지는 않았다.

### 영향

**버그 발생 가능성 증가** — 정의되지 않은 동작인 데이터 경합이다. 몬스터가 찢어진 위치를 읽어 엉뚱한 방향을 보거나 이동한다.

---

# 낮음

## TD-005 인증 서버의 Redis 주소 설정이 클라이언트에 전달되지 않는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · server
> 위치: `Server/AuthServer/src/DB/redis.js` 5~8줄
> 등록일: 2026년 10월 4일

`redis.createClient({ host, port })`로 `.env`의 `REDIS_HOST`와 `REDIS_PORT`를 넘긴다. node-redis v5의 옵션에는
최상위 `host`와 `port`가 없고 `url`이나 `socket: { host, port }`만 있어서, 두 값은 무시된다. 클라이언트는
기본값인 `localhost:6379`로 붙는다. `@redis/client`의 타입 정의를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 지금은 설정값과 기본값이 같아서 동작한다. Redis 주소나 포트를 바꾸면 인증 서버만
옛 주소로 붙고, 게임 서버는 토큰을 찾지 못해 모든 로그인이 `INVALID_TOKEN`으로 끝난다.

## TD-010 리슨 소켓의 네이글 비활성화가 주석과 반대로 동작한다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/ServerCore/Network/Listener.cpp` 52~54줄 (`Listen`)
> 등록일: 2026년 10월 5일

`Listener::Listen`은 주석이 「네이글 알고리즘 비활성화」인데 `SetTcpNoDelay(_listenSocket, false)`로 `false`를 넘긴다.
대상도 리슨 소켓뿐이고 세션 소켓에는 걸지 않는다. 코드를 읽고 판단했고 패킷 지연은 측정하지 않았다.

### 영향

**버그 발생 가능성 증가** — 세션 소켓에 네이글 알고리즘이 켜져 있어 작은 패킷(이동, 공격)이 모였다가 나갈 수 있다.
주석을 믿고 지연 원인에서 소켓 옵션을 빼면 진단이 늦어진다.

## TD-011 IOCP 오류 경로의 검사가 틀렸다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · server
> 위치: `Server/ServerCore/Network/IocpCore.cpp` 11~12줄, 36~49줄
> 등록일: 2026년 10월 5일

- `CreateIoCompletionPort`는 실패하면 `NULL`을 돌려준다. 생성자는 결과를 `INVALID_HANDLE_VALUE`와 비교하므로 실패를 잡지 못한다
- `GetQueuedCompletionStatus`가 실패하면 `WAIT_TIMEOUT`이 아닌 경우 모두 `networkEvent->owner`를 역참조한다.
  완료 패킷을 꺼내지 못한 실패에서는 `networkEvent`가 `nullptr`다
- I/O가 실패한 완료도 같은 `Dispatch`로 보낸다. `Session`은 `numOfBytes == 0`을 보고 끊으므로, 오류 처리가 바이트 수 0에 기대고 있다

코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — IOCP 핸들 생성 실패나 포트 자체의 오류가 생기면 원인 로그 없이 워커 스레드가 널 포인터로 죽는다.

## TD-012 리스너의 Accept 오류 경로가 실패를 검사하지 않는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · server
> 위치: `Server/ServerCore/Network/Listener.cpp` 40~56줄, 118~130줄 · `Server/ServerCore/Network/Service.cpp` 35~50줄
> 등록일: 2026년 10월 5일

- `Service::CreateSession`이 소켓을 IOCP에 등록하고, `Listener::RegisterAccept`가 같은 소켓을 다시 등록한다. 두 번째 반환값은 보지 않는다
- `CreateSession`은 등록에 실패하면 `nullptr`를 돌려주는데 `RegisterAccept`와 `ClientService::Start`는 검사하지 않는다
- `AcceptEx`가 `WSA_IO_PENDING` 말고 다른 오류로 실패하면 `RegisterAccept`가 자신을 다시 부른다. 오류가 계속되면 재귀가 끝나지 않는다
- `maxSessionCount`는 이름과 달리 동시 접속 상한이 아니라 동시에 걸어 두는 `AcceptEx`의 개수다. `AddSession`은 상한을 보지 않는다

코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** · **유지보수 어려움** — 소켓 자원이 바닥난 상태에서 접속이 몰리면 리스너가 스택 오버플로나
널 포인터로 죽는다. `maxSessionCount`라는 이름만 보고 접속 상한이 있다고 믿게 된다.

## TD-013 수신 등록이 일부 오류에서 세션을 끊지도 다시 걸지도 않는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/ServerCore/Network/Session.cpp` 159~181줄 (`RegisterRecv`), 329~341줄 (`HandleError`)
> 등록일: 2026년 10월 5일

`WSARecv`가 `WSA_IO_PENDING` 말고 다른 오류로 실패하면 `HandleError`를 부른다. `HandleError`는 `WSAECONNRESET`과
`WSAECONNABORTED`일 때만 끊고, 나머지 오류는 로그만 찍는다. 그 경우 수신이 다시 걸리지 않고 세션은 연결된 상태로 남는다.
코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 그 세션은 패킷을 더 받지 못하는데 끊기지 않아서, 접속 종료 저장도 일어나지 않고 룸에 플레이어가 남는다.

## TD-014 `JobQueue::ClearJobs` 뒤에 그 큐가 다시 실행되지 않는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/ServerCore/Job/JobQueue.h` 39줄
> 등록일: 2026년 10월 5일

`ClearJobs`는 `_jobs`만 비우고 `_jobCount`를 줄이지 않는다. `Push`는 `_jobCount`의 이전 값이 0일 때만 실행을 맡으므로,
비운 뒤에 들어온 잡은 실행되지 않는다. 지금 `ClearJobs`를 부르는 곳은 없다.

### 영향

**버그 발생 가능성 증가** — 룸을 정리하는 데 이 함수를 쓰기 시작하면 그 룸이 조용히 멈춘다.

## TD-015 타이머 분배가 잡 실행이 끝날 때까지 막힌다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/ServerCore/Job/JobTimer.cpp` 19~51줄 (`Distribute`)
> 등록일: 2026년 10월 5일

`Distribute`는 `_distributing`을 세운 채 만기 항목마다 `owner->Push(job)`를 부른다. `Push`는 그 큐가 비어 있으면
분배하던 스레드에서 바로 `Execute`하므로, 잡 실행이 끝날 때까지 `_distributing`이 내려가지 않는다. 그동안 다른 워커는
분배를 건너뛰고, 같은 묶음의 뒤쪽 항목도 늦게 들어간다. 코드를 읽고 판단했고 지연을 측정하지는 않았다.

### 영향

**버그 발생 가능성 증가** — 몬스터의 50ms 틱과 200ms 상태 판정이 모두 타이머로 돌기 때문에, 한 룸의 무거운 잡이 다른 룸의
몬스터 틱을 늦춘다.

## TD-016 DB 연결을 정리하는 경로가 핸들을 해제하지 못한다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 모듈 · server
> 위치: `Server/ServerCore/DB/`
> 등록일: 2026년 10월 5일

- `DBConnection::Clear`는 DBC 핸들을 STMT 핸들보다 먼저 해제하고 `SQLDisconnect`를 부르지 않는다
- `DBConnection`에 소멸자가 없어서 `DBConnectionPool::Clear`의 `delete`는 ODBC 핸들을 해제하지 않는다. 그 전에 환경 핸들부터 해제한다
- `DBConnectionPool::Connect`는 연결에 실패하면 만든 `DBConnection`을 지우지 않는다. `DBConnection::Connect`는 연결 결과를 보기 전에 STMT를 할당한다
- `DBQueue`에는 `stopFlag`를 세우는 함수가 없어서 DB 스레드를 멈출 수 없다

지금 게임 서버에는 종료 경로가 없어서 드러나지 않는다. 코드를 읽고 판단했다.

### 영향

**유지보수 어려움** — 정상 종료나 연결 재시도를 만들 때 이 경로를 그대로 쓰면 핸들이 새고 DB 스레드가 끝나지 않는다.

## TD-017 게임 서버가 DB를 준비하기 전에 접속을 받는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · server
> 위치: `Server/GameServer/GameServer.cpp` 70~112줄 · `Server/GameServer/DB/ItemDAO.cpp` 35~80줄 (`GetMaxItemUID`)
> 등록일: 2026년 10월 5일

`main`은 `service->Start()`로 리슨을 연 뒤에 DB에 연결하고, 워커 스레드를 띄운 뒤에 `GDBManager->Init`과
`ItemDAO::GetMaxItemUID`를 부른다. 그 사이에는 다음 일이 생길 수 있다.

- 워커가 뜬 뒤 `GDBManager->Init` 전에 `C_LOGIN`이 오면 DB 큐가 0개라 `GetRandom(0, -1)`로 범위 밖 큐를 고른다.
  `GetDBQueueFromId`는 0으로 나눈다
- `GNextItemUID`가 정해지기 전에 아이템이 만들어질 수 있다

`GetMaxItemUID`는 실패하면 `wstring`을 던지는데 `catch`는 `DBCustomError`만 받으므로 `std::terminate`로 끝난다.
결과를 받는 열 변수도 `int32`인데 `GNextItemUID`와 `item_uid`는 `int64`다. 코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 클라이언트가 서버 기동 직후에 붙으면 서버가 죽거나 아이템 번호가 겹친다. 아이템 번호가
`INT32` 상한을 넘으면 다음 번호가 틀린다.

## TD-018 로그인 잡이 Redis 값의 JSON 파싱 예외를 받지 않는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/GameServer/Network/ServerPacketHandler.cpp` 93~95줄 (`Handle_C_LOGIN`)
> 등록일: 2026년 10월 5일

`Json::parse(*val)`와 `json["userId"]`가 `try` 밖에 있다. 이 코드는 DB 스레드의 잡 안에서 돌고, DB 스레드 루프에도
예외를 받는 곳이 없다. 코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 인증 서버가 토큰 값의 형식을 바꾸거나 키에 다른 값이 들어가면 로그인 한 번에 게임 서버가 죽는다.

## TD-019 캐릭터 요청이 세션의 로그인과 입장 상태를 보지 않는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 기능 · server
> 위치: `Server/GameServer/Network/ServerPacketHandler.cpp` 126~238줄 · `Server/GameServer/Game/Entities/Player.cpp` 43~63줄 (`Init`)
> 등록일: 2026년 10월 5일

- `Handle_C_CREATE_CHARACTER`, `Handle_C_DELETE_CHARACTER`, `Handle_C_ENTER_GAME`은 `_userId`가 0이어도, 즉 `C_LOGIN`을
  거치지 않은 세션이어도 진행한다. 삭제와 입장은 SQL의 `user_id` 대조로 실패하지만, 생성은 `user_id` 0으로 INSERT를 시도한다.
  DB 제약이 막는지는 확인하지 않았다
- `Handle_C_ENTER_GAME`은 세션에 이미 플레이어가 있는지 보지 않는다. `Player::Init`이 `session->_player`를
  덮어쓰므로, 룸에 들어간 뒤 `C_ENTER_GAME`을 다시 보내면 이전 `Player`가 룸에 남는다. `OnDisconnected`는 새 플레이어만
  보므로 이전 플레이어는 퇴장하지 않는다

코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 조작한 클라이언트는 주인 없는 캐릭터 행을 만들거나, 룸에 지워지지 않는 플레이어를 남긴다.

## TD-020 패킷 핸들러의 반환값을 아무도 읽지 않는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/GameServer/Network/GameSession.cpp` 76~84줄 (`OnRecvPacket`)
> 등록일: 2026년 10월 5일

`OnRecvPacket`은 `ServerPacketHandler::HandlePacket`의 반환값을 버린다. 그래서 `ParseFromArray`가 실패하거나 핸들러가
`false`를 돌려줘도 로그도 끊기도 없다. 같은 함수의 `header` 변수는 쓰이지 않는다.

### 영향

**유지보수 어려움** — 클라이언트와 서버의 프로토콜이 어긋나도 패킷이 조용히 사라져서 원인을 찾기 어렵다.

## TD-021 처음 입장한 플레이어에게 자기 스폰을 두 번 보낸다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/GameServer/Game/Room/Room.cpp` 304~310줄, 893~909줄
> 등록일: 2026년 10월 5일

`C_HandleEnterRoom`의 INITIAL 분기는 `SpawnPlayer(player)`를 부른 뒤 `ReplicateRoomData(player, true)`를 부른다.
`SpawnPlayer`는 `Broadcast(sendBuffer)`로 본인을 빼지 않고 보내고, `ReplicateRoomData`도 `includeThisPlayer`가 참이라
본인을 다시 싣는다. 클라이언트 `UP1StatefulEntityManager::SpawnPlayer`가 이미 있는 번호를 무시해서 드러나지 않는다.

### 영향

**버그 발생 가능성 증가** — 클라이언트의 중복 검사를 지우거나 스폰에 부수 효과를 붙이면 내 플레이어가 두 번 처리된다.

## TD-023 생성된 패킷 직렬화가 크기를 `uint16`으로 자른다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · protocol
> 위치: `Protocol/Templates/PacketHandler.h` 75줄
> 등록일: 2026년 10월 5일

생성 템플릿의 `MakeSerializedPacket`이 `static_cast<uint16>(pkt.ByteSizeLong())`로 본문 크기를 자르고, 헤더 4바이트를 더한
값도 `uint16`에 담는다. 이 템플릿에서 클라이언트의 `ClientPacketHandler.h`와 서버의 `ServerPacketHandler.h`가 생성된다.
크기가 넘치는지 검사하지 않는다.

### 영향

**버그 발생 가능성 증가** — 본문이 65,531바이트를 넘는 패킷은 헤더의 크기가 틀어져 받는 쪽의 패킷 경계가 깨진다.
지금은 그만큼 큰 패킷이 없지만, 몬스터와 플레이어가 많은 룸의 `S_SPAWN`이 커지면 닿는다.

## TD-024 클라이언트의 `GetStatValue`가 없는 스탯에서 중단된다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · client
> 위치: `P1/Source/P1/Game/Progress/P1MyPlayerData.cpp` 132~136줄
> 등록일: 2026년 10월 5일

`GetStatValue`가 protobuf `Map::at`으로 스탯을 읽는다. 키가 없으면 예외 대신 프로세스가 중단된다. HUD와 상태 창이
`NativeConstruct`에서 이 함수를 부른다.

### 영향

**버그 발생 가능성 증가** — 서버가 스탯 종류를 하나 빼거나 새 스탯을 표시하려고 할 때 클라이언트가 위젯을 만들다 죽는다.

## TD-025 클라이언트가 서버의 슬롯 번호를 범위 검사 없이 배열 인덱스로 쓴다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 모듈 · client
> 위치: `P1/Source/P1/Game/Inventory/P1Inventory.cpp` 15~67줄 · `P1/Source/P1/Game/Equipment/P1Equipment.h` 29줄
> 등록일: 2026년 10월 5일

- `UP1Inventory::Init`은 `slot_id`가 `[0, 칸 수)` 안이라고 가정하고 `GearLookup[Slot_->slot_id()]`에 넣는다
- `UP1Inventory::Rep_SlotChanged`는 범위를 보지 않고 `InvenLookup[Slot_.slot_id()]`를 역참조한다
- `UP1Equipment::EquippedGearLookup` 원시 포인터에 초기값이 없다. `Init` 전에 `GetAllSlot`이나 `Rep_SlotChanged`가
  불리면 쓰레기 값을 역참조한다

코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 서버가 칸 수를 바꾸거나 슬롯 번호를 잘못 보내면 클라이언트가 범위 밖 메모리를 읽고 죽는다.

## TD-026 몬스터 테이블이 비면 플레이어도 스폰되지 않는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · client
> 위치: `P1/Source/P1/Sync/P1EntitySpawner.cpp` 24~31줄 (`BeginPlay`)
> 등록일: 2026년 10월 5일

`AP1EntitySpawner::BeginPlay`는 `MonsterDataTable`이 비어 있으면 경고를 남기고 `RegisterSpawner` 전에 리턴한다.
`UP1StatefulEntityManager::HandleSpawn`은 항상 스포너 0번을 쓰므로, 스포너가 등록되지 않으면 몬스터뿐 아니라 내 플레이어와
다른 플레이어의 스폰도 모두 실패한다.

### 영향

**버그 발생 가능성 증가** — 맵에 스포너를 놓으면서 몬스터 테이블을 빠뜨리면 원인과 먼 증상(내 캐릭터가 나오지 않음)으로 드러난다.

## TD-027 게임 서버 접속이 게임 스레드를 블로킹한다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · client
> 위치: `P1/Source/P1/Network/P1ConnectionSubsystem.cpp` 55줄 (`Connect`)
> 등록일: 2026년 10월 5일

`UP1ConnectionSubsystem::Connect`가 게임 스레드에서 블로킹 소켓의 `Socket->Connect`를 부른다.

### 영향

**버그 발생 가능성 증가** — 게임 서버가 응답하지 않거나 주소가 틀리면 연결 시간 제한이 끝날 때까지 로그인 화면이 멈춘다.

## TD-028 클라이언트 네트워크 코드의 주석과 로그가 옛 구조를 가리킨다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 모듈 · client
> 위치: `P1/Source/P1/`
> 등록일: 2026년 10월 5일

| 위치 | 적힌 것 | 실제 |
|---|---|---|
| `Network/PacketSession.cpp` 106줄 | 소켓은 게임 인스턴스가 소유한다 | `UP1ConnectionSubsystem`이 소유한다 |
| `Network/P1SendWorker.cpp` 45줄 | 게임 인스턴스가 연결을 닫을 때 `C_LEAVE_GAME`이 나간다 | 연결 서브시스템의 `Deinitialize`가 닫는다 |
| `Network/P1ConnectionSubsystem.cpp` 129줄 | `PacketSession::GetGameInstance`를 쓴다 | 그 함수는 없다. `GetWorld`와 `GetConnection`을 쓴다 |
| `Game/Interaction/P1Shop.h` 30줄 | 범위에서 나간 액터가 누구든 상점 창을 닫는다 | `AP1MyPlayer`가 아니면 리턴한다. cpp의 주석은 코드와 맞다 |
| `Tests/P1PacketFramingTest.cpp` 8~9줄 | SendBuffer의 `Append`와 `Copy`는 `docs/tech-debt.md` 참조 | 그런 항목이 없다 |
| `Sync/P1StatefulEntityManager.cpp` 164~171줄, 197~204줄 | 중복 스폰을 경고 로그로 남긴다 | 첫 검사가 먼저 리턴해서 경고 로그는 실행되지 않는다 |
| `Sync/P1StatefulEntityManager.cpp` 71줄 | 엔티티 번호를 로그에 찍는다 | `uint64`를 `int32`로 잘라 `%d`로 찍는다 |

### 영향

**유지보수 어려움** — 주석을 따라가면 없는 함수나 다른 소유자를 찾게 된다.

## TD-029 몬스터와 다른 플레이어의 HP 막대가 피격으로 갱신되지 않는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 기능 · client
> 위치: `P1/Source/P1/Game/Entities/P1Creature.cpp` 87~90줄 (`S_Hit`) · `P1/Source/P1/UI/WorldSpace/P1NameplateWidget.cpp` 55줄
> 등록일: 2026년 10월 5일

`AP1Creature::S_Hit`은 `OnHit`을 알리기만 한다. C++에는 `OnHit`의 구독자가 없다. 네임플레이트는 `BindCreature`에서
`HpBar->Init(CurHp, MaxHp)`을 한 번 부르고 이후 HP를 갱신하지 않는다. `OnHit`은 `BlueprintAssignable`이라 블루프린트가
구독할 수 있지만, 위젯 블루프린트의 그래프는 확인하지 못했다. 코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 몬스터에게 맞은 다른 플레이어의 HP 막대가 그대로 남는다. 플레이어가 몬스터를 때리는 경로(`docs/backlog.md`
15번)가 생기면 몬스터의 HP 막대도 줄지 않는다.

## TD-030 테스트 문서가 테스트 프로젝트의 `main` 구성을 잘못 적고 있다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · build
> 위치: `docs/testing.md` 173줄
> 등록일: 2026년 10월 5일

`docs/testing.md` 173줄은 테스트 프로젝트가 `gtest-all.cc`와 `gtest_main.cc`를 직접 컴파일한다고 적는다.
`GameServerTests.vcxproj`는 `gtest_main.cc`를 쓰지 않고 `TestMain.cpp`가 `main`을 둔다(194~195줄 주석). 같은 문서의 188줄은
`TestMain.cpp`를 맞게 적고 있어서 문서 안에서도 어긋난다.

### 영향

**유지보수 어려움** — 문서를 따라 테스트 프로젝트를 고치면 `main`이 둘이 되어 링크가 실패한다.

## TD-031 ARCHITECTURE의 두 문장이 실제 동작과 다르다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 파일 · server
> 위치: `docs/ARCHITECTURE.md` 49~58줄, 192줄
> 등록일: 2026년 10월 5일

| 문장 | 실제 |
|---|---|
| 패킷 핸들러는 인라인으로 일하지 않고 `DoAsync`로 잡을 밀어넣고 리턴한다 | 핸들러의 본문은 일하지 않는다. 다만 `JobQueue::Push`는 큐가 비어 있고 그 스레드가 다른 큐를 실행 중이 아니면 그 자리에서 `Execute`한다. 대개 같은 IOCP 워커가 룸 잡을 실행한 뒤에 리턴한다 |
| 클라와 서버의 클래스 계층이 대칭이다. `Entity → Creature → { Player, Monster }` | 클라이언트에는 `Entity`에 해당하는 클래스가 없고 `AP1Creature : ACharacter`에서 시작한다 |

첫 문장은 직렬화를 설명하는 데는 맞다. 그러나 「핸들러 안에서 블로킹하면 IOCP 처리량이 줄어든다」를 판단할 때는 룸 잡의 비용도
IOCP 워커가 진다는 사실이 빠진다.

### 영향

**유지보수 어려움** — 룸 잡에 무거운 일을 넣어도 IOCP와 무관하다고 오해하게 된다. 클라이언트에 엔티티가 아닌 동기화 대상을 넣을
때 서버와 같은 계층이 있다고 가정하게 된다.

## TD-034 아이템 UID의 최댓값을 `int32`로 읽는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/GameServer/DB/ItemDAO.cpp` (`GetMaxItemUID`의 `BindObject`) · `Server/GameServer/Queries/GameDB_GetMaxItemUid.sql`
> 등록일: 2026년 10월 7일

`item_uid` 컬럼은 `BIGINT`이고 `GNextItemUID`도 `atomic<int64>`다. 그런데 `GetMaxItemUID`는 프로시저의 결과를
`int32 _maxItemUID`에 바인딩한다. `DBConnection::BindCol(int32)`은 `SQL_C_LONG`으로 묶는다. 최댓값이 `int32` 범위를 넘을 때
ODBC 드라이버가 값을 자르는지 `Fetch`를 실패시키는지는 확인하지 않았다. 코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 지금은 UID가 작아서 드러나지 않는다. 범위를 넘으면 값이 잘려서 새로 발급하는 UID가 기존 아이템과
겹치거나, `Fetch` 실패가 TD-017 「게임 서버가 DB를 준비하기 전에 접속을 받는다」의 잡히지 않는 예외로 이어져 서버가 시작하지 못한다.

## TD-035 DB에서 읽은 슬롯 번호를 검증 없이 인벤토리와 장비 칸에 쓴다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 기능 · server
> 위치: `Server/GameServer/Game/Inventory/InventoryComponent.cpp` (`AddItem`) · `Server/GameServer/Game/Equipment/EquipmentComponent.cpp` (`LoadEquipped`)
> 등록일: 2026년 10월 7일

`ItemDAO::LoadItems`는 DB 행의 `slot_id`를 두 함수에 넘긴다. 두 함수는 이 값을 그대로 믿는다.

- `InventoryComponent::AddItem`은 `setSlotId`의 범위(`[0, MAX_SLOTS)`)를 보지 않고 `bag->slots->Mutable`과 `dirtyFlags`의 인덱스로 쓴다.
  `Mutable`의 범위 검사는 `GOOGLE_DCHECK`라서 Release 빌드에서 빠진다
- 같은 함수는 그 칸에 이미 아이템이 있으면 템플릿이 같은지 보지 않고 수량을 더한다
- `EquipmentComponent::LoadEquipped`는 받은 `gearType`을 그대로 장착 부위로 쓴다. 아이템의 실제 부위와 같은지 보지 않는다.
  없는 부위면 칸을 찾지 못해 실패하므로 범위 밖 접근은 없다

코드를 읽고 판단했다. 잘못된 행을 넣어 재현하지는 않았다.

### 영향

**버그 발생 가능성 증가** — 수동 수정이나 마이그레이션 실수로 DB에 잘못된 슬롯이 들어가면, 그 캐릭터가 입장할 때 Debug 빌드는
중단되고 Release 빌드는 범위 밖 메모리를 쓴다. 범위 안이어도 다른 아이템이 합쳐지거나 무기가 다른 부위에 장착된 채로 저장된다.

## TD-036 쓰이지 않거나 이름과 다르게 동작하는 코드가 남아 있다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 모듈 · server
> 위치: `Server/GameServer/Utils/`, `Server/GameServer/Game/`
> 등록일: 2026년 10월 7일

| 위치 | 상태 |
|---|---|
| `Utils/Utils.h`의 `vector2D::operator-=` | lhs를 바꾸지 않고 차를 새로 돌려준다. `operator-`와 같다. 부르는 곳은 없다 |
| `Utils/TickIntervalTimer.h/.cpp` | 쓰는 곳이 없다. `Clear`가 `_repeatingCount`를 되돌리지 않아서, 한 번 발동한 타이머를 `doOnce`로 다시 `Init`하면 발동하지 않는다. `Clear`만 부르면 `_state`도 그대로 남는다 |
| `Game/Room/Room.h`의 `IsValid` | 읽는 곳이 없다 |
| `Game/Room/Room.cpp`의 `_monsterRespawnTime` | 룸 데이터에서 읽어 두기만 하고 쓰지 않는다 |
| `Game/Entities/Monster.h`의 `_attackTimer` | 생성자에서 할당하고 소멸자에서 해제할 뿐 쓰지 않는다(주석 「사용 안하는 중」) |

코드를 읽고 판단했다.

### 영향

**유지보수 어려움** · **버그 발생 가능성 증가** — 기능이 있는 것처럼 보여서 읽는 사람이 동작을 잘못 짐작한다.
`operator-=`나 `TickIntervalTimer`를 새로 쓰기 시작하면 그 자리에서 바로 버그가 된다.

## TD-041 세션의 계정 번호를 락 없이 여러 스레드가 읽고 쓴다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 기능 · server
> 위치: `Server/GameServer/Network/GameSessionManager.cpp` 12줄 · `Server/GameServer/Network/GameSession.cpp` 17줄 · `Server/GameServer/Network/ServerPacketHandler.cpp` (`Handle_C_CREATE_CHARACTER`, `Handle_C_DELETE_CHARACTER`, `Handle_C_ENTER_GAME`)
> 등록일: 2026년 10월 9일

`GameSession::_userId`는 원자적이지 않은 `int64`다. 로그인 잡(DB 스레드)이 관리자의 락 안에서 쓰고, IOCP 스레드의 핸들러와
`OnDisconnected`는 락 없이 읽는다. 계정 번호를 읽는 시점도 요청마다 다르다. 캐릭터 생성은 핸들러가 읽은 값을 잡에 넘기고,
삭제와 입장 불러오기의 DAO는 잡이 실행될 때 세션에서 다시 읽는다. 코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 정의되지 않은 동작인 데이터 경합이다. 로그인 직후의 요청이 0이나 찢어진 계정 번호로 처리될 수 있다.

## TD-043 몬스터의 일반 공격이 피격 시점에 거리를 다시 보지 않는다
> **심각도:** 낮음 · **난이도:** 낮음 · **범위:** 함수 · server
> 위치: `Server/GameServer/Game/Entities/Monster.cpp` 517줄 (`NormalAttack`) · `Server/GameServer/Game/Room/Room.cpp` (`HandleHit`)
> 등록일: 2026년 10월 9일

몬스터는 사거리 안에서 공격을 시작하고 200ms 뒤의 `Room::HandleHit`을 예약한다. `HandleHit`은 공격자가 룸에 남았는지만 보고
대상과의 거리를 다시 보지 않는다. 그래서 그 200ms 안에 사거리를 빠져나간 플레이어도 맞는다. 피격 시점에 거리를 다시 확인하기로
2026년 10월 9일에 정했다. 코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 플레이어가 공격을 피해도 맞는다. 클라이언트가 보여 주는 거리와 판정이 어긋난다.

## TD-001 룸 이동 요청이 플레이어의 위치를 보지 않는다
> **심각도:** 낮음 · **난이도:** 중간 · **범위:** 기능 · server
> 위치: `Server/GameServer/Game/Room/RoomTransfer.cpp` · `Server/GameServer/Game/Room/Room.cpp` (`C_HandleEnterMap`)
> 등록일: 2026년 10월 1일

포털 이동은 포털 번호가 현재 룸에 있는지만 보고, 플레이어가 그 포털 근처에 있는지는 보지 않는다. 맵 간 이동의
목적지는 `C_ENTER_MAP`이 받아 두는데, `C_HandleEnterMap`은 아무 룸 번호나 받는다. 코드를 읽고 판단했다.

고치기 전에 포털을 쓰는 지금 방식이 근본적으로 맞는지 함께 논의해야 한다. 클라이언트가 포털을 밟았다고
알리고 서버가 믿는 구조인데, 서버가 포털 반경을 판정하려면 포털 위치와 반경의 원천을 서버에 두어야 한다.
맵 간 이동이 `C_ENTER_MAP`과 `C_ENTER_ROOM` 두 요청으로 나뉜 것도 같은 논의에 들어간다.

### 영향

**버그 발생 가능성 증가** — 조작한 클라이언트는 룸 안 어디서든 포털을 타고, 아무 룸으로나 맵 간 이동을 할 수 있다.

## TD-002 상점 판매 목록과 상점 위치를 서버가 보지 않는다
> **심각도:** 낮음 · **난이도:** 중간 · **범위:** 기능 · server
> 위치: `Server/GameServer/Game/Entities/Player.cpp` (`ProcessBuyItem`, `ProcessSellItem`)
> 등록일: 2026년 10월 1일

게임 서버에는 상점이 파는 아이템의 목록이 없어서, 아이템 표에 있는 아이템은 모두 살 수 있다. 플레이어가 상점
근처에 있는지도 보지 않으므로 어디서나 사고팔 수 있다. 상점 판매 목록은 기획 데이터에 없어서 원천부터 정해야
한다. 위치 검증은 TD-001 「룸 이동 요청이 플레이어의 위치를 보지 않는다」와 같은 논의(클라이언트가 상호작용을 알리고
서버가 믿는 구조)에 들어간다. 코드를 읽고 판단했다.

### 영향

**버그 발생 가능성 증가** — 조작한 클라이언트는 상점에 없는 아이템을 사고, 상점에 가지 않고 거래할 수 있다.

## TD-003 저장 대기가 덮지 못하는 틈 두 곳에서 저장 전의 진행을 불러올 수 있다
> **심각도:** 낮음 · **난이도:** 중간 · **범위:** 기능 · server
> 위치: `Server/GameServer/Network/GameSession.cpp` (`OnDisconnected`, `LeaveGame`) · `Server/GameServer/Network/ServerPacketHandler.cpp` (`Handle_C_ENTER_GAME`)
> 등록일: 2026년 10월 4일

`SaveGate`는 접속 종료 저장이 끝나기 전의 입장 불러오기를 막는다. 다만 아래 두 경우에는 대기 없이 불러온다.
설계할 때 드물다고 보고 받아들였다. 코드를 읽고 판단했고 실행해서 확인하지는 않았다.

| 틈 | 무슨 일이 일어나는가 |
|---|---|
| 룸 입장 잡이 큐에 있는 동안 끊긴다 | 끊길 때 룸이 없어 `OnDisconnected`는 대기를 걸지 않는다. 그 룸 잡이 돌아 `LeaveGame`이 대기를 걸기 전에 새 세션의 불러오기가 `userId` 큐에 들어가면 먼저 불러온다 |
| 5초 상한이 지난 뒤 저장이 도착한다 | 상한이 지나면 입장을 거절하고 대기를 지운다. 그 뒤의 입장이 늦게 도착한 저장보다 먼저 불러온다 |

### 영향

**버그 발생 가능성 증가** — 밀려나거나 끊긴 세션의 마지막 진행이 사라지고, 새 세션이 끊길 때 낡은 진행으로 덮어쓴다.

## TD-033 클라이언트가 맵 간 이동을 요청하지 않아 `S_ENTER_MAP` 경로가 실행되지 않는다
> **심각도:** 낮음 · **난이도:** 중간 · **범위:** 기능 · client
> 위치: `P1/Source/P1/Core/P1GameInstance.cpp` 42줄 (`OpenInGameMap`) · `P1/Source/P1/Network/ClientPacketHandler.cpp` 60~67줄 (`Handle_S_ENTER_MAP`)
> 등록일: 2026년 10월 5일

클라이언트 코드는 생성물 말고는 `C_ENTER_MAP`을 보내는 곳이 없다. 서버의 `S_ENTER_MAP`은 `Handle_C_ENTER_MAP`의 응답이므로,
`Handle_S_ENTER_MAP` → `UP1MyPlayerData::HandleEnterMap` → `OnMapEntered` → `OpenInGameMap` 경로는 지금 실행되지 않는다.
`OpenInGameMap`도 맵 번호와 무관하게 `"L_InGameMap"`을 연다(주석 「임시」). 첫 입장은 `LoadLastState`가 `_enteringRoomId`를
채워 두므로 동작한다. 코드를 읽고 판단했다.

### 영향

**새 기능 개발 지연** — 맵이 둘 이상이 되면 클라이언트의 맵 간 이동을 처음부터 이어야 한다. 실행되지 않는 핸들러가 동작하는
경로처럼 보여서, 그 경로를 고친 결과를 확인할 수 없다.

## TD-040 아이템 배열 저장이 일부 행의 실패를 성공으로 본다
> **심각도:** 낮음 · **난이도:** 중간 · **범위:** 함수 · server
> 위치: `Server/GameServer/DB/ItemDAO.cpp` 412~419줄, 529~536줄 (`SaveGearItems`, `SaveStackableItems`)
> 등록일: 2026년 10월 9일

배열 파라미터로 여러 행을 한 번에 저장하면서 `Execute`의 반환값만 본다. 처리된 행 수(`SQL_ATTR_PARAMS_PROCESSED_PTR`)나 행별
상태 배열을 바인딩하지 않는다. `DAOCommon.h`의 `SQL_MISMATCHED_PROCESSED_PARAMSET_SIZE`는 이 경우를 잡으려고 정의한 것으로
보이지만 쓰는 곳이 없다. ODBC 명세상 일부 파라미터 집합이 실패하면 `SQLExecute`가 `SQL_SUCCESS_WITH_INFO`를 돌려주는데,
`DBConnection::Execute`는 이를 성공으로 본다. 드라이버가 실제로 이렇게 동작하는지는 확인하지 않았다. 추측이다.

### 영향

**버그 발생 가능성 증가** — 아이템 행의 일부만 저장되고 저장은 성공으로 끝나서, 다음 입장 때 아이템이 사라진 원인을 찾을 수 없다.
