---
paths:
  - "Server/GameServer/**"
  - "Server/GameServerTests/**"
  - "Server/DummyClient/**"
  - "Server/ServerCore/**"
---

# 게임 서버 코드

- `GameServer`에 새 `.cpp`를 만들면 `GameServer.vcxproj`와 `GameServerTests.vcxproj`, 두 `.filters`에
  모두 등록한다. `GameServerTests`가 `GameServer`의 `.cpp`를 직접 컴파일하고, 두 프로젝트 모두 파일을
  자동으로 수집하지 않는다.
- `GameServer.exe`가 떠 있으면 실행 파일이 잠겨 솔루션 빌드가 실패한다. 이때는 MSBuild에
  `/t:GameServerTests`를 붙여 테스트 프로젝트만 빌드한다. 사람의 프로세스는 종료하지 않는다.

아래 컨벤션은 `GameServer`, `GameServerTests`, `DummyClient`에 적용한다. `ServerCore`에는 「서버 include」와 `GLogger` 로그만 적용한다.
— ServerCore는 완성된 코어라서 이름을 고쳐 얻는 것보다 잃는 것이 크다. 남아 있는 미준수 멤버 이름도 고치지 않는다

## 서버 이름

| 대상 | 형식 | 예 |
| --- | --- | --- |
| 클래스·구조체 | 접두사 없는 PascalCase | `RoomManager` |
| 함수 | PascalCase | `ClearDirtyFlags` |
| 멤버 변수 (public 포함) | `_camelCase` | `_roomId` |
| static 멤버 변수 | `s_camelCase` | `s_idGenerator` |
| 지역 변수·매개변수 | camelCase | `targetRoomId` |
| 상수 | SCREAMING_SNAKE_CASE | `MAX_LEVEL` |

- 예외: 동작 없는 순수 데이터 구조체의 필드, 벡터 성분 `x`·`y`·`z`, OS가 정한 이름을 담는 포인터(`ConnectEx`)는 밑줄 없이 쓴다
- 클라이언트 패킷으로 진입하는 핸들러는 `C_Handle*`, 서버 내부에서 부르는 핸들러는 `Handle*`로 짓는다. `C_Handle*`에 들어온 값은 전부 검증한다
- `EntityComponent` 파생 클래스의 이름은 `Component`로 끝내고, 그 밖의 클래스 이름은 `Component`로 끝내지 않는다

## 서버 include

- 자기 프로젝트 헤더는 프로젝트 루트 기준으로, ServerCore 헤더는 `ServerCore/`부터 적는다. ServerCore 안에서도 같다
  — GameServer와 ServerCore에 `Core/Types.h`가 하나씩 있어서, 접두사가 없으면 엉뚱한 파일이 잡힌다
- pch는 GameServer와 테스트가 `"Core/pch.h"`, ServerCore가 `"ServerCore/Core/pch.h"`, DummyClient가 `"Main/pch.h"`다
- 새 폴더를 만들어도 `.vcxproj`의 `IncludePath`에 등록하지 않는다

## 락, 로그, 전역 객체

- `USE_LOCK`으로 자기 락을 잡는 함수는 헤더 주석을 `[LOCK]`으로 시작한다. 호출자의 락에 기대는 함수에는 "락을 잡은 상태에서만 부른다"고 적는다
  — `MAKE_LOCK`은 재진입하지 않으므로, 같은 락을 잡은 채로 `[LOCK]` 함수를 부르면 교착 상태에 빠진다
- 로그는 `GLogger->Warning("템플릿 {}이 없다", id)`처럼 `std::format` 형식으로 남긴다. `cout`·`printf`로 표준 출력에 직접 쓰지 않는다
  — 여러 스레드가 한 콘솔에 쓴다. `GLogger`는 한 줄을 통째로 쓰고 시각, 레벨, 스레드 id를 붙인다
- 전역 객체는 ServerCore면 `CoreGlobal`에, 게임 서버면 `ServerContext`에 넣는다. 두 객체는 `main`이 지역 변수로 만든다
  — 정적 객체끼리는 생성·소멸 순서가 정해지지 않는다. 같은 이유로 정적 객체의 생성자와 소멸자에서는 `GLogger`가 `nullptr`이다
