# 하네스 전환 절차서

기존 하네스를 새 하네스로 바꾸는 실행 문서. 전환이 끝나면 `docs/work/`에 기록으로 남긴다.

---

## 0. 시작 상태와 목표 상태

### 0-1. 지금 있는 것

| 대상 | 상태 | 처분 |
|---|---|---|
| `CLAUDE.md` | 273줄. 서술과 규칙이 섞여 있음 | 분리 |
| `docs/ARCHITECTURE.md` | 216줄. 파일:줄 번호·스키마표·프로토콜 매핑표 포함 | 재작성 |
| `docs/plans/active/` | 비어 있음 | 삭제 |
| `docs/plans/completed/` | `verification-infra.md` 1개 (139줄) | `docs/work/`로 |
| `docs/decisions/` | 3개. 프론트매터 없음. 무날짜 중복본 1개 | MADR 전환 |
| `docs/tech-debt.md` | 281줄. D-01~D-23. 수정 완료 3건 | scope 추가 |
| `docs/reports/` | 사람용 HTML 리포트 3개 | 유지 |
| `docs/references/` | 1개 | 유지 |
| `prompts/` | 하네스 초기 구축 부산물 2개 | 삭제 |
| `.claude/hooks/` | `guard_dangerous_cmd.py` + 테스트 + 로그 | 유지 |
| `.claude/agents/` `commands/` `skills/` | 빈 README 자리표시자 4개 | 삭제 |
| `.claude/transcripts-archive/` | jsonl 3개, gitignore | 유지 |
| `Server/GameServerTests/` | 4스위트 14개 통과 + DISABLED 1 | 유지 |
| AuthServer `npm test` | `configs.test.js` 2개 통과 | 유지 |

**테스트가 도는 범위**

| 대상 | 러너 | 커버 |
|---|---|---|
| GameServer 순수 로직 | gtest, 종료 코드 | Inventory 슬롯 매핑·removeItem, protobuf 직렬화, 프로토콜 ID 불변식 |
| AuthServer 설정 | `node --test` | `.env` 6키 존재, 풀 크기 파싱 |
| GameServer Room·DB·IOCP | 없음 | 0 |
| AuthServer 라우터·인증 | 없음 | 0 |
| UE 클라 | 없음 | 0 |

### 0-2. 목표 트리

```
CLAUDE.md                        # 규칙 + 목차. 100줄 이하
README.md                        # 기존 유지
docs/
├── ARCHITECTURE.md              # 불변식 중심. 코드에서 읽을 수 있는 것은 없음
├── NEXT.md                      # 미래를 말하는 유일한 문서
├── harness.md                   # 스킬 목록 + 판단 기준
├── tech-debt.md                 # D-NN. scope 필드 추가
├── build.md                     # CLAUDE.md 에서 분리
├── codegen.md                   # CLAUDE.md 에서 분리
├── testing.md                   # CLAUDE.md 에서 분리
├── decisions/                   # MADR. 채택 후 불변
│   ├── index.md
│   ├── adr-template.md
│   ├── 0001-harness-verdict.md
│   ├── 0002-l1-test-infra.md
│   └── 0003-....md
├── work/                        # 작업 기록
│   └── 2026-08-27-l1-test-infra.md
├── reports/                     # 사람용 설명 자료
└── references/
.claude/
├── settings.json
├── settings.local.json
├── hooks/
│   ├── guard_dangerous_cmd.py
│   ├── test_guard_dangerous_cmd.py
│   └── guard_test_files.py      # 4단계
├── agents/
│   └── test-auditor.md          # 5단계
├── skills/
│   ├── new-work/SKILL.md        # 2단계
│   ├── end-session/SKILL.md     # 2단계
│   ├── new-adr/SKILL.md         # 1d
│   └── project/
│       ├── tdd-slice/SKILL.md          # 4단계
│       └── three-tier-change/SKILL.md   # 5단계
└── transcripts-archive/
CONTEXT.md                       # tdd 스킬용 얇은 포인터
```

### 0-3. 시제로 나눈 문서

| 시제 | 문서 | 답하는 질문 |
|---|---|---|
| 미래 | `docs/NEXT.md` | 다음에 무엇을 왜 |
| 현재 (좋은 것) | `docs/ARCHITECTURE.md` | 깨면 안 되는 것은 무엇인가 |
| 현재 (나쁜 것) | `docs/tech-debt.md` | 지금 무엇이 틀렸나 |
| 과거 (무엇) | `docs/work/` | 무엇을 어떻게 했나 |
| 과거 (왜) | `docs/decisions/` | 왜 그 방식을 골랐나 |
| 상시 | `CLAUDE.md` | 매 세션 지켜야 할 규칙 |
| 상시 (얇게) | `docs/harness.md` | 어떤 스킬이 언제 도나 |
| 참조 | `docs/{build,codegen,testing}.md` | 그 영역을 건드릴 때만 |
| 사람용 | `docs/reports/` | 이 결정을 어떻게 이해하나 |

미래를 말하는 문서는 `NEXT.md` 하나다. work 파일에 「다음에 할 것」을 적지 않고, tech-debt에 순서를 적지 않고, 세션 프롬프트를 따로 커밋하지 않는다.

### 0-4. 단계 개요

| 단계 | 내용 | 소요 |
|---|---|---|
| 1 | 문서 골격 (1a~1e) | 2~3일 |
| 2 | `new-work` · `end-session` 스킬 | 반나절 |
| 3 | 첫 TDD 슬라이스 (D-22 + D-23), 수동 | 1세션 |
| 4 | `tdd-slice` 스킬 + 테스트 잠금 훅 | 반나절 |
| 5 | D-14 3티어 + `three-tier-change` + `test-auditor` | 1~2세션 |
| 6 | Room seam 만들기 | 2~4주 |

3단계를 수동으로 두는 이유는 `tdd-slice`가 무엇을 감쌀지 한 번 돌려봐야 정해지기 때문이다. vcxproj 등록, 빌드 폴링, 종료 코드 판정 중 어디가 실제로 반복되는지 모르는 상태에서 스킬을 만들면 잘못된 것을 고정한다.

---

# 1단계. 문서 골격

## 1a. 폴더와 양식

### 명령

```bash
git mv docs/plans docs/work
git mv docs/work/completed/verification-infra.md \
       docs/work/2026-08-27-l1-test-infra.md
rmdir docs/work/active docs/work/completed

git rm -r prompts/
git rm .claude/agents/README.md .claude/commands/README.md
git rm .claude/skills/project/README.md .claude/skills/unreal-core/README.md
```

`.claude/skills/unreal-core/`는 폴더째 사라진다. 넣을 것이 생기면 그때 만든다.

### work 파일 양식

```markdown
---
작업: l1-test-infra
세션: 2
날짜: 2026-08-27
상태: 완료                  # 진행중 / 완료 / 폐기
관련: decisions/0002-l1-test-infra.md · tech-debt D-01, D-15
---

# L1 테스트 인프라 구축 + 첫 실전 버그(D-01)

표기: `[x]` 완료(옆에 summary 한 줄) · `[B] blocked — 무엇이/어디서`

## 왜 이 작업인가
막혀 있던 것을 번호로. 코드베이스를 읽기 전에 쓴다.

## 결정
선택지가 여럿이었을 때만. 각 항목:
### <결정 제목>
**채택**: ... **기각**: ...
**근거**: ...
**틀렸다는 신호**: 무엇을 보면 이 결정이 틀렸다고 알 수 있는가
**승격**: ADR-00NN (다른 작업에도 영향을 주면)

## 위험 요소
이 변경이 무엇을 깨뜨릴 수 있는가. 어느 Phase 가 가장 위험한가.
검토했지만 하지 않기로 한 방법.

## 증명
각 Phase 완료를 무엇으로 확인하는가. 실행 가능한 검사로 적는다.
사전에 정한다. 작업이 끝난 뒤 채우지 않는다.

## Phase A — <묶음 이름>
- [x] **A-1 <항목>** — summary 한 줄 (파일 경로 + 핵심 결정)
- [x] **<항목>** *(계획에 없던 항목)* — 왜 필요했는지
- [B] **blocked — 무엇이 / 어디서** (에러 원문 + 근거 file:line)

## 작업 중 기록
- YYYY-MM-DD: 계획에서 벗어난 이유, 막힌 지점, tech-debt 로 보낸 것
```

기존 `verification-infra.md`에 프론트매터를 붙이고, 「다음 세션 후보」 절을 잘라 `NEXT.md`로 옮긴다. 본문은 그대로 둔다.

