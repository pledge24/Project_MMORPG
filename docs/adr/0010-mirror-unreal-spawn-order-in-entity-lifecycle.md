---
status: accepted
---

# 엔티티 생명주기를 언리얼의 스폰 순서에 맞춘다

게임 서버의 엔티티(`Entity → Creature → { Player, Monster }`)는 언리얼 액터의 스폰 순서를 따른다.
`EntityFactory::Create<T>(params)`가 id를 발급하고 엔티티를 만든 뒤 `Init(params)`로 스폰 매개변수를
연결한다. 룸은 `AddEntity`에서 엔티티를 등록하고, 엔티티 수명에서 한 번만 `Start`를 부른다. `Start`는
언리얼의 BeginPlay에 대응하고 실패하지 않는다.

| 언리얼 | 게임 서버 |
| --- | --- |
| `SpawnActorDeferred` | `EntityFactory::Create<T>(params)`: id 발급, 생성, `Init(params)` |
| `FinishSpawning` | `Player::OnLoaded()`: 플레이어만 있다 |
| `UWorld::SpawnActor` | `Room::SpawnEntity<T>(params)`: 생성, `AddEntity` |
| BeginPlay | `AddEntity`가 처음 한 번 부르는 `Start()` |

플레이어만 생성과 첫 프레임 사이에 단계가 하나 더 있고, 스폰이 월드 하나가 아니라 팩토리와 룸으로
나뉜다. 둘 다 언리얼과 다르게 보이므로 이유를 적는다.

## 플레이어는 룸을 모른 채 만들어진다

플레이어는 게임 입장 때 DB 스레드의 불러오기 잡에서 만들어진다. 들어갈 룸 번호는 DB에서 읽어야 알
수 있다. 그래서 룸의 함수로는 플레이어를 만들 수 없고, 만드는 일(팩토리)과 룸에 넣는 일(룸)을 나눈다.
몬스터는 룸 큐 위에서 태어나므로 `Room::SpawnEntity`가 두 일을 이어서 한다.

DB 불러오기는 `Init`이 만든 인벤토리와 장비에 값을 채운다. 저장된 스탯을 계산 결과와 대조하는 검증은
불러오기가 끝난 뒤에야 할 수 있다. 이 검증은 `Player::OnLoaded`가 맡고, 실패하면 `S_ENTER_GAME` 실패를
보낸다. 검증이 빠져나간 덕분에 `Start`에는 실패할 일이 남지 않는다.

## `Start`는 엔티티 수명에서 한 번만 불린다

플레이어는 룸을 옮길 때마다 `AddEntity`를 다시 거친다. 룸 이동 때의 처리는 `OnEnterRoom`이 맡는다.
`Start`는 `_hasBegunPlay`로 막아서 처음 등록될 때만 부른다.

`_room` 설정, `_prevTime` 설정, `Start` 호출을 `AddEntity`와 `Entity::Start` 안에 둔다.
— 이전에는 호출자가 이 셋을 순서대로 불러야 했다. `_room`을 넣기 전에 `Start`를 부르면 틱이 조용히
시작되지 않았다.

## 스폰 매개변수는 비가상 `Init`으로 받는다

클래스마다 `SpawnParams`를 둔다. 파생 클래스의 구조체는 부모 클래스의 구조체를 상속한다. 가상 함수는
재정의할 때 매개변수 타입을 바꿀 수 없으므로 `Init(const SpawnParams&)`은 비가상으로 둔다. 팩토리가
템플릿이라 `T`를 정적으로 알고 그 클래스의 `Init`을 부른다. 몬스터에 플레이어 매개변수를 넘기면
컴파일러가 막는다.

엔티티 타입은 생성자가, id는 팩토리가 정하므로 둘 다 매개변수에 넣지 않는다.

## 검토한 대안

- **DB를 먼저 읽어 `PlayerSpawnParams`에 담고 `Init`에서 검증까지 끝낸다.** 스폰 매개변수 모델에
  가장 충실하다. 하지만 DAO가 `session->_player`에 직접 쓰는 구조를 모두 바꿔야 한다.
- **`Start`를 룸에 들어갈 때마다 부른다.** 플레이어의 `OnEnterRoom`과 역할이 겹친다.
- **언리얼의 `UWorld`에 대응하는 클래스(가칭 렐름)를 두고 스폰 진입점으로 삼는다.** 새로 맡는 책임이
  id 발급 하나뿐이어서 이름만 바뀐 `RoomManager`가 된다. 엔티티를 관리하는 주체가 소속 룸이라는
  원칙과도 어긋난다.
- **가상 `Init(const EntitySpawnParams&)` 안에서 각 클래스가 자기 구조체로 캐스트한다.** 잘못된
  구조체를 넘겨도 컴파일러가 잡지 못한다.

## 결과

- 엔티티 클래스를 추가하면 그 클래스의 `SpawnParams`를 정하고, `Init`의 맨 앞에서 `Super::Init(params)`를
  부른다.
- `Init`은 protected이고 `EntityFactory`만 부른다. 테스트도 팩토리로 엔티티를 만든다.
- `Player::Init`은 세션이 비어 있으면 세션 연결을 건너뛴다. 운영 코드는 언제나 세션을 넘기므로 이
  분기는 테스트만 탄다.
- 언리얼의 `EndPlay`에 대응하는 함수는 두지 않는다. 룸에서 빠진 엔티티는 다른 참조가 없으면 사라진다.
