# 테스트 계층

실행 경로와 그 근거를 함께 적는다.

---

## 지금 도는 것

| 대상 | 빌드 | 실행 (에이전트·CI) | 실행 (사람·IDE) |
|---|---|---|---|
| 게임 서버 L1 (GoogleTest) | `MSBuild Server.sln` (`docs/build.md` 「빌드 명령」) | `Server/Binary/Debug/GameServerTests.exe` | Rider 실행 구성 `GameServerTests` |
| 인증 서버 | — | `cd Server/AuthServer && npm test` | Rider npm 구성 |
| 인증 서버 정적 검사 | — | `cd Server/AuthServer && npm run lint` | Rider npm 구성 |
| UE 클라 L2 (Automation) | `Build.bat P1Editor` (`docs/build.md` 「빌드 명령」) | `pwsh P1/Scripts/Run-UeTests.ps1` | 에디터 `Window > Test Automation` |
| 규범 검사 | — | `py -3 Tools/ConventionLint/check_conventions.py` | 같은 명령 |

**판정은 종료 코드다.** 0이 아니면 실패다. 인증 서버는 `npm test`와 `npm run lint`가 둘 다 0이어야 완료다.

**UE 클라만 예외다.** `UnrealEditor-Cmd`는 테스트가 실패해도, 필터가 아무것도 맞추지 못해도 종료
코드 `0`을 돌려준다(2026년 9월 17일 실측, 두 경우 모두 확인). 그래서 `Run-UeTests.ps1`이 리포트의
`index.json`을 읽어 판정하고 자기 종료 코드를 낸다. **에디터의 종료 코드를 보지 않는다.**
실행 전에 지난 `index.json`을 지우므로 이전 결과가 초록으로 읽히지 않는다.

UE 테스트는 에디터를 띄우지 않고 돈다. 다만 **빌드에는 에디터를 닫아야 한다**
(`docs/build.md` 「빌드 명령」). 여기가 이 계층의 유일한 사람 손이다.

**규범 검사는 자기 검증을 먼저 돌린다.**
— 위반 0건과 대상 0건은 출력이 같다. 검사가 대상을 하나도 찾지 못하면 아무것도 검사하지 않으면서
초록을 낸다. `--self-test`가 일부러 어긋낸 입력을 다섯 검사에 먹여 위반이 실제로 잡히는지 보고,
규범을 지키는 입력에서는 잡지 않는지도 함께 본다. CI의 「규범 검사」 잡이 이 순서를 그대로 쓴다.

```
py -3 Tools/ConventionLint/check_conventions.py --self-test
py -3 Tools/ConventionLint/check_conventions.py
```

검사 대상은 git이 추적하는 파일뿐이다. 빌드도 엔진도 필요 없으므로 호스티드 러너에서 그대로
돈다. 검사 항목과 그 근거 조항은 스크립트 첫머리의 표에 있다.

테스트는 `Server/GameServerTests/`, gtest는 `Server/Libraries/googletest/`에 벤더링돼 있다(v1.18.0, gmock 없음). 인증 서버는 Node 내장 러너(`node --test`)라 새 의존성이 없다.

정적 검사는 ESLint 9다. flat config(`Server/AuthServer/eslint.config.js`)가 `@eslint/js`의 recommended를 적용한다. `node_modules/`는 검사하지 않는다.

### 현재 커버리지