**「위험 요소」와 「증명」이 새로 생긴 절이다.** 기존 양식은 이 둘이 Phase 항목 안에 녹아 있었다. TDD 루프에서 slice 시작 전에 "무엇이 통과하면 끝인가"가 없으면 seam 승인이 성립하지 않으므로 절로 분리한다.

### 파일명

```
docs/work/YYYY-MM-DD-{작업명}.md
```

날짜가 앞이라 정렬이 시간순이고 `decisions/`와 규칙이 같다. 상태는 폴더가 아니라 프론트매터로 표현한다.

### tech-debt scope 추가

항목 헤더에 scope를 넣는다.

```markdown
### D-22. `Inventory`가 검증 없는 인덱싱으로 널 역참조에 열려 있다
`[심각도: 하] [난이도: 중] [scope: server]` · `Server/GameServer/Game/System/Inventory.cpp:44`

- **왜 문제인가** — ...
- **어디로 갈 것인가** — ...
- **선행 조건** — (하드 의존이 있을 때만)
```

scope 값은 `client` `server` `protocol` `shared` `build` `ops`. `protocol`은 클라와 서버를 동시에 구속한다는 뜻이다. D-14가 여기 해당한다.

**23개 전부에 「선행 조건」을 소급하지 않는다.** 대부분 "모름"이 된다. 순서는 `NEXT.md`가 담고 tech-debt는 카탈로그로 남는다. scope는 23개 전부에 붙인다. `파일:줄`에서 유추 가능하지만 기계가 읽을 수 없다.

---

## 1b. `CLAUDE.md` 분리

### 처분

| 절 | 처분 |
|---|---|
| 프로젝트 개요 (3티어 표) | 유지 |
| 도구 라우팅 | 유지 |
| 완료 기준 | 유지 |
| 안전 | 유지 |
| 작업 방식 | 유지 (개정) |
| 컨벤션 | 인코딩·커밋만 유지. 나머지 → `build.md` |
| 코드 생성 | → `docs/codegen.md` |
| 빌드 및 실행 | → `docs/build.md`. 엔진 제약 경고 한 줄만 남김 |
| 테스트 계층 | → `docs/testing.md`. 실행 명령표만 남김 |
| 아키텍처 핵심 규칙 | 4줄 요약 + 포인터 |
| 기존 코드베이스 방침 | 유지 |

목표 100줄 이하.

### 추가할 절

```markdown
## 문서 위치
- docs/NEXT.md — 다음에 무엇을 왜. **세션 시작 시 읽는다.**
- docs/harness.md — 스킬 목록과 판단 기준
- docs/ARCHITECTURE.md — 깨면 안 되는 불변식
- docs/tech-debt.md — 알려진 부채 (D-NN)
- docs/decisions/index.md — ADR 표. 본문은 필요한 것만 연다
- docs/work/ — 세션별 작업 기록
- docs/{build,codegen,testing}.md — 해당 영역 건드릴 때만

## 아키텍처 결정 (ADR)
코드에 드러나지 않는 제약이 docs/decisions/ 에 있다. 추측하지 말고 읽는다.

- 다음 작업 전에는 docs/decisions/index.md 를 읽는다:
  프로토콜 변경, 서버 스레딩·동시성, 클라-서버 경계를 넘는 타입 추가,
  빌드 구성 변경, 새 서드파티 의존성 도입.
- 인덱스 표에서 해당 scope 의 ADR만 골라 본문을 연다. 전부 열지 않는다.
- 각 ADR의 Confirmation 절이 실제 준수 항목이다. 코드를 쓰기 전에 읽고,
  작업을 마친 뒤 그 항목으로 자체 점검한 결과를 보고한다.
- status: accepted 만 현행이다. superseded / deprecated / rejected 는
  이력이므로 따르지 않는다.
- 되돌리기 비싼 결정을 하게 되거나 기존 제약을 어겨야 하면 코드를 먼저 쓰지 말고
  알린다. 초안은 status: proposed 로 두고 승인 없이 accepted 로 바꾸지 않는다.

## 작업 방식
- 세션 시작: docs/NEXT.md 를 읽는다.
- 복잡한 작업은 new-work 스킬로 시작한다. 코드베이스를 읽기 전에
  「왜 이 작업인가」와 「결정」을 먼저 쓴다.
- **막힌 항목은 지우지 않는다.** `[B] blocked — 무엇이/어디서` 로 표기하고 멈춘다.
- **완료 항목 옆에는 summary 한 줄** (파일 경로 + 핵심 결정).
- 발견한 부채는 즉시 고치지 말고 docs/tech-debt.md 에 기록한다.
- 세션 종료: end-session 스킬을 돈다. NEXT.md 갱신이 마지막이다.
- 같은 실수가 두 번이면 이 파일에 규칙 한 줄, 세 번이면 훅 승격을 제안한다.
```

### 커밋 규칙 (유지, 명시)

gitmoji + 한국어: `<emoji> <type>: <설명>`

| 접두어 | 용도 |
|---|---|
| `🚧 wip:` | 진행 중 |
| `✅ done:` | 완료 |
| `📝 plan:` | 계획·의도 문서 |
| `♻️ refactor:` | 리팩토링 |
| `🧹 clean:` | 정리 |
| `🎉 add:` | 추가 |

`git commit -F <파일>`을 쓴다. 훅이 명령 문자열 전체를 보므로 위험 패턴을 언급만 하는 메시지도 차단된다. `-m`과 heredoc은 메시지가 명령 문자열에 들어간다.

브랜치 `#<issue>-<Topic>`, PR로 `dev`에 머지.

---

## 1c. `NEXT.md` 신설

```markdown
# 다음에 무엇을

최종 갱신: YYYY-MM-DD (세션 N 종료 시)

## 지금 하는 것
docs/work/2026-09-10-inventory-cleanup.md — D-22 + D-23
(없으면 "없음")

## 다음 후보 (우선순위 순)

### 1. <제목>
**왜 지금**: 실측한 근거만. 추측은 적지 않는다.
**선행 조건**: 없으면 "없음"
**완료 신호**: 무엇이 바뀌면 끝인가

### 2. ...

## 하지 않기로 확인된 것
- <항목> — 근거 (ADR 번호 또는 work 파일)
```

### 규칙

- 후보는 5개를 넘기지 않는다. 넘으면 tech-debt에 있어야 할 것이 섞인 것이다.
- 「왜 지금」에는 실측한 것만 적는다.
- 세션 프롬프트는 여기서 생성한다. 따로 커밋하지 않는다.

### 첫 내용

`verification-infra.md`의 「다음 세션 후보」 6개를 옮긴다. 우선순위와 근거가 이미 붙어 있다. 「하지 말 것으로 확인된 것」은 「하지 않기로 확인된 것」으로 간다.

---

## 1d. MADR 전환

### 왜

현 ADR 3개에 세 가지가 없다.

- **Confirmation** — 결정이 지켜지는지 확인할 방법. 이 프로젝트는 종료 코드·린트·빌드라는 수단이 이미 있는데 ADR에 묶여 있지 않다.
- **status 프론트매터** — 무날짜 `final-harness-verdict.md`가 `2026-08-19`의 선행본인데 파일 안에 표시가 없다.
- **scope** — 3티어 프로젝트인데 어느 티어를 구속하는지가 없다.

### 파일 배치

```
docs/decisions/
├── index.md
├── adr-template.md
├── 0001-harness-verdict.md      # 2026-08-19-final-harness-verdict.md
├── 0002-l1-test-infra.md        # 2026-08-27-l1-test-infra.md
└── 0000-superseded-harness-verdict.md   # 무날짜 구본
```

날짜 접두어에서 번호 접두어로 바꾼다. 순서가 시간순이면서 참조가 짧아진다(`ADR-0002`).

### 템플릿

`docs/decisions/adr-template.md`:

