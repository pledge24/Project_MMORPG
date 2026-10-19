---
paths:
  - "Server/AuthServer/**"
---

# 인증 서버 코드

- `src/` 아래의 폴더 이름은 소문자로 쓴다
  — 대소문자를 구분하지 않는 파일 시스템에서 개발하고 구분하는 곳에 배포하면 import 경로가 조용히 깨진다
- `common/`의 코드는 도메인 폴더(`auth/` 등)를 import하지 않는다. 도메인의 타입이 필요하면 `common/` 쪽은 원시 타입으로 받는다
  — 도메인 폴더를 지우면 그 도메인 로직만 사라져야 한다. 배치 규칙 전체는 `docs/folder-structure.md`의 인증 서버 문단에 있다
- 클라이언트에 알릴 실패는 `AppError`로 던진다. 핸들러에서 `try`/`catch`로 응답을 만들지 않는다
  — 실패 봉투와 상태 코드는 `common/errorHandlers.ts`의 `errorHandler` 한 곳이 만든다. 핸들러마다 응답을 만들면 봉투 형식이 갈라진다
- `catch`는 알아본 에러만 다른 값으로 바꾸고, 나머지는 다시 던진다(`auth/auth.repository.ts`의 UNIQUE 제약 위반 처리)
  — 다시 던진 에러는 `errorHandler`가 `console.error`로 원문을 남긴다. 삼키면 bcrypt, MSSQL, Redis 중 무엇이 죽었는지 재현해야 안다
- `mssql`과 `redis`는 저장소 구현이 있는 파일(`*.repository.ts`, `common/ticket.ts`)과 `common/db.ts`에서만 import한다. 판정 코드는 저장소 인터페이스만 안다
  — 테스트가 메모리 구현을 넣어 판정과 HTTP 계약을 검사하는 것이 이 경계에 기댄다
- 저장소 인터페이스에 메서드를 더하면 메모리 구현(`*.memory.ts`)에도 같은 동작을 넣는다
  — 메모리 구현이 실제 구현과 다르게 동작하면 테스트가 통과해도 실제 서버에서 틀린다
