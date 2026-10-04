# 정책 축 판정 기준

문서가 이 저장소의 문서 규칙을 지키는지 본다. 규칙은 문서마다 무엇을 담는지와 문서끼리 어떻게 가리키는지다.
내용이 사실인지는 드리프트 축이, 문장이 매끄러운지는 `/korean-md-style`이 본다.

**양식은 보지 않는다.** 절 구성, 표의 열, 항목 순서, 파일 이름 패턴, 개수 줄 같은 양식은 수시로 바뀌고
엄격하게 지킬 대상이 아니다. 양식이 문서마다 다르거나 템플릿과 달라도 발견으로 올리지 않는다.

규칙의 원문은 출처 열의 문서에 있다. 이 파일은 판정 문장만 모은다. 출처와 이 파일이 다르면 출처가 맞다.
판정이 애매하면 출처를 열어 원문을 읽는다.

## 일하는 방식

1. 대상 문서마다 아래 표에서 그 문서에 걸리는 규칙을 고른다. 「대상」 열이 기준이다
2. 고른 규칙을 하나도 빼지 않고 적용한다
3. 위반만 발견으로 올린다

완료 기준: 대상 문서마다 걸리는 규칙을 전부 적용했다. 기계 검사(`check_docs.py`)가 이미 보는 규칙은 표에 없다.
그 결과는 프롬프트로 받는다.

## 문서마다 담는 것

| ID | 대상 | 판정 | 출처 |
|---|---|---|---|
| P1 | `CONTEXT.md` | 세 티어가 공유하는 이 프로젝트 고유의 용어만 담는다. 구현, 배치, 일반 프로그래밍 개념이 들어가면 위반이다. 정의는 1~2문장이고 무엇을 하는지가 아니라 무엇인지를 말한다. 동의어는 하나를 고르고 나머지를 「피할 말」에 적는다 | `CONTEXT.md` 머리말, `domain-modeling` 「Update CONTEXT.md inline」, `CONTEXT-FORMAT.md` 「Rules」 |
| P2 | `docs/ARCHITECTURE.md` | 코드를 읽어서 알 수 있는 내용을 반복하지 않는다. 왜 이렇게 되어 있는지와 깨면 안 되는 것만 적는다 | `docs/ARCHITECTURE.md` 머리말 |
| P3 | `docs/conventions.md`, `docs/folder-structure.md` | conventions는 코드 안의 규칙(이름, 주석, 멤버 배치, 타입)만, folder-structure는 무엇을 어디에 둘지만 담는다. 서로의 몫이 들어가면 위반이다 | 두 문서의 머리말 |
| P4 | `docs/tech-debt.md` | 지금 틀린 것만 담는다. 해결된 항목, 「해결됨」·「완료」 표기, 착수 순서와 계획은 위반이다. 순서와 계획은 backlog의 몫이다 | `docs/tech-debt.md` 머리말, `CLAUDE.md` 「작업 방식」 |
| P5 | `docs/backlog.md` | 착수하지 않은 작업 후보를 담는다. 지금 틀린 것(결함)은 tech-debt의 몫이다. 「하지 않기로 확인된 것」의 행마다 근거 문서를 가리킨다 | `docs/backlog.md` 머리말과 「하지 않기로 확인된 것」 |
| P6 | `docs/adr/` | ADR은 되돌리기 어렵고, 맥락 없이는 놀랍고, 실제 트레이드오프의 결과인 결정이다. 셋 중 하나라도 아니면 판단 필요로 올린다. 구현 수준의 결정은 ADR이 아니라 커밋 메시지와 work 파일의 「결정」이 담는다 | `ADR-FORMAT.md` 「When to offer an ADR」 |
| P7 | `docs/adr/` | ADR은 자기 완결적이다. `docs/reports/`나 다른 임시 자료를 링크하거나 근거로 인용하지 않는다. `docs/references/`도 같다 | `docs/work/2026-09-19-convention-migration.md` 「반드시 지킬 것」, `docs/references/README.md` |
| P8 | `docs/adr/` | 덧붙임에는 덧붙인 날짜가 있다. 날짜 없이 본문이 바뀌었는지는 `git log -p --follow <ADR>`로 본다. 본문 문장이 덧붙임 없이 고쳐졌으면 위반이다 | `CLAUDE.md` 「문서 위치」, `ADR-0007`의 덧붙임 |
| P9 | `docs/work/` | 진행 표의 「검증」에는 실제로 돌린 명령과 결과만 적는다. 실행하지 않은 검증을 적었으면 위반이다 | `CLAUDE.md` 「완료 기준」 |
| P10 | `docs/work/` | 진행 상태의 원천은 work 파일의 진행 표다. 진행 상태를 다른 곳(이슈만, 문서 본문)에 둔다고 적었으면 위반이다. 모든 단계가 끝난 work 파일이 남아 있으면 판단 필요로 올린다 | `CLAUDE.md` 「작업 방식」 |
| P11 | `docs/agents/` | 상류 템플릿 문구를 그대로 둔다. 의도적 이탈은 `issue-tracker.md`의 `--body-file` 절 하나다 | `docs/agents/issue-tracker.md` 「Why `--body-file` here」 |
| P12 | `docs/reports/` | 앞으로도 지켜야 할 결론이 리포트에만 있고 규범 문서(folder-structure, conventions, tech-debt, backlog, adr)에 없으면 발견으로 올린다. 리포트는 git이 추적하지 않아서 저장소에 남지 않는다 | `CLAUDE.md` 「문서 위치」 |