| 스위트 | 개수 | 대상 |
|---|---|---|
| `PacketSerialization` | 4 | S_CHAT 가변 문자열 · S_MOVE 중첩 메시지 protobuf 왕복 · 헤더 크기 상한의 본문은 직렬화되고 상한을 넘는 본문은 만들지 않음 |
| `ProtocolContract` | 3 | `PROTOCOL_MESSAGES(X)` 목록 대조 · 패킷 ID 연속·유일성 · 전 메시지 리플렉션 왕복 |
| `PacketDispatch` | 2 | 핸들러 테이블 밖의 id(65535)를 디스패치하지 않고 거절 · 헤더보다 짧은 입력(0~3바이트)을 디스패치하지 않고 거절 |
| `InventoryTest` | 12 | 슬롯 타입 교차오염 · 더티 플래그 순서 · 실패한 remove 후 슬롯 재사용 · 알 수 없는 슬롯 타입과 범위 밖 슬롯 번호 거부 · 매핑 표 키 집합과 기대 집합 대조 |
| `AllSlotTypes/InventorySlotTypeTest` | 6 | 슬롯 추가·제거 왕복 전 타입 (TEST_P 2 × Gear/Consumable/Misc) |
| `SessionDisconnectTest` | 1 | 서버가 건 끊기. 받기만 하고 소켓을 닫지 않는 루프백 상대도 상한 안에 `OnDisconnected`까지 가고 세션 집합에서 빠짐 |
| `SaveGateTest` | 9 | 접속 종료 저장 대기. 대기 없는 계정은 맡지 않음 · 해제가 맡긴 불러오기를 돌려줌 · 두 번째 맡기기 거절 · 두 번 걸어도 한 번에 풀림 · 만료가 입장만 돌려주고 대기는 남김 · 해제 뒤 늦게 온 만료와 지난 토큰의 만료 무시 · 계정별 분리 |
| `ProgressCoordinatorTest` | 14 | 저장 대기와 입장 불러오기의 순서. 대기 없는 입장은 곧바로 불러옴 · 접속 종료 중 재입장은 저장 뒤에 불러옴 · 중복 로그인은 밀려난 세션의 저장을 기다림 · 대기 만료는 입장 거절 · 기다리는 중의 두 번째 입장 거절 · 실패한 저장도 대기를 풂 · 룸에 들어간 적 없는 플레이어는 저장하지 않음 · 룸 이동 중 끊긴 플레이어는 들어갈 룸이 저장하고, 들어가지 못해도 저장 · 첫 룸 입장 전에 끊긴 플레이어는 재입장 뒤에 저장되지 않음 · 만료 뒤의 입장도 저장을 기다림 · 끊긴 세션을 늦게 밀어내도 입장이 막히지 않음(저장 전, 저장 뒤, 룸 입장 전) |
| `AccessToken` | 4 | Redis 토큰 값 해석. 계정 번호와 이름을 읽음 · 형식이 틀린 값, 빠진 키, 틀린 타입은 예외 없이 거절 |
| `CellMatrixTest` | 7 | 근접 탐색 격자. 겹치는 칸의 엔티티만 반환 · 칸 경계는 위 칸 소속 · 격자 끝과 밖의 엔티티 · 가장자리 질의 보정 · 제거와 재구성 |
| `CharacterListDAOTest` | 4 | 캐릭터 생성 DAO(가짜 연결). 만든 캐릭터 번호 · 거절은 예외가 아니라 결과 · 쿼리가 모르는 거절 사유와 실패한 쿼리는 DBError |
| `CharacterCreationTest` | 7 | 캐릭터 생성 검증. 유효한 전사 · 레벨 표가 없거나 정의되지 않은 직업 거절 · 레벨 표가 있어도 NONE 거절 · 빈 이름 거절 · 이름 길이 경계(한글 50자 통과, 51자 거절) |
| `CombatTest` | 8 | 피격과 처치 판정. 데미지만큼 HP 감소 · 과잉 데미지의 HP 0 고정과 사망 · 죽은 대상과 죽은 공격자 · 플레이어가 몬스터를 처치할 때만 보상 · 피격 후 몬스터 정보의 HP |
| `DBConnectionGuardTest` | 1 | 빈 연결 풀에서 빌리면 널 연결 대신 DBError |
| `DBWorkerTest` | 6 | DB 스레드. 끝까지 돈 잡 · 표준 예외와 DBError와 표준 예외가 아닌 값이 잡 밖으로 나가지 않음 · 멈춘 큐의 남은 잡은 돌리고 새 잡은 버림 |
| `EntityFactoryTest` | 4 | 엔티티 생성. 몬스터의 id·타입·템플릿·스폰 위치와 위치의 엔티티 id · 모르는 템플릿 거절 · 세션 없는 플레이어의 id·인벤토리 · 엔티티마다 다른 id |
| `GameEntryTest` | 9 | 게임 입장. 검증을 통과한 진행만 세션에 등록 · 불러온 슬롯이 그 번호에 들어가고 바뀐 슬롯으로 표시되지 않음 · 검증 실패와 레벨 표에 없는 레벨은 세션을 비워 둠 · 두 번째 입장 거절 · 같은 칸의 두 행, 다른 부위의 장비, 다른 종류의 표에 든 아이템, 범위 밖 칸 번호 거절 |
| `GameSessionManagerTest` | 6 | 계정별 세션 등록. 같은 계정의 새 로그인이 밀어낸 세션 반환 · 밀려난 세션 제거가 새 등록을 지우지 않음 · 다른 계정끼리 밀어내지 않음 |
| `GamedataParserTest` | 19 | 기획표 검증과 변환. 행이 템플릿으로 바뀜(아이템 종류, 장비 부위, 착용 조건, 재사용 대기 ms, 최대 레벨, 마을, 맵 번호, 포털 출발 위치와 반경) · 틀린 행의 파일·행 번호·필드를 알림(누락, 타입, 중첩 필드, 아이템 종류가 아닌 itemType, 부위 없는 장비, 직업이 아닌 착용 조건, maxStack, 보상 최솟값, 중복 templateId, 끊긴 레벨, 0 이하의 포털 반경) · 마을이 하나가 아님 · 없는 포털 목적지와 스폰 몬스터 · 실패한 불러오기가 이전 표를 남김 · 저장소의 실제 기획표 통과 |
| `GearEquipTest` | 8 | 장비 착용과 해제가 알리는 장비 종류 · 착용과 해제가 다시 계산해 싣는 스탯 · HP가 가득 찬 채 벗은 뒤의 저장 사본이 다음 입장을 통과 · 요구 레벨과 요구 직업 · DB에서 불러온 장착 장비가 스탯을 건드리지 않음 |
| `JobTimerTest` | 2 | 예약 잡 분배. 만기 잡을 실행하지 않고 글로벌 큐로 넘김 · 만기 전 잡은 남김 |
| `ItemDAOTest` | 1 | 아이템 UID 최댓값을 int64로 읽음(가짜 연결) |
| `ItemSaveRowsTest` | 8 | 저장할 아이템 행 생성. 더티 칸만 행이 됨 · 비운 칸은 템플릿 0 · 기타 칸의 플래그 · 장비는 쌓이지 않음 · 착용 여부 · 플래그 누락은 실패 |
| `LoggerTest` | 5 | 로그 한 줄의 형식. 시각·레벨·스레드 표시와 밀리초 세 자리 · 레벨마다 다섯 칸 표시 · 쓰는 스레드의 id와 줄바꿈 · 여러 스레드가 동시에 써도 줄이 섞이지 않음 |
| `MonsterTest` | 2 | 몬스터 초기화의 HP · 최소와 최대가 같은 보상 |
| `MonsterAITest` | 4 | 룸 틱으로 도는 몬스터 AI. 상태 전환 판정은 0.2초가 쌓인 뒤 · 룸을 떠난 대상을 다음 틱에서 놓음 · 사거리 안에 남은 대상은 맞고 피격 전에 사거리를 벗어난 대상은 맞지 않음 |
| `PacketHandlerTest` | 5 | 패킷 핸들러의 세션 상태 검사. 처리에 실패한 패킷 id를 로그에 남김 · 삭제 잡이 핸들러가 읽은 계정 번호로 일함 · 로그인하지 않은 세션의 캐릭터 생성, 삭제, 입장은 DB 큐에 넣지 않고 실패 응답 |
| `ParamSetTest` | 5 | 배열 파라미터 저장(가짜 연결). 행이 없으면 실행하지 않음 · 한 번에 실행 · 상한을 넘으면 실행하지 않고 DBError · 일부 행만 실패한 실행과 실행 실패는 DBError |
| `PlayerItemRequestTest` | 11 | 구매·판매·착용·해제 요청 검증. 모르는 템플릿 · 위조한 템플릿과 uid · 판매 불가 아이템 · 빈 칸 착용 · 가득 찬 인벤토리로 해제 |
| `PlayerLevelTest` | 7 | 레벨 상승. 최대 레벨은 레벨 표의 마지막 레벨 · 레벨 표가 없는 직업은 최대 레벨 · 최대 레벨에서 멈춤 · 최대 경험치 미만 누적 · 최대 경험치 표가 없을 때 · 최대 레벨의 보상 경험치 버림 |
| `PlayerMultiLevelUpTest` | 2 | 큰 보상의 여러 레벨 상승 · 최대 레벨 도달 시 남는 경험치 버림 |
| `PlayerSaveDataTest` | 2 | 저장 스냅샷이 이후 변경을 따라가지 않음 · 더티 플래그를 싣음 |
| `PlayerUseItemTest` | 11 | 소모품 사용. HP·MP 회복과 최대치 고정 · 요청과 칸이 어긋나면 거절 · 장비 칸, 빈 칸, 모르는 템플릿, 죽은 플레이어 거절 · 템플릿별 재사용 대기 |
| `ProgressStorageTest` | 6 | 진행 저장소와 DAO(가짜 연결). 캐릭터 기본 정보 행과 계정 대조 파라미터 · 다른 계정의 캐릭터 · 불러오기 실패 뒤 빈 세션 · 저장 하나가 트랜잭션 하나 · 아이템 저장 실패와 DBError가 아닌 예외의 되돌림 |
| `RandomTest` | 2 | 정수 범위 난수의 같은 경계와 최대값 포함 |
| `RoomAxisTest` | 1 | 룸의 X 범위는 깊이, Y 범위는 폭으로 계산 |
| `RoomLocationTest` | 2 | 룸 안 무작위 위치가 여백 안에 머묾 · 여백이 없으면 룸 전체 |
| `RoomRequestTest` | 7 | 룸 큐의 요청 처리. 이동은 보낸 사람의 위치만 바꿈 · 허용 거리를 넘는 이동은 버림 · 처음 입장한 플레이어는 자기 스폰을 한 번 받음 · 룸을 떠난 플레이어의 아이템 요청 무시 · 세션이 사라진 플레이어의 착용과 해제는 응답 없이 끝남 · 사망한 채 끊긴 플레이어는 마을 리스폰 상태로 저장 · 첫 입장 전 다른 룸으로의 맵 입장 거절 |
| `MoveValidation` | 5 | 이동 위치 판정. 허용 거리 안은 통과하고 넘으면 거절 · 오래 서 있어도 경과 시간은 상한에서 자름 · 평면 거리로 잼 · 룸 경계 밖 거절(경계 위는 안쪽) |
| `RoomTransferTest` | 12 | 룸 이동 판정. 첫 입장 · 다른 맵의 다른 룸 거절 · 같은 맵 이동에 포털 필요 · 포털 목적지 · 사망한 플레이어의 마을 리스폰만 허용 · 맵 표에서 찾는 마을 리스폰 지점 |
| `RoomTransferMapTest` | 6 | 위치를 보는 룸 이동 판정. 포털 반경 경계와 높이 무시 · 반경 밖 포털 이동 거절 · 첫 맵 입장은 불러온 룸으로만 · 맵 간 이동은 다른 맵으로 가는 포털 반경 안에서만 · 맵 번호 대조 |
| `ServerConfigTest` | 7 | 서버 설정 로더. 환경 변수가 없을 때 기본값 · 환경 변수가 각 값을 덮음 · 연결 수의 기본값은 DB 스레드 수를 따라감 · 연결 수가 DB 스레드 수보다 작으면 거절 · IPv4가 아닌 바인드 주소 거절 · 잘못된 개수와 포트는 기본값 |
| AuthServer `configs.test.js` | 2 | `.env` 필수 키 존재 · 커넥션 풀 크기 파싱 |
| AuthServer `redis.test.js` | 1 | Redis 주소 설정이 클라이언트의 `socket` 옵션으로 넘어감(Redis에 붙지 않고 옵션만 읽음) |
| `P1.Network.PacketFraming` | 1 | 패킷 헤더의 size·id 배치 · 본문 왕복 · 빈 메시지 경계 |
| `P1.Sync.MoveCorrection` | 1 | 원격 크리처 보정의 순간이동 경계(800) · 정지 중 접근 · 이동 중 수선의 발 접근 · Z 유지 · 회전 보정 켜고 끄기 · ACTION 중 보정 멈춤과 순간이동 |
| `P1.Sync.MoveSendThrottle` | 1 | 내 플레이어 이동 패킷의 주기 송신(0.2초)과 타이머 리셋 · 입력 변화 즉시 송신(이동 가능할 때만) · 회전 허용치(60도) 경계와 ±180도 감싸기 · 입력이 없을 때 서버와 마지막으로 맞춘 yaw와의 비교 · 공격 중 즉시 송신 억제 |
| `P1.Inventory.SlotAction` | 1 | 인벤토리 칸 더블클릭의 요청 판정. 빈 칸 · 소모품 사용 · 무기와 방어구 착용 · 요구 레벨 경계 · 기타 칸과 착용 장비 칸 · 재사용 대기 중인 소모품 |
| `P1.Inventory.ItemCooldown` | 1 | 아이템 재사용 대기의 남은 시간과 남은 비율(1 → 0) · 끝나는 순간의 경계 · 길이가 0 이하인 아이템 |
| `P1.Combat.NormalAttackCombo` | 1 | 일반 공격의 콤보 순번 순환(1→N→1) · 몽타주가 하나이거나 없을 때 · 순번 N의 몽타주 인덱스 · 서버가 보낸 순번 0 · 범위 밖 순번 |
| `P1.Progress.RewardResult` | 1 | 보상 결과의 반영. 경험치만 쌓일 때 레벨을 알리지 않음 · 여러 레벨 상승의 레벨, 레벨업 스탯, 최대 경험치 · 경험치를 알릴 때 최대 경험치가 이미 새 값 · 골드를 사본에 쓴 뒤 알림 |

