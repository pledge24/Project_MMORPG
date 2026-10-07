---
status: accepted
---

# 클라이언트 모듈의 include 평탄화를 제거한다

`P1.Build.cs`의 `PrivateIncludePaths`에서 도메인 폴더를 뺀다. `#include`는
`#include "Combat/AttackSystemComponent.h"`처럼 경로를 한정해서 쓴다. 생성물이 있는 폴더는
예외로 남긴다.

평탄화는 편의 장치다. 일부러 없앤 것을 보면 왜 불편하게 만들었는지 묻게 되므로 근거를 남긴다.

## 무엇이 문제였는가

UBT는 모듈 루트를 include 경로에 자동으로 넣는다. 거기에 더해 `PrivateIncludePaths`가 모듈의
하위 폴더를 전부 등록하면, 어느 폴더에 있는 파일이든 `#include "Creature.h"`처럼 파일 이름만으로
부를 수 있다.

편한 대신 대가가 있다. **폴더가 도메인을 나타내게 되어도 `#include` 줄이 그 도메인을 감춘다.**
전투 코드가 인벤토리 코드를 부르는지 UI 코드를 부르는지 include 줄만 보아서는 알 수 없다. 폴더를
옮겨도 빌드가 깨지지 않는 대신, 경계를 넘는 참조도 빌드가 알려주지 않는다. 남는 방어선은 리뷰
하나뿐이다.

## 이 결정이 하지 않는 일

**컴파일러가 경계 위반을 막아 주지는 않는다.** 모듈 루트가 여전히 include 경로에 있으므로
`Combat/`의 파일이 `UI/`의 헤더를 경로 한정으로 부르는 것은 그대로 컴파일된다.

얻는 것은 위반이 **보인다**는 점이다. `#include` 줄에 도메인 이름이 적히므로 검색 한 번으로
잡히고 리뷰에서 눈에 띈다. 이 정도로 충분하다고 판단했다.

컴파일러 차원의 차단은 도메인을 별도 모듈로 나눠야 가능하다. 수작업 파일이 100개에 못 미치는
지금 규모에서 모듈을 나누면 빌드 구성만 늘고 얻는 것이 적다. 도메인을 넘는 include가 경로 한정
도입 이후에도 계속 늘어난다면 그때 모듈 분리를 검토한다.

## 2026년 9월 22일 덧붙임 — 위 재검토 조건을 재 보았다

모듈 분리를 다시 꺼낸 자리에서 조건을 실측했고, **충족되었다는 근거가 없어서 모듈을 나누지
않기로 했다.** 같은 질문을 다음에 꺼내는 사람이 측정을 되풀이하지 않도록 남긴다.

| 시점 | 손코딩 파일 | `folder-structure.md` 3.3이 금지한 방향의 `#include` |
| --- | --- | --- |
| 이 ADR 도입 (커밋 `eeb2cbe`, 2026-09-20) | 81개 | 42건 |
| 측정 시점 (2026-09-22) | 86개 | 27건 |