```markdown
---
# 이 블록은 기계가 읽는 메타데이터다. 값을 임의로 늘리지 않는다.
# status : proposed | accepted | rejected | deprecated | superseded
# scope  : client | server | protocol | shared | build | ops
#          (여러 개면 배열. protocol 은 클라·서버를 동시에 구속한다는 뜻)
status: proposed
date: YYYY-MM-DD
scope: [server]
supersedes: null          # 예: ADR-0000
superseded-by: null       # 예: ADR-0007
---

# {해결한 문제와 채택한 해법이 함께 드러나는 짧은 제목}

## Context and Problem Statement

{2~3문장. 가능하면 질문 형태로. 이 결정이 어느 부분(모듈, 레이어,
프로세스 경계)에 걸리는지 명시한다.}

## Decision Drivers

* {제약 또는 상충하는 힘. 측정 가능하면 수치로.}

## Considered Options

1. {채택안 — 맨 앞}
2. {대안}

<!-- 옵션은 같은 추상 수준이어야 한다.
     이 맥락에서 동작하지 않는 옵션을 들러리로 세우지 않는다. -->

## Decision Outcome

채택: "{옵션 1}". 이유는 {근거}.

### Consequences

* Good, because {…}
* Bad, because {…}

<!-- 단점 칸이 비어 있거나 무해한 것만 있으면 아직 미완성이다. -->

### Confirmation

<!-- 필수. 이 결정이 지켜지는지 코드에서 확인하는 방법.
     자동 검사가 있으면 명령까지. 없으면 리뷰 시 확인할 구체적 지점. -->

* {예: GameServerTests 가 config.h 없이 빌드된다 — 빌드 후 종료 코드 0}

## Pros and Cons of the Options

### {옵션 1}
* Good, because {…}
* Bad, because {…}

## More Information

{확신 수준과 이유, 재검토 조건, 관련 ADR.}
```

MADR full에서 `decision-makers` `consulted` `informed`를 뺐다. 혼자 하는 프로젝트라 전부 같은 값이다.

### `index.md`

```markdown
# Architecture Decision Log

코드를 읽어도 알 수 없는 제약과 그 근거만 남긴다.
설계 문서도, 구현 가이드도 아니다.

## 읽기 규칙

1. 작업 시작 전에 이 표를 먼저 읽는다. 개별 ADR 본문은 관련 있는 것만 연다.
   전부 컨텍스트에 올리지 않는다.
2. `status: accepted` 인 ADR만 현행 제약이다.
   `superseded` / `deprecated` / `rejected` 는 이력이며 따르지 않는다.
3. 각 ADR의 `Confirmation` 절이 실제로 지켜야 하는 항목이다.
4. 채택된 ADR은 수정하지 않는다. 결정이 바뀌면 새 번호로 쓰고
   `superseded-by` 와 `supersedes` 를 상호 링크한다. 이 표도 갱신한다.

## 언제 ADR을 쓰는가

- 되돌리는 데 비용이 큰 결정 (프로세스 경계, 스레딩 모델, 직렬화 포맷, 저장소)
- 코드만 봐서는 "왜 이렇게 안 했는지" 알 수 없는 결정
- 같은 논쟁이 두 번 이상 반복된 주제

**쓰지 않는 것**: 함수 하나의 에러 처리, 모듈의 재시도 로직, 네이밍.
구현 수준 결정은 커밋 메시지와 코드 주석의 영역이다.

## 결정 목록

| ID | 제목 | Scope | Status | 한 줄 요약 |
|---|---|---|---|---|
| [0000](0000-superseded-harness-verdict.md) | 하네스 판정표 (구본) | build | superseded | ADR-0001 이 대체 |
| [0001](0001-harness-verdict.md) | 하네스 판정표 최종 확정 | build | accepted | 도구·실행 경로 판정 |
| [0002](0002-l1-test-infra.md) | L1 테스트 인프라 | build, server | accepted | gtest 벤더링, 판정은 종료 코드, UE LLT 기각 |

## 새 ADR 만들기

new-adr 스킬을 쓴다. 없으면:

    cp docs/decisions/adr-template.md docs/decisions/000N-title-with-dashes.md

파일명은 `NNNN-title-with-dashes.md` (소문자, 대시). 작성 후 위 표에 한 줄 추가.
```

### 기존 3개 처리

**프론트매터와 Confirmation만 추가하고 본문은 건드리지 않는다.** ADR은 수정하지 않는 문서이므로 소급 개작은 그 규율과 충돌한다. 메타데이터 추가는 예외로 두되 예외임을 명시한다. 각 파일 상단에:

```markdown
> 2026-09-XX MADR 전환 시 프론트매터와 Confirmation 절만 추가. 본문 미변경.
```

무날짜 구본은 삭제하지 않는다. `0000`으로 번호를 주고 `status: superseded`, `superseded-by: ADR-0001`로 표시한다. 삭제는 이력을 지우는 것이고 ADR의 존재 이유가 이력이다.

### Confirmation 채우기 예시

ADR-0002(L1 테스트 인프라)의 결정 6건에 붙일 것:

```markdown
### Confirmation

* GameServerTests 가 config.h 없이 빌드된다 —
  build_solution_start(rootFolder=".../Server") 후 종료 코드 0
* 판정은 종료 코드다. 로그 문자열을 파싱해 성공을 판단하는 코드가 없다.
* 테스트 .cpp 는 GameServerTests.vcxproj 의
  <ItemGroup Label="테스트 소스"> 에 등재되어 있다.
* AuthServer 는 새 테스트 의존성을 추가하지 않는다 — package.json 의
  devDependencies 에 테스트 러너가 없다 (node --test 내장).
* P1Tests 타깃을 만들지 않는다 — 설치본 엔진이 거부한다.
```

### 주의

참고로 받은 예시 ADR 묶음은 **이 프로젝트 것이 아니다.** flatbuffers IDL과 Zone 단위 스레딩을 전제하는데 이 프로젝트는 protobuf와 Room이다. 형식만 가져오고 내용은 가져오지 않는다.

### `new-adr` 스킬

`.claude/skills/new-adr/SKILL.md`:

```markdown
---
name: new-adr
description: 아키텍처 결정 기록(ADR)을 새로 작성한다. 되돌리기 비싼 결정을
  내렸을 때, 기존 ADR을 대체해야 할 때, work 파일의 결정을 승격할 때 사용한다.
---
# ADR 작성

## 먼저 확인

이 결정이 ADR 대상인지 판단한다. 아래 중 하나에 해당해야 한다.

- 되돌리는 데 비용이 큰 결정 (프로세스 경계, 스레딩 모델, 직렬화 포맷, 저장소)
- 코드만 봐서는 "왜 이렇게 안 했는지" 알 수 없는 결정
- 같은 논쟁이 두 번 이상 반복된 주제

아니면 work 파일의 「결정」 절에 남기고 여기서 멈춘다.
함수 하나의 에러 처리, 모듈의 재시도 로직, 네이밍은 ADR 대상이 아니다.

## 절차

1. `docs/decisions/index.md` 를 읽어 다음 번호를 확인한다.
2. `adr-template.md` 를 `NNNN-title-with-dashes.md` 로 복사한다.
3. 채운다. 아래 규칙을 지킨다.
4. `index.md` 표에 한 줄 추가한다.
5. `status: proposed` 로 두고 멈춘다. **승인 없이 accepted 로 바꾸지 않는다.**

## 작성 규칙

- **옵션은 최소 두 개**, 같은 추상 수준이어야 한다.
  기술과 제품을 비교하거나 아키텍처 스타일을 프로토콜 명세와 비교하지 않는다.
- **이 맥락에서 동작하지 않는 옵션을 들러리로 세우지 않는다.**
- **Consequences 의 Bad 칸을 비우지 않는다.** 장기적으로만 드러나는 비용
  (운영, 유지보수, 재작업)을 특히 챙긴다.
- **Confirmation 은 필수다.** 자동 검사가 있으면 실행 명령까지 적는다.
  없으면 리뷰 시 확인할 구체적 지점을 적는다. "코드 리뷰로 확인"만으로는 부족하다.
- **확신 수준을 More Information 에 밝힌다.** 낮아도 된다.
- 분량은 몇 페이지가 상한이다. 컴포넌트 책임과 협력 관계, 다이어그램,
  코드 스니펫을 밀어 넣지 않는다.

## 기존 결정을 대체할 때

옛 ADR을 수정하지 않는다. 새 번호로 쓰고 양쪽을 상호 링크한다.

- 새 ADR: `supersedes: ADR-00NN`
- 옛 ADR: `status: superseded`, `superseded-by: ADR-00MM`
- `index.md` 의 두 줄을 모두 갱신한다.
```

---

## 1e. `ARCHITECTURE.md` 재작성

### 원칙

- 코드를 읽어서 알 수 있는 것은 적지 않는다.
- **파일:줄 번호를 적지 않는다.** 심볼 이름만 적는다. 줄 번호는 코드 한 줄만 늘어도 틀린다.
- 자주 바뀌는 수치(줄 수, 위젯 개수, 메시지 개수)를 적지 않는다.
- 불변식을 명시적으로 적는다. 중요한 불변식은 대개 **무언가의 부재**로 표현된다.
- 계층과 시스템 사이의 경계를 짚는다.

### 뺄 것

