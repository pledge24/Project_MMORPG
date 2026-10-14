---
paths:
  - "P1/Source/**"
  - "Server/GameServer/**"
  - "Server/GameServerTests/**"
  - "Server/DummyClient/**"
  - "Server/ServerCore/**"
---

# C++ 공통 컨벤션

클라이언트와 게임 서버의 C++ 코드 안의 규칙이다. 파일을 어디에 두고 무엇이라 부를지는 `docs/folder-structure.md`가 다룬다.
생성물(`*.pb.h`, `*PacketHandler.*`)과 생성물끼리의 `#include`는 생성기가 만들므로 손으로 고치지 않는다.

## 멤버 배치

`Server/ServerCore/`에는 `.cpp` 정의 순서만 적용한다.

- 헤더는 생성자·소멸자 → 엔진 인터페이스 오버라이드 → 기능 섹션 순서로 쓴다. 서버의 엔진 인터페이스는 기반 클래스의 `Init`·`Start`·`Tick`이다
- 기능 섹션 안에서는 public → protected → private 순서로, 함수 다음에 변수를 둔다
  — 접근 지정자로만 정렬하면 한 기능의 함수와 변수가 파일 양 끝으로 갈라진다
- 섹션 표시는 `//~ Begin <TypeName> Interface` / `//~ End <TypeName> Interface`와 `//~ <기능 이름>` 두 형태만 쓴다
- 섹션은 기능마다 한 번만 연다. 여러 기능이 함께 쓰는 멤버는 마지막 섹션인 `//~ 공통`에 둔다. 기능이 하나뿐인 클래스는 섹션 표시를 생략한다
- `.cpp`는 헤더의 선언 순서대로 정의한다. 그 파일에서만 쓰는 자유 함수와 익명 네임스페이스는 첫 멤버 정의 위에 둔다
- 멤버 선언 순서를 바꾸면 초기화 순서를 확인한다. 초기화 리스트가 다른 멤버를 읽으면 읽는 값이 달라지고, 컴파일러가 경고하지 않을 때가 있다
