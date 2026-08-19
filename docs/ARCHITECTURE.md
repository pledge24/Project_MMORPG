# ARCHITECTURE (as-is)

/init 생성 내용을 이관한 초안. 세션 1에서 실제 코드와 대조·검증하고 보강한다.

## 로그인은 3티어를 모두 거친다

1. 클라의 `ULoginManager`(`P1/Source/P1/Login/LoginManager.h`)가 인증 서버로 HTTP POST.
2. `Server/AuthServer/src/routes/login.router.js`가 `UserDB`의 bcrypt 해시를 검증하고, UUID 액세스
   토큰을 발급해 `accessToken:<uuid>` → `{userId, username}`을 TTL과 함께 Redis에 저장.
3. 클라는 토큰을 `UP1GameInstance`에 보관하고 `127.0.0.1:7777`로 TCP 소켓을 열어
   `C_LOGIN { access_token }`을 전송.
4. `Handle_C_LOGIN`(`Server/GameServer/ServerPacketHandler.cpp`)이 Redis에서 토큰을 되읽고,
   맞으면 `GameDB`에서 캐릭터를 로드.

즉 인증 티어와 게임 티어를 잇는 건 Redis뿐이다. 게임 서버는 `UserDB`를 직접 건드리지 않는다.

## 게임 서버 스레딩

`Server/GameServer/GameServer.cpp`의 `main()`은 패킷 핸들러 테이블 초기화 → 게임 데이터 로드 →
맵 템플릿마다 `Room` 생성 → 리스너 시작 순으로 부팅한 뒤, 워커 스레드 5개와 DB 스레드 5개를 띄운다
(메인 스레드가 6번째 워커가 된다).

각 워커는 64ms 틱 예산으로 `IocpCore::Dispatch(10)` → `ThreadManager::DistributeReservedJobs()` →
`ThreadManager::DoGlobalQueueWork()`를 반복한다. 따라서 패킷 핸들러는 IOCP 워커 스레드에서 돈다.

동시성 모델: `Room`이 `JobQueue`를 상속한다. 핸들러는 인라인으로 일을 거의 하지 않고 잡만
밀어넣고 리턴한다. 예: `room->DoAsync(&Room::C_HandleMove, pkt)`. 룸/오브젝트 상태 변경은 전부 해당
룸의 큐에서 직렬화되므로, 큐 위에 머무는 한 룸 소유 상태에는 락이 필요 없다. DB 작업도 같은 식으로
분리한다 — 핸들러가 `DBQueue`에 `Job`을 push하고(유저 친화도가 필요하면
`GDBManager->GetDBQueueFromId(userId)`, 아니면 랜덤) 전용 DB 스레드가 소비한다.

`Room`은 공간 분할용 셀 행렬(`CELL_SIZE = 1000.f`)도 유지하며 `FindClosestPlayer`와 브로드캐스트
범위 계산에 쓴다.

## 클라이언트 스레딩

`PacketSession`(`P1/Source/P1/Network/PacketSession.h`)이 `RecvWorker`와 `SendWorker`를 소유하고,
둘 다 `FRunnable` 스레드다. 수신 바이트는 `TQueue`에 쌓이고 게임 스레드의
`UP1GameInstance::HandleRecvPackets()`가 이를 비운다. 이 함수는 `BlueprintCallable`이고 블루프린트에서
호출된다 — 네트워크 스레드는 UObject를 절대 만지지 않는다. `UP1GameInstance`가 허브 역할로 소켓과
세션을 소유하고 모든 `HandleXxx(const Protocol::S_XXX&)` 핸들러를 구현하며, 게임플레이 액터와 UMG
위젯으로 멀티캐스트 델리게이트(`OnRecvBuyItemPkt` 등)를 통해 전파한다.

## 클라/서버 클래스 계층이 대칭이다

양쪽 모두 `Object → Creature → { Player, Monster }` 형태를 공유한다.

- 서버: `Server/GameServer/{Object,Creature,Player,Monster}.h`
- 클라: `P1/Source/P1/Game/Objects/{Creature,P1Player,P1MyPlayer,Monster}.h`

서버 오브젝트는 상태를 protobuf 메시지(`Protocol::ObjectInfo`, `PosInfo`, `StatInfo`)로 직접 들고
있어서 복제가 변환이 아니라 복사다. 게임플레이를 바꾸면 보통 양쪽을 대칭으로 고치고 프로토콜도
같이 손대야 한다 — 3곳 수정을 기본으로 생각할 것.

## 미작성 (세션 1에서 채울 것)

- UserDB / GameDB 스키마 (주요 테이블과 용도)
- 클라 폴더 구조 현황 (레이어 분리 관점)
- 프로토콜 메시지 목록과 소유 핸들러 매핑