**안 덮는 것**: `Room` 본체의 대부분(입장 일부, 이동, 아이템 착용과 해제, 접속 종료 저장, 맵 입장만 `RoomRequestTest`가, 룸 틱과 몬스터 피격만 `MonsterAITest`가 덮는다) · DAO의 SQL 문장과 ODBC 드라이버 동작(바인딩과 흐름은 가짜 연결로 덮는다) · IOCP · 기획표 파일 읽기(`Gamedata::LoadAllGamedata`) · AuthServer 라우터/인증 흐름. 전부 0개. UE 클라는 패킷 프레이밍, 이동 보정 계산, 이동 패킷 송신 판정, 인벤토리 칸 요청 판정, 아이템 재사용 대기 계산, 일반 공격 콤보 순번, 보상 결과 반영 일곱뿐이고 나머지 계층은 0개다.

---

## TDD가 도는 범위

| 대상 | 러너 | 루프 |
|---|---|---|
| GameServer 순수 로직 | `GameServerTests.exe`, 종료 코드 | 가능 |
| AuthServer 설정 | `npm test` | 가능. 가장 빠름 |
| GameServer DAO의 바인딩과 흐름 | `GameServerTests.exe`의 `FakeDBConnection` | 가능. SQL 문장과 드라이버 동작은 확인하지 못한다 |
| GameServer Room·SQL·IOCP | 없음 | **불가.** JobQueue 비동기, 실제 DB 필요 |
| AuthServer 라우터·인증 | 없음 | 가능하나 비쌈 (bcrypt+MSSQL+Redis) |
| UE 클라 순수 로직 | `Run-UeTests.ps1`, 종료 코드 | 가능. 다만 한 바퀴마다 에디터를 닫고 빌드해야 한다 |
| UE 클라 액터·월드 의존 로직 | 없음 | **불가.** 월드를 띄우는 테스트를 아직 써 보지 않았다 |