| 내용 | 이유 |
|---|---|
| DB 스키마 표 | SQL 스크립트가 원본. 요약도 불필요 |
| 프로토콜 40개 매핑표 | 생성물에서 유도됨 |
| 클라 폴더 트리 | 코드에서 읽힘 |
| 파일:줄 번호 전부 | 썩는다 |
| 레벨별 BP 유무 표 5행 | 불변식 한 줄 + 확인 방법으로 압축 |
| 예외 2건 상세 | tech-debt D-07 참조로 |
| 「이렇게 안 한 이유」 | ADR로 나가 있음. `index.md` 링크만 |

### 살릴 불변식

현 문서에서 추출한다. 이것이 재작성의 실질이다.

1. 게임 서버는 `UserDB`를 모른다. 인증 티어와의 접점은 Redis 토큰 하나뿐이다.
2. 룸 소유 상태 변경은 룸 큐 위에서만 일어난다. 락이 없다. (예외 2건 = D-07)
3. 네트워크 스레드는 UObject를 만지지 않는다.
4. `HandleRecvPackets`를 부르는 C++ 코드는 없다. 레벨 BP의 `ReceiveTick`만 부른다.
5. 서버 오브젝트는 상태를 protobuf 메시지로 직접 들고 있다. 복제가 변환이 아니라 복사다.
6. 클라 폴더가 도메인이 아니라 UE 타입으로 갈려 있다.
7. `PrivateIncludePaths`가 include를 평탄하게 만들어 경계 위반을 빌드가 잡지 않는다.
8. 생성물은 XCOPY 사본이다. 직접 고치면 다음 생성 때 덮어써진다.
9. `Users.user_id`는 INT, `Characters.user_id`와 `GameSession::userId`는 BIGINT다.

6·7·9는 문제 서술이므로 tech-debt 참조를 붙인다.

### 골격

```markdown
# Architecture

이 문서는 코드를 읽어서 알 수 있는 내용을 반복하지 않는다.
왜 이렇게 되어 있는지, 그리고 깨면 안 되는 것만 적는다.

최종 수정: 2026-09-XX
검증: 세션 N 에서 실코드·라이브 DB·UE 에디터와 대조

## Bird's Eye View

UE5 클라이언트 · 외부 IOCP C++ 게임 서버 · Node 인증 서버의 3티어 MMORPG.

ground state 는 UserDB·GameDB 의 행과 Redis 의 액세스 토큰이다.
룸 안 오브젝트 상태는 derived 다 — 재접속으로 재생성된다. 진실의 원천으로 삼지 않는다.

## Code Map

### Server/AuthServer

계정, 로그인, 토큰 발급. bcrypt 해시 검증 후 UUID 토큰을 Redis 에 TTL 과 함께 넣는다.

**Architecture Invariant:** 게임 상태를 모른다. GameDB 에 접근하지 않는다.
**API Boundary:** HTTP. 게임 서버와의 유일한 접점은 Redis 의 토큰 키다.

### Server/GameServer

**Architecture Invariant:** UserDB 를 모른다. 인증 티어와의 접점은 Redis 토큰 하나뿐이다.
**Architecture Invariant:** 룸 소유 상태 변경은 룸 큐 위에서만 일어난다. 락이 없다.
   핸들러는 잡을 밀어넣고 리턴한다. 예외 2건은 tech-debt D-07.
**Architecture Invariant:** 서버 오브젝트는 상태를 protobuf 메시지로 직접 들고 있다.
   복제가 변환이 아니라 복사다. 게임플레이 변경은 대개 클라·서버·프로토콜 3곳을 건드린다.
**API Boundary:** 패킷 핸들러. C_* 로 진입한다. 인라인으로 일하지 않는다.

### P1 (UE 클라이언트)

**Architecture Invariant:** 네트워크 스레드는 UObject 를 만지지 않는다.
   수신 바이트는 큐에 쌓이고 게임 스레드가 비운다.
**Architecture Invariant:** 수신 펌프를 부르는 C++ 코드는 없다.
   레벨 블루프린트의 ReceiveTick 만 부른다. 새 레벨을 만들면 손으로 넣어야 하고,
   잊으면 조용히 죽는다. 확인: 레벨 BP 에 해당 노드가 있는가.
**Architecture Invariant:** 폴더가 도메인이 아니라 UE 타입으로 갈려 있다.
   PrivateIncludePaths 가 include 를 평탄하게 만들어 경계 위반을 빌드가 잡지 않는다.
   (tech-debt 참조)

### 생성물

**Architecture Invariant:** 패킷 정의와 게임 데이터 JSON 은 생성물이다.
   복사본을 직접 고치면 다음 생성 때 덮어써진다.
   손으로 쓰는 것은 핸들러 .cpp 뿐이다. 절차는 docs/codegen.md.

## Layering Rules

클라와 서버의 클래스 계층이 대칭이다: Object → Creature → { Player, Monster }.
한쪽만 고치면 어긋난다.

교차 티어 타입 불일치가 하나 있다 — 계정 id 의 폭이 티어마다 다르다. (tech-debt 참조)

## Cross-Cutting Concerns

### 코드 생성
상세: docs/codegen.md

### 테스트
판정은 종료 코드다. 로그 문자열로 성공을 판단하지 않는다.
계층별 실행 경로: docs/testing.md

### 인코딩
.proto 와 .bat 은 cp949, 나머지는 UTF-8. UTF-8 로 읽으면 깨져 보이는 것이 정상이다.

### 빌드 환경
런처 설치본 엔진 고정. 엔진 소스 패치는 하지 않는다. 상세: docs/build.md

## 이렇게 안 한 이유

docs/decisions/index.md 참조.
```

---

## 1단계 커밋

```bash
cat > /tmp/msg.txt <<'EOF'
🧹 clean: 하네스 문서 체계 전환

- plans/{active,completed} → work/ 평탄화, 상태는 프론트매터로
- NEXT.md 신설: 미래 정보를 한곳으로
- prompts/ · 빈 README 4개 제거
- CLAUDE.md 273줄 → build/codegen/testing.md 분리
- ADR을 MADR 형식으로 전환, Confirmation 필수화
- ARCHITECTURE.md 재작성: 불변식 중심, 줄 번호·스키마표·프로토콜표 제거
- tech-debt 항목에 scope 필드 추가
EOF
git commit -F /tmp/msg.txt
```

## 1단계 검증

```bash
wc -l CLAUDE.md                            # 100 이하
Server/Binary/Debug/GameServerTests.exe    # 14 passed, exit 0
cd Server/AuthServer && npm test           # 2 passed, exit 0
py -3 .claude/hooks/test_guard_dangerous_cmd.py
grep -rn "\.cpp:[0-9]" docs/ARCHITECTURE.md   # 결과 없음
```

새 세션을 열어 `NEXT.md`만 읽고 다음에 뭘 할지 알 수 있는지 확인한다.

---

# 2단계. 기본 스킬

## `.claude/skills/new-work/SKILL.md`

