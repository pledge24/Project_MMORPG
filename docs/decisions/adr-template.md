---
# ─────────────────────────────────────────────────────────────
# 이 블록은 기계가 읽는 메타데이터다. 키를 임의로 늘리지 않는다.
# status : proposed | accepted | rejected | deprecated | superseded
# scope  : client | server | protocol | shared | build | ops
#          여러 개면 배열. protocol 은 클라·서버를 동시에 구속한다는 뜻이다.
# ─────────────────────────────────────────────────────────────
status: proposed
date: YYYY-MM-DD
scope: [server]
supersedes: null          # 예: ADR-0000
superseded-by: null       # 예: ADR-0007
---

# {해결한 문제와 채택한 해법이 함께 드러나는 짧은 제목}

## Context and Problem Statement

{2~3문장. 가능하면 질문 형태로 끝낸다.
이 결정이 어느 부분(모듈, 레이어, 프로세스 경계, 티어)에 걸리는지 명시한다.}

## Decision Drivers

* {제약 또는 상충하는 힘. 측정 가능하면 수치로.}
* {예: 런처 설치본 엔진만 쓴다 — 소스 빌드는 선택지가 아니다}
* {예: 판정이 종료 코드여야 한다 — 에이전트가 자기 점검할 수 있게}

## Considered Options

1. {채택안 — 관례상 맨 앞}
2. {대안}
3. {대안}

<!-- 옵션은 반드시 같은 추상 수준이어야 한다.
     기술과 제품을 비교하거나 아키텍처 스타일을 프로토콜 명세와 비교하지 않는다.
     이 맥락에서 아예 동작하지 않는 옵션을 들러리로 세우지 않는다. -->

## Decision Outcome

채택: "{옵션 1}". 이유는 {근거}.

### Consequences

* Good, because {…}
* Bad, because {…}

<!-- 단점 칸이 비어 있거나 무해한 것만 적혀 있으면 그 ADR은 아직 미완성이다.
     장기적으로만 드러나는 비용(운영, 유지보수, 재작업)을 특히 챙긴다. -->

### Confirmation

<!-- ★ 이 프로젝트에서는 선택 항목이 아니라 필수다.
     "이 결정이 지켜지고 있는가" 를 코드에서 확인하는 방법을 적는다.
     자동 검사가 있으면 실행 명령까지. 없으면 리뷰 시 확인할 구체적 지점을 적는다.
     "코드 리뷰로 확인" 만으로는 부족하다.

     작업을 마친 뒤 에이전트가 이 항목들로 자체 점검하고 결과를 보고한다. -->

* {예: `Server/` 하위 어떤 소스도 UE 헤더를 include 하지 않는다}
* {예: `GameServerTests.exe` 종료 코드 0 — 판정이 종료 코드다}

## Pros and Cons of the Options

### {옵션 1}

* Good, because {…}
* Neutral, because {…}
* Bad, because {…}

### {옵션 2}

* Good, because {…}
* Bad, because {…}

## More Information

{확신 수준(높음/보통/낮음과 그 이유), 재검토 조건, 관련 ADR.}

<!-- 재검토 조건 예: "동시 작업 세션이 3개를 넘으면 다시 본다"
     확신 수준은 낮아도 된다. 솔직한 편이 낫다. -->
