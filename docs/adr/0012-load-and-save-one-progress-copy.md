---
status: accepted
---

# 불러오기와 저장이 진행 사본 하나를 쓴다

플레이어의 진행(용어집의 「진행」)을 `PlayerProgress`(`Server/GameServer/Game/Entities/PlayerProgress.h`) 하나로 정의한다.
불러오기는 DAO가 이 사본을 채우고, 저장은 룸 큐가 뜬 `PlayerSaveData`가 이 사본을 바뀐 슬롯 표시와 함께 감싼다.
DB 스레드는 어느 쪽에서도 살아 있는 `Player`를 읽거나 쓰지 않는다. 2026년 10월 9일 #209에서 정했다.

입장은 이 순서로 한다(`Network/GameEntry`).

1. `ProgressStorage::Load`가 DB 잡에서 사본을 채운다. DAO는 세션과 패킷을 모른다.
2. 사본을 `PlayerSpawnParams`에 실어 `EntityFactory::Create<Player>`로 플레이어를 만든다. `Player::Init`이 사본을 쓰고
   `OnLoaded`로 검증한다. 검증에 실패하면 팩토리가 `nullptr`를 돌려준다.
3. 검증을 통과한 플레이어만 세션에 등록한다. 세션이 이미 플레이어를 갖고 있으면 등록하지 않는다.

## 왜 사본인가

전에는 불러오기 전에 세션에 빈 플레이어를 넣어 두고, DAO가 세션에서 그 플레이어를 꺼내 직접 채웠다.
그래서 불러오기나 검증이 실패해도 절반만 채운 플레이어가 세션에 남았다. 저장 쪽은 이미 룸 큐에서 뜬 사본을 썼으므로
불러오기와 저장이 서로 다른 모양이었다. 사본 하나로 맞추면 한쪽에만 있는 필드가 생길 때 바로 드러난다.

## 검토한 대안

- **세션의 플레이어에 직접 채우는 방식을 두고 실패할 때 세션에서 지운다.** 지우기 전까지 다른 스레드가 절반만 채운
  플레이어를 볼 수 있고, DB 스레드가 플레이어를 만지는 구조는 그대로 남는다.
- **`ProgressStorage`가 `Player`를 만들어 돌려준다.** DB 계층이 엔티티의 생성과 검증 규칙을 알게 된다. 테스트하려면
  DB를 거쳐야 한다.
- **불러오기용 구조체와 저장용 구조체를 따로 둔다.** 저장 사본은 이미 있었다. 둘이 같은 필드를 따로 들면 한쪽에
  필드를 더하고 다른 쪽을 잊는 일이 생긴다.

## 결과

- `docs/adr/0010-mirror-unreal-spawn-order-in-entity-lifecycle.md`의 「검토한 대안」 첫 항목(DB를 먼저 읽어
  `PlayerSpawnParams`에 담고 `Init`에서 검증까지 끝낸다)을 이 결정이 채택한다.
- 불러온 사본의 인벤토리는 칸 수만큼 채운 배열이 아니라 DB 행마다 슬롯 하나이고, 칸 번호는 `slot_id`에 있다.
  칸에 넣는 일은 `Player::ApplyProgress`가 하고, DB 값을 믿지 않고 검증한다.
- 최대 HP, 최대 MP, 공격력, 최대 경험치는 사본에 있어도 믿지 않는다. 레벨 표와 착용 장비로 다시 계산한다.
- 진행에 필드를 더하면 `PlayerProgress`, 불러오기 DAO, 저장 DAO, `Player::ApplyProgress`, `Player::MakeSaveData`를 함께 고친다.