**두 숫자를 곧바로 추세로 읽으면 안 된다.** 그 사이에 폴더 구조 자체가 바뀌었고(#72·#73), 경과한
시간이 이틀이며, 그 이틀의 변경은 새 기능이 아니라 재배치였다. 말할 수 있는 것은 「늘었다는
근거가 없다」와 「파일 수가 여전히 100개 미만이다」까지다.

**모듈을 나눠도 대부분이 남는다.** 27건 중 18건이 게임 도메인에서 `.pb.h`를 직접 부르는 것이다.
이것은 모듈 경계가 아니라 자료형 설계의 문제라서, 모듈을 쪼개면 게임 모듈이 프로토콜 모듈에
의존하는 형태로 옮겨갈 뿐이다.

그래서 **컴파일러 대신 CI가 막는 쪽을 택했다.** `Tools/ConventionLint/check_conventions.py`에
폴더 간 의존 방향 검사를 더한다. 나머지 9건(`Core` 2 · `UI` 2 · 모듈 헤더 5)이 그 대상이고,
생성물을 부르는 18건은 `docs/tech-debt.md`의 「게임 도메인이 배선 계층을 거꾸로 부른다」가 맡는다.

**이 판단을 다시 볼 자리는 모듈이 셋 이상이 될 때다.** `P1Editor` 모듈을 만들면(`docs/backlog.md`
8번의 `C9`) 검증할 모듈 의존 관계가 생긴다. 그때는 `Build.cs`가 C# 코드라는 점을 이용해 공통
베이스 클래스에 레이어 표를 두고 각 모듈 생성자가 검증 함수를 부르게 하는 방법이 값어치를 갖는다.
지금은 모듈이 `P1`과 `ProtobufCore` 둘뿐이고 그 관계가 하나여서 표가 비어 있다.

**2026년 10월 4일 덧붙임 — 의존 방향 검사는 아직 없고, 생성물 역참조는 해소됐다.** 의존 방향 검사는
`docs/backlog.md` 25번이 맡는다. 생성물을 부르던 자리를 맡던 tech-debt 항목은 #105(`f54c178`)가 고치면서
지웠다.

## 예외

생성물이 있는 폴더는 `PrivateIncludePaths`에 남긴다. protobuf 생성 코드는 서로를
`#include "Enum.pb.h"` 형태로 부르고, 생성물은 손으로 고치지 않는다. 이 폴더까지 빼면 생성기를
다시 돌릴 때마다 빌드가 깨진다.

## 2026년 10월 7일 덧붙임 — 서버 프로젝트에도 같은 결정을 적용했다

`Server/`의 네 프로젝트(ServerCore, GameServer, GameServerTests, DummyClient)도 `.vcxproj`의
`IncludePath`에서 하위 폴더를 뺐다. 이유는 클라이언트와 같다. 서버도 `Game/` 아래가 도메인으로
갈려 있고, 파일 이름만 적으면 그 경계가 include 줄에서 보이지 않는다.

**ServerCore 헤더는 `ServerCore/` 접두사를 붙여 부른다.** ServerCore 안에서 부를 때도 같다.

```cpp
#include "ServerCore/Network/Session.h"   // ServerCore 헤더: 솔루션 폴더 기준
#include "Game/Room/Room.h"               // 자기 프로젝트 헤더: 프로젝트 루트 기준
```

— 두 프로젝트가 `Core/`·`Network/`·`Utils/`·`DB/` 폴더를 함께 갖고, `Core/Types.h`는 파일 이름까지
같다. ServerCore 헤더가 `"Core/Types.h"`를 부르면 GameServer에서 컴파일할 때 GameServer의
`Core/Types.h`가 먼저 잡힌다. 솔루션 폴더 기준으로 적어야 두 파일이 갈린다.

그래서 각 프로젝트의 `IncludePath`에는 아래 경로만 남는다. 서드파티 경로는 그대로 둔다.

| 프로젝트 | 남은 경로 |
| --- | --- |
| ServerCore | `$(SolutionDir)` |
| GameServer | `$(ProjectDir)`, `$(ProjectDir)Protocol`, `$(SolutionDir)` |
| GameServerTests | `$(ProjectDir)`, `$(SolutionDir)GameServer`, `$(SolutionDir)GameServer\Protocol`, `$(SolutionDir)` |
| DummyClient | `$(ProjectDir)`, `$(ProjectDir)Protocol`, `$(SolutionDir)` |

**`Protocol` 폴더는 클라이언트와 같은 이유로 남겼다.** 생성기가 만드는 `ServerPacketHandler.h`와
`ClientPacketHandler.h`가 `#include "Protocol.pb.h"`를 파일 이름으로 부른다. 이 템플릿은 UE
클라이언트와 공유하므로 서버에 맞춰 고치지 않는다. 손으로 쓰는 코드는 생성물도
`"Protocol/Protocol.pb.h"`처럼 경로로 부른다.

**pch도 경로로 부른다.** `/Yu`는 `.cpp`의 include 문자열이 `PrecompiledHeaderFile`과 글자 그대로
같아야 하므로 둘을 함께 바꿨다. GameServer와 GameServerTests는 `Core/pch.h`, ServerCore는
`ServerCore/Core/pch.h`, DummyClient는 `Main/pch.h`다.

**ServerCore는 `docs/conventions.md` 3장의 이름 규칙에서 빠져 있지만 include 규칙은 따른다.**
— ServerCore 헤더는 GameServer와 DummyClient의 컴파일 단위 안에서도 펼쳐진다. ServerCore 헤더끼리
파일 이름으로 부르면, 그 이름을 찾도록 ServerCore의 하위 폴더를 두 프로젝트의 `IncludePath`에
다시 넣어야 한다.
