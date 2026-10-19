---
paths:
  - "Server/AuthServer/**"
---

# 인증 서버 코드

- `src/` 아래의 폴더 이름은 소문자로 쓴다
  — 대소문자를 구분하지 않는 파일 시스템에서 개발하고 구분하는 곳에 배포하면 import 경로가 조용히 깨진다
- `common/`의 코드는 도메인 폴더(`auth/` 등)를 import하지 않는다. 도메인의 타입이 필요하면 `common/` 쪽은 원시 타입으로 받는다
  — 도메인 폴더를 지우면 그 도메인 로직만 사라져야 한다. 배치 규칙 전체는 `docs/folder-structure.md`의 인증 서버 문단에 있다
- `catch`에서는 응답을 돌려주기 전에 `console.error`로 에러 원문을 남긴다. 원문이 없으면 bcrypt, MSSQL, Redis 중 무엇이 죽었는지 재현해야 안다
