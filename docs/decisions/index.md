# Architecture Decision Log

이 디렉터리는 프로젝트의 **결정 로그**다. 코드를 읽어도 알 수 없는 제약과 그 근거만 남긴다.
설계 문서도, 구현 가이드도, 튜토리얼도 아니다.

## 읽기 규칙

1. **작업 시작 전에 이 표를 먼저 읽는다.** 개별 ADR 본문은 관련 있는 것만 연다.
   전부 컨텍스트에 올리지 않는다.
2. `status: accepted` 인 ADR만 현행 제약이다. `superseded` / `deprecated` / `rejected` 는
   이력이며 **따르지 않는다.**
3. 각 ADR의 `Confirmation` 절이 실제로 지켜야 하는 항목이다. 코드를 쓰기 전에 읽고,
   작업을 마친 뒤 해당 `scope` 의 Confirmation 항목으로 자체 점검한 결과를 보고한다.
4. **채택된 ADR은 수정하지 않는다.** 결정이 바뀌면 새 번호로 ADR을 쓰고, 기존 ADR의
   `superseded-by` 와 새 ADR의 `supersedes` 를 상호 링크한다. 이 표의 상태도 함께 갱신한다.

## 언제 ADR을 쓰는가

- 되돌리는 데 비용이 큰 결정 (프로세스 경계, 스레딩 모델, 직렬화 포맷, 저장소 선택, 빌드 환경)
- 코드만 봐서는 "왜 이렇게 안 했는지" 알 수 없는 결정
- 같은 논쟁이 두 번 이상 반복된 주제

반대로 **쓰지 않는 것**: 특정 함수의 에러 처리 방식, 어떤 모듈의 재시도 로직, 변수 네이밍.
이런 구현 수준 결정은 커밋 메시지와 `docs/work/` 의 「결정」 절에 남긴다.
ADR에 밀어 넣으면 로그가 금방 못 쓰게 된다.

## scope 값

| 값 | 구속 대상 |
|---|---|
| `client` | `P1/` 아래. UE 클라이언트 |
| `server` | `Server/GameServer`, `Server/AuthServer` |
| `protocol` | `.proto` 와 생성 파이프라인. **클라와 서버를 동시에 구속한다** |
| `shared` | 양쪽이 쓰는 공용 코드 |
| `build` | 빌드 구성, 솔루션, 의존성, 테스트 인프라 |
| `ops` | DB 스키마, 배포, 운영 |

## 결정 목록

| ID | 제목 | Scope | Status | 한 줄 요약 |
|---|---|---|---|---|
| [0000](0000-superseded-harness-verdict.md) | 하네스 판정표 (초안) | build | superseded | ADR-0001 이 대체. 소스 4개 기반 초기 판정 |
| [0001](0001-harness-verdict.md) | 하네스 판정표 최종 확정 | build | accepted | 4개 소스의 하네스 도구·워크플로 채택 판정. 9건 수정 확정, 배치 실행 모드는 §9에서 해동 조건부 보류 |
| [0002](0002-l1-test-infra.md) | L1 테스트 인프라 | build, server | accepted | gtest 벤더링, 판정은 종료 코드, 빌드=Rider/실행=셸, UE LLT 기각 |

## 새 ADR 만들기

`new-adr` 스킬을 쓴다. 스킬이 없으면:

```bash
cp docs/decisions/adr-template.md docs/decisions/000N-title-with-dashes.md
```

파일명은 `NNNN-title-with-dashes.md` (소문자, 대시). 작성 후 위 표에 한 줄 추가한다.
`status` 는 `proposed` 로 두고 사람 승인 없이 `accepted` 로 바꾸지 않는다.