불가 영역은 지금 마땅한 테스트가 없다.

---

## seam

관찰 가능한 공개 경계. 여기서 테스트하면 내부를 전부 다시 써도 동작이 살아남는다.

**합의하지 않은 seam에는 테스트를 쓰지 않는다.** 사람이 먼저 승인한다.

| seam | 상태 |
|---|---|
| `InventoryComponent` 공개 API | 존재 |
| protobuf 메시지 왕복 | 존재. 리플렉션으로 자동 확장 |
| 프로토콜 ID 목록 | 존재 |
| 패킷 디스패치 | 존재. `ServerPacketHandler::HandlePacket`. 서버 생성물만 테스트한다. 클라이언트와 DummyClient의 헤더는 같은 템플릿에서 생성되지만 테스트하지 않는다 |
| 클라 패킷 프레이밍 | 존재. `ClientPacketHandler::MakeSerializedPacket`의 공개 오버로드 |
| 원격 크리처 이동 보정 | 존재. `FP1MoveCorrection::Compute`. 이동 상태 분기와 크리처 종류 판정은 `UP1MoveSyncComponent::TickRemote`에 있다 |
| 내 플레이어 이동 패킷 송신 판정 | 존재. `FP1MoveSendThrottle::Decide`. 이동 상태 판정과 패킷 구성은 `UP1MoveSyncComponent::TickMyPlayer`에 있다 |
| 인벤토리 칸 더블클릭의 요청 판정 | 존재. `FP1InventorySlotAction::Decide`. 응답 대기와 패킷 구성은 `UP1InventoryWidget::HandleSlotDoubleClicked`에 있다 |
| 아이템 재사용 대기 계산 | 존재. `FP1ItemCooldown`. 템플릿별 대기의 보관과 시작은 `UP1MyPlayerData`에, 막대 갱신은 `UP1SlotWidget::RefreshCooldown`에 있다 |
| 일반 공격 콤보 순번과 몽타주 선택 | 존재. `FP1NormalAttackCombo`. 몽타주 재생, 입력 가능 상태, 2초 초기화 타이머는 `UP1AttackSystemComponent`에 있다 |
| 보상 결과 반영 | 존재. `UP1MyPlayerData::HandleRewardResult`. 경험치와 레벨은 서버가 계산하므로 클라이언트는 사본에 쓰고 알리기만 한다 |
| 접속 종료 저장 대기 | 존재. `SaveGate` 공개 API |
| 진행 조율 | 존재. `ProgressCoordinator`의 진입점 네 개(`OnDisconnected`, `OnDuplicateLogin`, `RequestEnter`, `LeaveRoomAndSave`). 테스트는 DB 큐, 진행 저장소, 만료 타이머를 주입해 잡을 쌓아 두고 순서를 정해 돌린다. 세션 끊기와 밀어내기, 응답 전송은 `GameSession`과 `ServerPacketHandler`에 있고 테스트하지 않는다 |
| 전투 판정 | 존재. `Combat`(`Game/Combat/`). 룸 대조와 패킷 전송은 `Room`에 있다 |
| 룸 틱 | 존재. `Room::Tick(deltaTime)`. 테스트는 타이머를 기다리지 않고 시간을 넘긴다. 틱 예약(`RunScheduledTick`)은 테스트하지 않는다 |
| 몬스터 AI | 존재. `MonsterAIComponent`의 `GetState`와 `GetTarget`, 그리고 룸 틱 뒤의 엔티티 상태. 몬스터는 `Room::SpawnEntity`로 넣는다 |
| 이동 위치 판정 | 존재. `MoveValidation::Validate`. 기준 시각과 위치의 보관, 패킷 버리기는 `Room::C_HandleMove`와 `Player`에 있다 |
| 룸 이동 판정 | 존재. `RoomTransfer`의 자유 함수. 퇴장과 입장, 목적지 큐로 넘기기는 `Room`에 있다 |
| 기획표 검증과 변환 | 존재. `GamedataParser::Parse`와 `Gamedata::Load`. 파일 읽기는 `Gamedata::LoadAllGamedata`에 있고 테스트하지 않는다 |
| 기획 데이터 주입 | 존재. `Gamedata::Install`. 테스트는 전역 표를 직접 고치지 않고 템플릿을 채운 `GamedataTables`를 설치한다 |
| DB 연결 | 존재. `DBConnection`의 가상 함수. 테스트는 `GameServerTests/FakeDBConnection.h`를 DAO와 `ProgressStorage`에 넘긴다. 결과 행과 실행 실패를 미리 넣고, 바인딩한 파라미터와 트랜잭션 호출을 기록한다 |
| 게임 입장 | 존재. `GameEntry::Enter`와 `GameEntry::SpawnPlayer`. 응답 전송은 `ServerPacketHandler`에 있고 테스트하지 않는다. 저장 대기는 「진행 조율」이 덮는다 |
| 액세스 토큰 값 | 존재. `AccessToken::ParsePayload`. Redis 읽기와 지우기, 밀어내기는 `Handle_C_LOGIN`에 있고 테스트하지 않는다 |
| 세션 끊기 | 존재. 루프백(127.0.0.1)의 고정 포트 47913에 `ServerService`를 띄우고 Winsock 소켓으로 접속한다. 테스트가 IOCP 디스패치, 예약 잡 분배, 글로벌 큐 소비를 직접 돈다. 그 포트를 다른 프로세스가 쓰고 있으면 준비 단계에서 실패한다 |
| 예약 잡 분배 | 존재. `JobTimer`의 `Reserve`와 `Distribute`, `JobQueue::Execute`. 테스트는 지역 타이머와 큐를 만들고 전역 `GGlobalQueue`에서 넘겨진 큐를 꺼내 돌린다. 워커 루프(`DistributeReservedJobs`, `DoGlobalQueueWork`)는 테스트하지 않는다 |
| DB 잡 실행 | 존재. `DBWorker::RunJob`과 `DBWorker::Run`. 스레드를 띄우는 일은 `main`에 있다 |
| 세션 송수신 | 존재. `Session::Send`가 가상이다. 테스트는 `GameServerTests/RecordingSession.h`로 보낸 패킷을 기록하고, `Receive`로 받은 패킷을 흉내 낸다 |
| 플레이어 상태 준비 | 존재. `GameServerTests/PlayerTestAccess.h`(Player의 friend). 테스트가 레벨, 직업, 골드, 계정 번호, 소지품(인벤토리와 장비 컴포넌트)을 직접 바꾸는 길은 이것 하나다. Player는 컴포넌트를 읽기 전용으로만 연다 |

