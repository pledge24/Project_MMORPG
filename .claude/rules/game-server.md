---
paths:
  - "Server/**"
---

# 게임 서버 코드

- `GameServer`에 새 `.cpp`를 만들면 `GameServer.vcxproj`와 `GameServerTests.vcxproj`, 두 `.filters`에
  모두 등록한다. `GameServerTests`가 `GameServer`의 `.cpp`를 직접 컴파일하고, 두 프로젝트 모두 파일을
  자동으로 수집하지 않는다.
- `GameServer.exe`가 떠 있으면 실행 파일이 잠겨 솔루션 빌드가 실패한다. 이때는 MSBuild에
  `/t:GameServerTests`를 붙여 테스트 프로젝트만 빌드한다. 사람의 프로세스는 종료하지 않는다.