```markdown
---
name: new-work
description: 새 작업을 시작할 때 work 파일을 만든다. 며칠 걸리는 작업,
  되물을 것이 셋 이상인 작업, 되돌리기 어려운 변경을 시작할 때 사용한다.
---
# work 파일 생성

세 스텝이다. **순서를 지킨다.** 스텝 3에서만 코드베이스를 읽는다.

플랜 모드에서 코드를 잔뜩 읽은 세션이 스텝 1을 쓰면 구현 가능성에 이끌려
문제를 축소한다. "지금 구조로는 어려우니 범위를 줄이자" 가 문제 정의에
섞여 들어온다. 그래서 순서를 지킨다.

## 시작 전

`docs/NEXT.md` 를 읽는다. 이 작업이 후보 목록에 있으면 「왜 지금」과
「완료 신호」를 스텝 1과 3의 재료로 쓴다.

파일 경로: `docs/work/YYYY-MM-DD-{작업명}.md`

---

## 스텝 1 — 「왜 이 작업인가」

**도구**: 대화만. grill-me 스킬이 있으면 사용한다.
**코드베이스**: 읽지 않는다.
**종료 조건**: 되물을 것이 없어진다.

사용자에게 되묻는다. 한 번에 하나씩. 아래가 다 나올 때까지 멈추지 않는다.

- 지금 무엇이 안 되는가 (현상. 해결책이 아니라)
- 나아진 상태는 어떤 모습인가 (결과. 구현 방법이 아니라)
- 어느 화면·모듈·데이터가 건드려지는가
- 반드시 지켜야 할 것 / 하지 말아야 할 것
- 지금 답을 모르는 것

마지막 항목이 이 스텝의 핵심이다. 사용자가 아직 정하지 않은 것을 드러내는 것이
목적이지, 문서를 채우는 것이 목적이 아니다.

**사용자가 직접 읽고 고칠 때까지 다음 스텝으로 넘어가지 않는다.**

---

## 스텝 2 — 「결정」

**도구**: 대화. 코드는 필요 최소한만.
**종료 조건**: 각 옵션에 Good/Bad 가 붙는다.

선택지가 하나뿐이면 이 절을 만들지 않고 스텝 3으로 간다.

선택지가 여럿이면 각각에 대해:

    ### <결정 제목>
    **채택**: ...
    **기각**: ... (왜 아닌지)
    **근거**: ...
    **틀렸다는 신호**: 무엇을 보면 이 결정이 틀렸다고 알 수 있는가
    **승격**: ADR-00NN 또는 "없음"

규칙:

- 옵션은 같은 추상 수준이어야 한다.
- 동작하지 않는 옵션을 들러리로 세우지 않는다.
- **「틀렸다는 신호」를 비우지 않는다.** 결정이 옳은지 지금 판단할 수 없어도,
  무엇을 보면 틀렸는지는 지금 적을 수 있다.

### ADR 승격 판단

> 다음 작업을 할 때 이 결정을 다시 봐야 하나?

봐야 하면 new-adr 스킬로 ADR을 쓰고 여기에는 번호만 남긴다.
안 봐도 되면 여기에 전문을 남긴다. 애매하면 여기에 남긴다.

---

### 커밋 1

    git add docs/work/YYYY-MM-DD-{작업명}.md
    git commit -F <메시지 파일>   # 📝 plan: {작업} 의도와 결정

**플랜 모드 진입 전에 커밋한다.** 코드를 읽기 전 상태를 고정하는 것이 목적이다.
스텝 3을 다시 짜도 앞부분이 흔들리지 않는다.

스텝 1과 2가 30분 안에 끝났으면 커밋 2와 합쳐도 된다.
다만 플랜 모드 진입 전이라는 경계는 지킨다.

---

## 스텝 3 — 「위험 요소」·「증명」·Phase

**도구**: 플랜 모드 (Shift+Tab). 파일을 읽되 고치지 않는다.
**종료 조건**: 이 대화를 한 번도 본 적 없는 사람이 계획만 보고 구현할 수 있다.

### 3-1. Phase 목록

작업을 Phase 로 묶고 각 Phase 를 항목으로 나눈다.
바뀌는 파일 경로를 항목에 적는다.

### 3-2. 계획을 심문한다

세 가지를 묻고 답을 「위험 요소」에 적는다.

    이 계획에서 뭐가 깨질 수 있어?
    어느 Phase 가 가장 위험해?
    고려했지만 하지 않기로 한 다른 방법이 있었어?

형식적으로 묻지 않는다. 답에서 새로 나온 것이 하나도 없으면
계획이 아직 얕은 것이다.

### 3-3. 「증명」을 채운다

각 Phase 완료를 무엇으로 확인하는지 **실행 가능한 검사**로 적는다.

- 테스트가 있는 영역: 어느 테스트가 통과해야 하는가
- 테스트가 없는 영역: 빌드 통과 + 무엇을 눈으로 확인하는가

**작업이 끝난 뒤에 채우지 않는다.** 사전에 정하는 것이 이 절의 존재 이유다.

### 3-4. tech-debt 확인

`docs/tech-debt.md` 에서 이 작업이 건드리는 파일의 D-NN 항목을 찾아
프론트매터 `관련:` 에 적는다. 같은 자리를 두 번 열지 않기 위해서다.

---

### 커밋 2

    git commit -F <메시지 파일>   # 📝 plan: {작업} 실행 계획

**브랜치를 파기 전, main 에서 커밋한다.** 브랜치를 버려도 계획이 남는다.

그다음 브랜치를 판다.

    git checkout -b '#<issue>-<Topic>'

---

## 세션이 끊겼을 때

프론트매터 `상태:` 와 채워진 절로 판단한다.

| 상태 | 다음 |
|---|---|
| 「왜」만 있음 | 스텝 2 |
| 「결정」까지 있고 커밋 1 있음 | 스텝 3 |
| Phase 까지 있고 커밋 2 있음 | 구현 |
```

## `.claude/skills/end-session/SKILL.md`

```markdown
---
name: end-session
description: 작업 세션을 마무리한다. 작업이 끝났거나 중단할 때 사용한다.
  work 파일 마감, tech-debt 갱신, ADR 작성, NEXT.md 갱신을 순서대로 처리한다.
---
# 세션 종료

다섯 스텝이다. **마지막 스텝을 빠뜨리면 다음 세션의 진입점이 사라진다.**

## 1. work 파일 마감

- 프론트매터 `상태:` 를 `완료` / `진행중` / `폐기` 중 하나로
- 체크박스를 실제 상태로. **막힌 것은 `[B] blocked — 무엇이/어디서` 로 남긴다.**
  지우지 않는다.
- 완료 항목 옆에 summary 한 줄이 있는지 확인 (파일 경로 + 핵심 결정)
- 계획에 없던 항목에 *(계획에 없던 항목)* 표기가 있는지 확인
- 「작업 중 기록」에 계획 이탈·막힌 지점이 날짜와 함께 있는지 확인

**판정이 틀렸던 것이 있으면 지우지 말고 남긴다.** 무엇을 근거로 틀렸는지까지.
이것이 다음에 같은 실수를 막는 유일한 기록이다.

## 2. 「증명」 대조

work 파일의 「증명」 절을 읽고 각 항목을 **실제로 실행한다.**

- 실행 명령과 종료 코드를 「작업 중 기록」에 붙인다
- 자기신고로 대체하지 않는다
- 검증 불가한 항목이 있으면 불가하다고 명시한다

## 3. tech-debt 갱신

- 이번에 발견한 것을 D-NN 으로 추가.
  형식: `[심각도] [난이도] [scope]` · `파일:줄` · 왜 문제인가 / 어디로 갈 것인가
- 수정한 것은 제목에 *(세션 N 에서 수정 완료)* 를 붙이고
  불릿을 왜 문제였나 / 어떻게 고쳤나 / 남은 것 으로 바꾼다
- 난이도가 바뀐 것이 있으면 갱신하고 이유를 적는다
- 파일 최상단 「갱신 이력」 블록에 이번 세션 변경을 요약한다

## 4. ADR

work 파일 「결정」 절에 **승격**: ADR-00NN 표기가 있는데 파일이 없으면
new-adr 스킬로 작성한다.

승격 판단이 애매했던 것은 승격하지 않는다. ADR 개수가 늘면 각각이 가벼워진다.

## 5. NEXT.md 갱신

**이 스텝이 마지막이고 가장 중요하다.**

- 「지금 하는 것」을 이번 작업으로 바꾸거나, 끝났으면 "없음"
- 「다음 후보」를 다시 정렬한다. 각 후보에 세 항목이 있어야 한다:
  - **왜 지금** — 이번 세션에서 실측한 것만. 추측은 적지 않는다
  - **선행 조건** — 없으면 "없음"
  - **완료 신호** — 무엇이 바뀌면 끝인가
- 이번에 "하지 말 것"으로 확정된 것이 있으면
  「하지 않기로 확인된 것」에 근거(ADR 번호 또는 work 파일)와 함께 추가
- 후보가 5개를 넘으면 tech-debt 로 내려보낼 것이 섞인 것이다. 정리한다
- 최종 갱신 날짜와 세션 번호를 갱신한다

**work 파일에 「다음 세션 후보」를 적지 않는다.** 미래를 말하는 문서는 이것 하나다.

## 6. 커밋

    ✅ done: {작업} — {한 줄 요약}

work 파일, tech-debt, ADR, NEXT.md 를 한 커밋에 담는다.
코드는 별도 커밋이다.

## 마지막 확인

새 세션을 여는 사람이 `docs/NEXT.md` 하나만 읽고 다음에 뭘 할지 알 수 있는가.
아니면 5번으로 돌아간다.
```

## 2단계 검증

`new-work`를 실제로 한 번 돌려서 3단계용 work 파일을 만든다. 스킬이 스텝 3에서 플랜 모드로 전환하는지, 커밋을 두 번 요구하는지 확인한다.

---

# 3단계. 첫 TDD 슬라이스 (수동)

## 대상

`NEXT.md` 1번, D-22 + D-23.

첫 대상으로 적합한 이유:
- 그물이 이미 있다 (`InventoryTests.cpp`가 세 슬롯 타입 전부 검사)
- seam 이 명확하다 (`Inventory` 공개 API)
- 순수 로직이라 I/O 의존이 없다
- 둘이 같은 함수를 건드려 한 번에 처리된다

## 왜 수동인가