seam이 없으면 만드는 작업이 선행된다. 그것은 리팩토링이므로 별도 계획을 세운다.

---

## 빌드도 실행도 셸에서 한다

**빌드는 터미널에서 돌리고 종료 코드로 판정한다. 빌드에 Rider MCP를 쓰지 않는다.** 두 티어의 명령은 `docs/build.md` 「빌드 명령」에 있다. 그렇게 정한 이유와 Rider 경로의 오보 사례는 `docs/adr/0001-unify-build-path.md`에 있다.

`execute_run_configuration`은 호출마다 Rider가 확인 대화상자를 띄우고, 그걸 끄는 수단은 Brave 모드(IDE 전역으로 셸·실행구성 확인 해제)뿐이라 쓰지 않는다.

`Server/Binary/`는 gitignore되어 있으므로 **실행 전 빌드는 필수다.**

파일 편집에는 셸이 아니라 편집 도구를 쓴다. 근거는 `CLAUDE.md` 「안전」에 있다.

### Live Coding은 메인 DLL을 대체하지 않는다

에디터에서 Live Coding으로 컴파일하면 헤더 변경과 `UFUNCTION` 같은 리플렉션 변경이 반영되지
않는다. 결과는 `P1/Saved/Logs/P1.log`의 `LogLiveCoding`으로 판정한다. 성공하면
`Live coding succeeded`를 남긴다.