## 문서끼리의 관계

| ID | 대상 | 판정 | 출처 |
|---|---|---|---|
| P13 | 전부 | 한 사실은 한 곳에 둔다. 다른 문서는 그곳을 가리킨다. 근거가 ADR에 있으면 근거를 옮겨 적지 않고 ADR 경로를 적는다 | `writing-for-agents` 「Pruning」 |
| P14 | `CLAUDE.md`와 상세 문서 | `CLAUDE.md`의 요약(「아키텍처 핵심 규칙」 등)이 「상세」로 가리킨 문서의 내용과 어긋나면 위반이다 | `CLAUDE.md` 「아키텍처 핵심 규칙」 |
| P15 | 규범 문서 | `docs/reports/`를 근거로 인용하지 않는다. 리포트의 결론이 필요하면 그 결론을 본문에 옮긴다 | `CLAUDE.md` 「문서 위치」 |

## 에이전트가 읽는 문서

`CLAUDE.md`, `.claude/skills/*/SKILL.md`, 그리고 `CLAUDE.md`의 포인터로 닿는 문서에 걸린다.

| ID | 판정 | 출처 |
|---|---|---|
| P16 | 포인터(`CLAUDE.md`의 문서 줄, 스킬 description)는 무엇인지와 언제 읽는지를 담는다. 언제 읽는지가 빠졌으면 위반이다 | `writing-for-agents` 「Context pointers」 |
| P17 | 사람만 호출하는 스킬은 `disable-model-invocation: true`이고 description은 한 줄 요약이다. 에이전트가 스스로 호출해야 하는 스킬은 description에 트리거 분기를 담는다 | `writing-for-agents/SKILL-MECHANICS.md` 「Invocation」 |
| P18 | 모델이 기본으로 하는 일을 지시하는 문장(no-op)은 위반이다. 문장째 지우는 수정안을 낸다. 기본 동작인지 애매하면 판단 필요다 | `writing-for-agents` 「Pruning」 |
| P19 | 금지만 적은 지시는 긍정 목표와 짝지어 있어야 한다. 긍정으로 바꿀 수 있는 금지는 긍정으로 바꾸는 수정안을 낸다 | `writing-for-agents` 「Leading words」 |
| P20 | 환경에서 명령 한 번으로 찾을 수 있는 것(스크립트 인자 전체, 설정 파일 내용)을 옮겨 적은 줄은 낡는 캐시다. 찾기 비싼 것(관례, 이유, 함정)이 아니면 판단 필요로 올린다 | `writing-for-agents` 「Pruning」 |

## 모든 문서

| ID | 판정 | 출처 |
|---|---|---|
| P21 | 더 이상 문서가 하는 일과 관계없는 줄(끝난 계획, 해결된 문제의 경위, 걷어낸 도구의 설명)은 퇴적층이다. 지우거나 커밋 메시지로 넘기는 수정안을 낸다 | `writing-for-agents` 「Pruning」 |
| P22 | 실측 결과를 적은 문장에는 측정 날짜가 있다. 날짜 없는 실측은 언제까지 유효한지 알 수 없다 | 문서 전반의 관례(`docs/build.md`, `docs/testing.md`) |