`tdd-slice` 스킬에 무엇을 감쌀지가 한 번 돌려봐야 정해진다. vcxproj 등록, 빌드 폴링, 종료 코드 판정 중 어디가 실제로 반복 부담인지 모르는 상태에서 스킬을 만들면 잘못된 것을 고정한다.

**이 세션에서 반복된 동작을 세어둔다.** 그것이 4단계 스킬의 내용이 된다.

## 루프

```
① 사람 — seam 승인
     이 슬라이스를 어디서 관찰할 것인가
     ↓
② 실패 테스트 1개
     승인된 seam 에서만. 이름은 명세처럼
     ↓
③ 러너 — 기대한 이유로 실패했나
     빌드 → 실행 → 출력 확인
     ↓
④ 테스트 커밋            ← 세션 2와 달라지는 지점
     🚧 wip: <슬라이스> 실패 테스트
     ↓
⑤ 최소 구현 → 그린
     테스트를 고치지 않는다
     ↓
   다음 슬라이스 (①로)
```

## seam

관찰 가능한 공개 경계. 여기서 테스트하면 내부를 전부 다시 써도 동작이 살아남는다.

**합의하지 않은 seam 에는 테스트를 쓰지 않는다.** 사람이 먼저 승인한다.

이 프로젝트의 seam 상태:

| seam | 상태 |
|---|---|
| `Inventory` 공개 API | 존재. 현재 그물이 여기 |
| protobuf 메시지 왕복 | 존재. 리플렉션으로 자동 확장 |
| 프로토콜 ID 목록 | 존재 |
| 전투 판정 | **없음.** `Room` 안에 얽혀 있다 |
| Gamedata 테이블 로딩 | 미확인 |

seam 이 없으면 만드는 작업이 선행되고, 그것은 리팩토링이므로 별도 work 파일이 필요하다.

## 실행

**빌드**: `build_solution_start(rootFolder=".../Server")` → `build_solution_state` 폴링. `rebuild=true` 를 쓰지 않는다. 터미널 MSBuild 를 쓰지 않는다(출력 절단).

**실행**: 셸. `Server/Binary/Debug/GameServerTests.exe`

**테스트 파일 추가 시**: `Server/GameServerTests/GameServerTests.vcxproj` 의 `<ItemGroup Label="테스트 소스">` 에 `.cpp` 를 등록한다. 자동 수집이 없다. 잊으면 테스트가 조용히 안 돌아간다.

## 실패 로그

work 파일 「작업 중 기록」에 슬라이스마다 몇 줄.

```markdown
- 2026-09-10 슬라이스 1 (Inventory 슬롯 인덱스 검증)
  RED: 범위 밖 slot_id 가 operator[] 로 직행. 기대한 이유로 실패 확인.
  GREEN: 인덱스 검증 추가. 15/15 통과, 종료 코드 0.
```

**"기대한 이유로 실패 확인" 이라는 문구가 핵심이다.** 그 판정이 일어났다는 흔적이다.

## 안티패턴 셋

러너가 잡지 못한다. 동어반복 테스트는 통과한다.

| 이름 | 뜻 |
|---|---|
| 구현 결합 | 내부 구현을 검사한다 |
| 동어반복 | 실질적으로 아무것도 주장하지 않는다 |
| 수평 슬라이싱 | 하나의 동작이 아니라 한 계층을 자른다 |

## 리팩터링

루프 안에서 하지 않는다. 슬라이스가 전부 끝난 뒤 리뷰에서 처리한다.

리뷰 프롬프트:

```
git diff main...HEAD 를 보고 docs/work/{작업}.md 와 대조해서 리뷰해줘.
- 계획에 없던 변경
- 계획에 있는데 빠진 것
- 버그와 논리 오류
- 보안 문제
스타일과 이름 짓기 지적은 최대 5개까지.
```

나온 스멜의 행선지:

| 스멜 | 어디로 |
|---|---|
| 이번 work 범위 안이고 30분 내 | 지금 고침 |
| 범위 밖 | `docs/tech-debt.md` (D-NN 부여) |

## 머지

```bash
git checkout main
git merge --squash '#<issue>-<Topic>'
git commit -F <메시지 파일>     # ✅ done: ...
git branch -D '#<issue>-<Topic>'
```

계획은 이미 main 에 있으므로 스쿼시 대상에 들어가지 않는다.

## 완료 신호

D-22·D-23 이 tech-debt 에서 수정 완료로 바뀌고 기존 14개가 유지된다.

---

# 4단계. 루프 고정

## 4a. `.claude/skills/project/tdd-slice/SKILL.md`

3단계에서 실제로 반복된 것을 채운다. 아래는 뼈대이며, 3단계 경험에 따라 조정한다.

```markdown
---
name: tdd-slice
description: 이 프로젝트에서 TDD 슬라이스를 하나 돈다. GameServer 순수 로직이나
  AuthServer 설정 레이어를 고칠 때 사용한다. Room·DB·IOCP·UE 클라에는 쓰지 않는다.
---
# TDD 슬라이스

## 적용 범위 확인

먼저 대상이 루프 가능한 영역인지 확인한다.

| 대상 | 러너 | 루프 |
|---|---|---|
| GameServer 순수 로직 | GameServerTests.exe | 가능 |
| AuthServer 설정 | npm test | 가능 |
| GameServer Room·DB·IOCP | 없음 | **불가** |
| AuthServer 라우터·인증 | 없음 (bcrypt+MSSQL+Redis 필요) | 비쌈 |
| UE 클라 | 없음 | **불가** |

불가 영역이면 이 스킬을 쓰지 않는다. 빌드 통과 + DummyClient 스모크로 간다.

## 1. seam 승인 (사람)

이 슬라이스를 어디서 관찰할 것인지 제안하고 **사용자 승인을 받는다.**
승인 없이 테스트를 쓰지 않는다.

seam 은 관찰 가능한 공개 경계다. 내부를 전부 다시 써도 동작이 살아남는 자리다.

제안할 때 함께 밝힌다:
- 이 seam 이 이미 존재하는가, 만들어야 하는가
- 만들어야 한다면 그것은 리팩토링이므로 별도 work 파일이 필요하다

## 2. 실패 테스트 1개

**한 번에 하나.** 여러 개를 한꺼번에 쓰지 않는다.

- 이름은 함수가 아니라 명세처럼 짓는다.
  `RemovingMiscItemDoesNotTouchGearInventory` 같은 형태.
- 승인된 seam 에서만 관찰한다. 내부 상태를 들여다보지 않는다.
- 도메인 용어는 CONTEXT.md 와 docs/ARCHITECTURE.md 를 따른다.

새 .cpp 파일이면 `Server/GameServerTests/GameServerTests.vcxproj` 의
`<ItemGroup Label="테스트 소스">` 에 등록한다. **자동 수집이 없다.**

## 3. 빨강 확인

빌드: build_solution_start(rootFolder=".../Server") → build_solution_state 폴링.
  rebuild=true 를 쓰지 않는다. 터미널 MSBuild 를 쓰지 않는다.
실행: 셸에서 Server/Binary/Debug/GameServerTests.exe

확인할 것: **기대한 이유로 실패하는가.**
컴파일 에러나 다른 이유로 실패했다면 빨강이 아니다. 고치고 다시 돈다.

work 파일 「작업 중 기록」에 적는다:

    - YYYY-MM-DD 슬라이스 N (<seam>)
      RED: <무엇이 어떻게 실패했는가>. 기대한 이유로 실패 확인.

## 4. 감사 (test-auditor 가 있으면)

`.claude/agents/test-auditor.md` 가 존재하면 호출한다.
없으면 스스로 세 가지를 확인하고 넘어간다.

- 구현 결합 — 내부 구현을 검사하고 있는가
- 동어반복 — 실질적으로 아무것도 주장하지 않는가
- 수평 슬라이싱 — 하나의 동작이 아니라 한 계층을 자르고 있는가

## 5. 테스트 커밋

    🚧 wip: <슬라이스 이름> 실패 테스트

**이 커밋을 건너뛰지 않는다.** 수정 전부터 테스트가 존재했고 되돌릴 수
없었다는 증거다.

잠금 훅이 있으면 `.claude/tdd-lock` 에 테스트 파일 경로를 쓴다.

## 6. 최소 구현

테스트를 통과시킨다. **테스트를 고치지 않는다.**
실패하는 테스트를 건너뛰거나 삭제하지 않는다.

초록이 되면 잠금을 푼다 (`.claude/tdd-lock` 삭제).

work 파일에 적는다:

    GREEN: <어떻게 고쳤는가>. N/N 통과, 종료 코드 0.

## 7. 다음 슬라이스 또는 종료

Phase 의 항목이 남았으면 1로 돌아간다.

전부 끝나면 **리팩터링을 여기서 하지 않는다.** 리뷰 단계로 넘긴다.

## 계획 밖 발견

즉석에서 고치지 않는다. `docs/tech-debt.md` 에 D-NN 으로 기록하고 넘어간다.
work 파일 「작업 중 기록」에 한 줄 남긴다.

구현이 계획에서 벗어나면 같은 커밋에서 work 파일의 Phase 도 갱신한다.
```