**Live Coding으로 검증한 코드는 정식 빌드를 거친 것이 아니다.** 패치는
`Binaries/Win64/UnrealEditor-P1.patch_N.*`로 따로 나가고 `UnrealEditor-P1.dll`은 그대로 남는다.
에디터를 닫고 다시 열면 패치가 사라지고 옛 바이너리가 로드된다. **커밋하기 전에 에디터를 닫고
전체 빌드를 한 번 돌린다.**

새 코드가 메인 DLL에 들어갔는지는 PDB의 심볼로 확인한다.

```
strings -n 6 P1/Binaries/Win64/UnrealEditor-P1.pdb | grep -c <새 심볼>
```
— 셸을 거치지 않는 편집 도구를 쓴다. 이것이 예방이다.

**빌드한 뒤 `.obj`와 실행 파일의 수정 시각을 소스와 대조한다.**
— 산출물이 소스보다 오래됐으면 재컴파일되지 않은 것이다. 이것이 확인이다.

위 두 절차 중 하나만 쓰지 않는다. 회피가 매번 통하지는 않으므로 예방과 확인을 함께 쓴다.

---

## 테스트를 추가할 때

`.cpp`를 `Server/GameServerTests/GameServerTests.vcxproj`의 `<ItemGroup Label="테스트 소스">`에 등록해야 한다. **이 프로젝트는 파일 자동 수집을 하지 않는다.**

등록을 잊으면 테스트가 조용히 안 돌아간다.

---

## 왜 이 구조인가

### gtest는 소스 벤더링이다. vcpkg를 쓰지 않는다

`Server/Libraries/googletest/`에 v1.18.0(커밋 `063de7e`) 소스를 넣고 `gtest-all.cc`를 테스트 프로젝트가 직접 컴파일한다. `gtest_main.cc`는 쓰지 않고 `TestMain.cpp`가 `main`을 둔다.
콘솔 코드페이지를 UTF-8로 돌려야 한국어 실패 메시지가 읽히기 때문이다. gmock은 넣지 않았다.

**근거는 이 저장소가 이미 모든 서드파티를 벤더링한다는 점이다.** `Server/Libraries/include/`의 `google`, `nlohmann`, `sw`, `hiredis`가 전부 그렇다. gtest만 다른 메커니즘을 들이면 새 클론에 "vcpkg를 설치한다"는 단계가 하나 늘고, 그 단계는 문서에만 존재하게 된다. 벤더링은 툴셋·CRT 완전 일치(v145로 같이 컴파일), 머신 선행조건 없음, 네트워크 없음을 동시에 만족한다. 대가는 저장소 용량 1.1MB다.

gtest는 `main()`을 재정의하므로 vcpkg가 자동 링크해 주지 못하는 예외 라이브러리다. manifest를 쓰더라도 `.vcxproj`에 수동 설정이 세 군데 붙는다.

**재검토 조건**: 서버에 벤더링할 의존성이 3개 이상 더 늘거나, gtest 버전 갱신이 번거로워지는 시점.

### 테스트 프로젝트가 GameServer의 `.cpp`를 직접 포함한다

`GameServer`는 exe라 링크할 수 없다. `GameServerTests.vcxproj`(콘솔 exe)가 **`GameServer.cpp`를 제외한 GameServer `.cpp` 전부**를 `ClCompile`로 포함한다. UE Low-Level Tests와 같은 패턴이다.

"필요한 것만"이 아니라 "main 빼고 전부"인 이유는 둘이다.

- `Inventory.cpp` → `Player.cpp` → `Room.cpp` → `GameSession.cpp` → `ProgressStorage.cpp`로 전이 의존이 이어져 결국 대부분을 넣게 된다. 링크 에러가 날 때마다 파일을 추가하는 루프는 비결정적이라 재현되지 않는다.
- `GameServer.cpp`에는 `main`이 있다. 테스트 타깃은 자기 `main`(`TestMain.cpp`)을 쓰므로 이 파일만 뺀다.

---

## 아직 없는 것 — 무엇을 확인했고 무엇을 안 했는지

**UE 클라의 기본 경로는 L2다.** 별도 빌드 타깃이 필요 없어 `P1` 모듈에 그대로 컴파일된다.