## 4b. 테스트 잠금 훅

`.claude/hooks/guard_test_files.py`:

```python
#!/usr/bin/env python3
"""TDD 슬라이스 중 테스트 파일 편집을 차단한다.

.claude/tdd-lock 에 적힌 경로(줄 단위)와 일치하는 파일의 Edit/Write 를 막는다.
잠금 파일이 없으면 통과시킨다.
"""
import json
import os
import sys
from pathlib import Path

LOCK = Path(__file__).resolve().parents[1] / "tdd-lock"


def main() -> int:
    try:
        payload = json.load(sys.stdin)
    except Exception:
        return 0  # 입력을 못 읽으면 통과. 훅이 작업을 막는 원인이 되지 않게 한다.

    if not LOCK.exists():
        return 0

    target = payload.get("tool_input", {}).get("file_path", "")
    if not target:
        return 0

    target_norm = os.path.normcase(os.path.normpath(target))

    for line in LOCK.read_text(encoding="utf-8").splitlines():
        entry = line.strip()
        if not entry or entry.startswith("#"):
            continue
        entry_norm = os.path.normcase(os.path.normpath(entry))
        if target_norm.endswith(entry_norm) or entry_norm.endswith(target_norm):
            sys.stderr.write(
                f"TDD 잠금: {entry} 는 현재 슬라이스의 테스트 파일이다.\n"
                "테스트를 고치지 말고 구현을 고칠 것.\n"
                "이 테스트가 잘못됐다고 판단되면 사용자에게 알리고 "
                ".claude/tdd-lock 해제를 요청할 것.\n"
            )
            return 2

    return 0


if __name__ == "__main__":
    sys.exit(main())
```

`.claude/settings.json` 에 매처 추가:

```json
{
  "hooks": {
    "PreToolUse": [
      {
        "matcher": "Edit|Write",
        "hooks": [
          {
            "type": "command",
            "command": "py -3 \"${CLAUDE_PROJECT_DIR}/.claude/hooks/guard_test_files.py\""
          }
        ]
      }
    ]
  }
}
```

기존 매처(Bash|PowerShell, rider 상태변경 툴 21개)는 그대로 둔다. 인터프리터는 `py -3` 이다. 이 머신의 `python3` 는 MS Store 별칭 스텁이라 실행되지 않는다.

`.gitignore` 에 추가:

```
.claude/tdd-lock
```

### 확인할 구멍

`rename_refactoring` 이 테스트 파일을 건드릴 수 있다. `Edit|Write` 매처가 이걸 덮지 않는다. 잠금 중에 리네임을 시도해보고, 통과하면 rider 툴 매처에도 이 훅을 붙인다.

`ue_execute_python` 은 훅으로 덮이지 않는다. 사람 판단에만 의존한다.

### 테스트

`.claude/hooks/test_guard_test_files.py` 를 기존 훅 테스트와 같은 형태로 만든다. 최소 세 케이스: 잠금 없음 → 통과, 잠금 있고 대상 일치 → exit 2, 잠금 있고 대상 불일치 → 통과.

## 4단계 검증

```bash
echo "Server/GameServerTests/InventoryTests.cpp" > .claude/tdd-lock
# 세션에서 해당 파일 편집을 시도 → 차단되고 이유가 출력되는지
rm .claude/tdd-lock
py -3 .claude/hooks/test_guard_test_files.py
```

---

# 5단계. 3티어 변경

## 대상

`NEXT.md` 2번, D-14 (스택 상한).

`DISABLED_StackDoesNotExceedMaxStack` 이 이미 커밋된 실패 테스트다. `--gtest_also_run_disabled_tests` 로 실제 빨강임이 확인되어 있다.

이 작업이 3티어 규약을 그물이 있는 상태에서 처음 밟는 연습이 된다. 규모가 작아 연습용으로 적절하다.

## 5a. `.claude/skills/project/three-tier-change/SKILL.md`

```markdown
---
name: three-tier-change
description: 패킷 구조를 바꾸는 변경을 수행한다. .proto 수정이 필요한 작업,
  클라와 서버가 함께 바뀌어야 하는 게임플레이 변경에 사용한다.
---
# 3티어 변경

패킷 정의가 바뀌면 프로토콜·서버·클라 세 곳이 함께 바뀐다.

## 가장 위험한 것: 수평 슬라이싱

"프로토콜부터 다 하고, 서버 다 하고, 클라 다 하고" 는 안티패턴이다.
이 프로젝트의 구조가 자연스럽게 그렇게 자르고 싶게 만든다.

**하나의 동작을 세 티어에 걸쳐 세로로 자른다.**
한 슬라이스가 끝나면 그 동작이 클라에서 서버까지 실제로 작동해야 한다.

## 1. seam 승인 (사람)

3티어 변경에서 seam 후보가 셋이다. **어디서 관찰할지 사용자 승인을 받는다.**

| seam | 관찰 대상 | 러너 |
|---|---|---|
| 서버 순수 로직 | 새 필드가 로직에 미치는 영향 | GameServerTests |
| protobuf 왕복 | 메시지 직렬화·역직렬화 | GameServerTests |
| DummyClient | 실제 패킷 흐름 | 수동 (종료 코드 없음) |

가능하면 앞의 둘로 잡는다. DummyClient 는 환경 의존이 크다.

## 2. 순서

    1) .proto 수정
    2) GenPackets.bat 실행
    3) 서버 핸들러 .cpp 수정
    4) 클라 핸들러 .cpp 수정
    5) 테스트 (seam 에서)

3~4 를 하나의 동작 단위로 묶는다. 여러 동작을 한꺼번에 하지 않는다.

## 3. 생성물 주의

**생성물을 직접 고치지 않는다.** 다음 생성 때 덮어써진다.

| 생성물 | 손대는가 |
|---|---|
| `*.pb.{h,cc}` | 아니오 |
| `ServerPacketHandler.h` | 아니오 |
| `ClientPacketHandler.h` | 아니오 |
| `.proto` 사본 | 아니오 — 원본만 고친다 |
| 핸들러 `.cpp` 2개 | **예. 손으로 쓴다** |

상세는 docs/codegen.md.

`.proto` 는 cp949 다. UTF-8 로 저장하지 않는다.

## 4. 프로토콜 불변식

`ProtocolContractTests` 가 다음을 검사한다. 메시지를 추가하면 이 테스트가
먼저 알려준다.

- `PROTOCOL_MESSAGES(X)` 매크로 목록과 실제 메시지 목록이 일치
- 패킷 ID 가 연속이고 유일
- 전 메시지 왕복 (리플렉션으로 자동 확장)

**이 테스트는 메시지가 늘어도 고치지 않는다.** 고쳐야 한다면 뭔가 잘못된 것이다.

## 5. 클라 검증

클라에는 테스트가 없다. 빌드 통과가 하한이다.

    build_solution_start(rootFolder=".../P1")

동작 확인은 사람이 PIE 로 한다. 무엇을 확인해야 하는지 명시해서 요청한다.

## 6. 완료

work 파일 「증명」의 항목을 실제로 실행하고 종료 코드를 기록한다.

D-14 의 경우 완료 신호는 `DISABLED_` 접두어 제거 후 종료 코드 0 이다.
```

## 5b. `.claude/agents/test-auditor.md`