아래 표의 L3·L4는 **미착수**다. L2는 순수 로직 테스트만 돌고 입력과 월드를 쓰는 테스트는 미착수다. 「완료 기준」의 "존재 ≠ 가능"을 여기에도 적용해, 확인한 것과 확인하지 않은 것을 갈라 적는다. **착수할 때는 가장 싸게 실패하는 경로부터 돌린다** — 파일이 있는지 여러 번 확인하는 것보다 한 번 빌드해 보는 게 싸다.

| 계층 | 확인한 것 | 확인 안 한 것 |
|---|---|---|
| L2 게임 로직+입력 (Simple Automation Test + `InjectInputForAction`) | 순수 로직 테스트의 컴파일과 실행 · 실패한 테스트의 `failed` 집계와 `Run-UeTests.ps1`의 종료 코드 1 (2026년 10월 2일, `P1.Sync.MoveCorrection`의 빨강 단계에서 실측) | `InjectInputForAction`으로 입력을 넣는 테스트. 월드를 띄우는 테스트 |
| L3 UI 입력 (Automation Spec + Automation Driver) | 없음 — **아직 안 봤다** | 전부 |
| L4 E2E (Gauntlet TestController) | 설치본에 `Engine/Plugins/Experimental/Gauntlet` 플러그인 + public `GauntletTestController.h` + 컴파일된 `Gauntlet.Automation.dll` | **실행 전체** |

- L2의 기본 실행 경로는 `Run-UeTests.ps1`이다. 에디터를 띄우지 않는다. 대체 경로는 둘이다. 하나는 사람이 쓰는 에디터의 `Window > Test Automation`이고, 다른 하나는 에디터가 이미 떠 있을 때 프로세스 시작 비용을 치르지 않는 언리얼 MCP의 `RunTests`다. 두 대체 경로는 리포트를 판정하지 않으므로 완료 판정에 쓰지 않는다.
- L3 실행에는 **에디터가 필요하다** (`Window > Test Automation` 또는 `-ExecCmds="Automation RunTests ..."`). 서버처럼 무인 루프가 되지 않는다.
- **L3는 Live Coding 비호환 — TDD 루프 금지, 배치 전용.**
- L4는 병렬 실행 시 포트 파라미터화.
- CI: 서버 쪽 전제는 갖춰졌다. 저장소에 없는 파일 없이 `GameServerTests`가 빌드된다. UE 쪽은 에디터 의존 때문에 별도 검토가 필요하다.

---

## UE L1(Low-Level Tests)은 채택하지 않는다

**재검토 조건은 없다.** 폐기다. 근거는 둘이고 순서가 중요하다.

**1. 프로젝트 제약.** 이 프로젝트는 런처 설치본 엔진만 쓴다. 소스 빌드는 선택지가 아니다(2026-08-27 확정). 아래 2번이 뒤집히더라도 결론은 그대로다.

**2. 기술적 불가.** 설치본에서 LLT 타깃은 빌드 자체가 거부되고 설정으로 우회할 수 없다.

표면의 거부는 두 줄이다.

- `TargetRules.cs:2690-2693` — Program 타입 타깃이 `.uproject` 폴더 안에 있으면 조건 없이 `Unique`
- `RulesAssembly.cs:677-680` — `Unique` + 설치본 엔진이면 예외를 던지고 중단

여기까지만 보면 검사를 통과시키면 될 것처럼 읽힌다. 타깃에서 `BuildEnvironment = Shared`를 지정하면 실제로 저 분기를 건너뛰고, 커뮤니티 workaround도 그것이다. **그런데 LLT에는 통하지 않는다.** `TestTargetRules.SetupCommonProperties`가 `bCompileAgainstEngine = false`, `bCompileAgainstEditor = false`, `LinkType = Monolithic`, `STATS=0` 외 다수를 조건 없이 잡는데, 이 목록이 곧 "빌드 환경"의 정의다. 설치본이 주는 프리빌트 바이너리는 정반대 설정으로 컴파일돼 있다. `Shared`로 강제한다는 것은 UBT에게 그 바이너리를 재사용하라고 시키는 것이라, 검사를 통과시켜도 정의가 어긋난 산출물이 나온다.

플래그로 우회할 성질이 아니라 설치본의 정의상 한계다. 유일한 해제 수단인 소스 빌드 엔진은 근거 1에 의해 선택지가 아니다.

손실은 작다 — L1으로 검증할 순수 로직은 전투 판정·인벤토리·레벨 테이블처럼 대부분 서버 소유이고 그쪽은 GoogleTest가 덮는다. 클라에 남는 L1 대상은 이동 보간 수식 정도로 얇다.

**`P1/Source/`에 Program 타깃을 추가하지 않는다.** `ProjectFileGenerator.cs:3176`이 프로젝트 파일 생성 시에도 같은 검증을 하므로, 두면 P1의 프로젝트 파일 재생성이 깨진다.