```markdown
---
name: test-auditor
description: 실패 테스트를 커밋하기 전에 그 테스트가 좋은 테스트인지 감사한다.
  tdd-slice 스킬의 4번 스텝에서 호출된다.
tools: Read
---
# 테스트 감사

주어진 것은 seam 정의, 기대 동작, 테스트 코드다.
**구현 계획과 구현 코드는 주어지지 않는다. 요청하지 않는다.**
구현을 보면 "이 테스트는 이 구현을 잘 검증한다" 로 합리화하게 된다.

## 세 가지를 찾는다

### 1. 구현 결합
내부 구현을 검사하고 있는가.
seam 밖의 상태를 들여다보거나, 내부 자료구조의 형태를 가정하고 있으면 위반이다.
내부를 다시 써도 이 테스트가 살아남는지로 판단한다.

### 2. 동어반복
실질적으로 아무것도 주장하지 않는가.
값을 확인하지 않고 존재만 확인하거나, 방금 넣은 것을 그대로 꺼내 비교하거나,
어떤 구현이든 통과할 어서션이면 위반이다.

### 3. 수평 슬라이싱
하나의 동작이 아니라 한 계층을 자르고 있는가.
이 프로젝트는 클라·서버·프로토콜 3곳이 함께 바뀌는 구조라 계층별로 자르고
싶어진다. 슬라이스가 끝났을 때 관찰 가능한 동작 하나가 완성되지 않으면 위반이다.

## 추가 확인

- 테스트 이름이 함수가 아니라 명세처럼 지어졌는가
- 도메인 용어가 docs/ARCHITECTURE.md 와 일치하는가
- 승인된 seam 에서 관찰하고 있는가

## 보고 형식

각 항목에 대해 **통과 / 의심 / 위반** 과 근거 한 줄.

    구현 결합: 통과 — Inventory 공개 API 만 호출한다
    동어반복: 의심 — has_value() 만 보고 값을 확인하지 않는다
    수평 슬라이싱: 통과 — 단일 동작

**테스트를 고치지 않는다. 보고만 한다.**
```

## 5단계 검증

- `DISABLED_` 접두어가 사라지고 종료 코드 0
- `ProtocolContractTests` 3개가 수정 없이 통과
- `test-auditor` 가 한 번이라도 「의심」을 냈고 그것이 실제로 고칠 만한 지적이었는가

---

# 6단계. Room seam

D-03(Room 분해)의 전제다. Room 에 테스트가 붙기 전에는 분해가 안전하지 않다.

## 왜 어려운가

`Room` 이 `JobQueue` 를 상속하고 상태 변경이 큐 위에서 직렬화된다. L1 테스트로 잡을 수 없다. **seam 을 만드는 작업이 선행된다.**

전투 판정 같은 순수 로직을 `Room` 밖으로 빼내는 것부터다. 이건 리팩토링이므로 별도 work 파일이 필요하다.

## 순서

1. `Room` 안에서 순수 로직인 부분을 식별한다 (`analyze_calls` 로. grep 을 쓰지 않는다)
2. 그중 하나를 골라 별도 클래스로 뺀다. 동작을 바꾸지 않는다
3. 그 클래스에 seam 을 승인받고 테스트를 붙인다
4. 반복

각 회차가 하나의 work 파일이다. 한 번에 다 하지 않는다.

## 이후

`NEXT.md` 의 나머지 후보가 순서대로다.

- DummyClient 승격 (인자화 → 실토큰 → 봇 상태머신 → 종료 코드)
- CI (D-12) — 전제는 충족. `GameServerTests` 가 `config.h` 없이 빌드되고 판정이 종료 코드
- AuthServer 라우터 — Redis·DB 의존이라 DummyClient 승격과 같은 계열
- UE L2 — 에디터 필요라 무인 루프가 안 된다. 별도 계획

## 하지 않는 것

| 항목 | 근거 |
|---|---|
| UE L1 (LLT) | 폐기. 설치본이 `TargetType.Program` 을 거부. ADR-0002 결정 6 |
| UE 클라 TDD | L2 는 에디터 필요, L3 는 Live Coding 비호환 |
| 엔진 소스 패치 | 설치본 고정 확정 |
| 배치 실행 모드 해동 | AC 사다리 조건이 서버 한정으로만 충족 |
| 단계별 지표 측정 | 혼자 하는 프로젝트에서 값어치 없음 |

---

# 부록. 스킬 도입 시점 정리

| 시점 | 만드는 것 | 이유 |
|---|---|---|
| 1d | `new-adr` | 기존 ADR 3개에 Confirmation 을 채우면서 세 번 반복하게 된다 |
| 2단계 | `new-work` | 순서를 틀리면 문제 정의가 구현 가능성에 오염된다 |
| 2단계 | `end-session` | 매 세션 반복이고 마지막 항목을 빠뜨리면 진입점이 사라진다 |
| 4단계 | `tdd-slice` | 3단계에서 무엇이 반복되는지 확인한 뒤 |
| 5단계 | `three-tier-change` | D-14 를 한 번 밟아본 뒤 |
| 5단계 | `test-auditor` (에이전트) | 슬라이스가 3티어에 걸치며 수평 슬라이싱이 실체화된 뒤 |
| 6단계 이후 | `doc-garden` (선택) | ARCHITECTURE 가 코드와 어긋나기 시작한 뒤 |

## 스킬로 만들지 않는 것

| 항목 | 대신 |
|---|---|
| 커밋 규칙 (gitmoji, `-F` 파일) | `CLAUDE.md` 규칙 |
| vcxproj 테스트 소스 등록 | 두 번 놓치면 훅 |
| tech-debt 항목 추가 | `CLAUDE.md` 규칙 + 형식 예시 |
| 도구 라우팅 | `CLAUDE.md` 규칙 |

판단선: **절차가 3스텝 이상이고 순서가 중요하면 스킬, 그 외는 규칙.**

## 외부 스킬

```bash
npx skills add https://github.com/mattpocock/skills --skill grill-me
```

`grill-me` 만 개별 설치한다. **플러그인 전체를 설치하지 않는다.** 자체 문서 체계(`/to-spec`, `/to-tickets`, `/implement`, `/wayfinder`)가 들어 있어 `work/`·`decisions/`·`NEXT.md` 와 병렬로 돌면 문서가 갈라진다.

`grill-me` 는 `new-work` 스텝 1에서만 쓴다. 스텝 2(결정)에는 쓰지 않는다. 문제를 날카롭게 하는 도구지 선택지를 비교하는 도구가 아니다.

### `CONTEXT.md`

`tdd` 스킬을 나중에 설치한다면 `CONTEXT.md` 를 읽는다. 이 프로젝트에서 그 역할은 `docs/ARCHITECTURE.md` 가 하므로 얇은 포인터를 둔다.

```markdown
# CONTEXT
도메인 용어와 구조는 docs/ARCHITECTURE.md 를 따른다.
클래스 계층: Object → Creature → { Player, Monster } (클라/서버 대칭)
패킷 이름: C_* 는 클라→서버, S_* 는 서버→클라.
```

---

# 부록. 전환 완료 체크리스트

## 구조

- [ ] `CLAUDE.md` 100줄 이하
- [ ] `docs/{build,codegen,testing}.md` 존재
- [ ] `docs/NEXT.md` 존재. 후보 5개 이하, 각각에 왜 지금/선행 조건/완료 신호
- [ ] `docs/harness.md` 존재
- [ ] `docs/work/` 평탄. 파일명 `YYYY-MM-DD-*`
- [ ] `docs/decisions/` MADR. `index.md`, `adr-template.md`, 번호 접두어
- [ ] `prompts/` 없음
- [ ] `.claude/` 에 빈 README 없음
- [ ] `docs/reports/`, `docs/references/` 유지됨

## 양식

- [ ] work 파일에 프론트매터·「위험 요소」·「증명」·「작업 중 기록」
- [ ] work 파일에 「다음 세션 후보」 없음
- [ ] 모든 ADR 에 `status`, `scope`, `Confirmation`
- [ ] 무날짜 구본이 `superseded` 로 살아 있음 (삭제되지 않음)
- [ ] tech-debt 23개 전부에 `[scope: ...]`
- [ ] `ARCHITECTURE.md` 에 `파일:줄` 번호 없음
- [ ] `ARCHITECTURE.md` 에 불변식이 `**Architecture Invariant:**` 로 표기됨

## 스킬과 훅

- [ ] `new-work`, `end-session`, `new-adr` 존재
- [ ] `tdd-slice`, `three-tier-change` 존재 (4·5단계 후)
- [ ] `test-auditor` 존재 (5단계 후)
- [ ] `guard_test_files.py` + 테스트 존재, `settings.json` 매처 등록
- [ ] `.claude/tdd-lock` 이 `.gitignore` 에 있음
- [ ] `rename_refactoring` 우회 가능 여부 확인됨

## 실행

- [ ] `GameServerTests.exe` 종료 코드 0
- [ ] `npm test` 종료 코드 0
- [ ] `py -3 .claude/hooks/test_guard_dangerous_cmd.py` 통과
- [ ] `py -3 .claude/hooks/test_guard_test_files.py` 통과
- [ ] 잠금 상태에서 테스트 파일 편집이 실제로 차단됨

## 최종

- [ ] 새 세션이 `docs/NEXT.md` 하나만 읽고 다음에 뭘 할지 안다
- [ ] 이 절차서가 `docs/work/YYYY-MM-DD-harness-migration.md` 로 이동됨
- [ ] 하네스 전환 근거가 ADR 로 남았다
