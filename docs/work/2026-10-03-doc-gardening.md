---
작업: doc-gardening · 시작: 2026-10-03
브랜치: `chore/185-doc-lint` · 이슈: #185 · PR: #188
---

# 문서 gardening: 낡은 문서의 최신화와 문서 리뷰

## 목표

- `/my-doc-gardening` 슬래시 명령이 바뀐 코드·문서와 연관된 문서를 골라, 낡은 내용과 문서 정책 위반을 리포트로 낸다
- 사람이 번호로 승인한 항목만 현재 작업 트리에 고친다. 스킬은 커밋하지 않는다
- 정규식으로 판정할 수 있는 문서 규칙은 `Tools/DocLint/check_docs.py`가 판정하고, CI가 새 위반을 막는다

## 세션 시작 절차

1. `chore/185-doc-lint`를 체크아웃한다. 작업 전체를 PR #188 하나에 커밋을 이어 붙여 올린다
2. 진행 표에서 첫 「대기」 단계를 고르고, #185 본문의 같은 번호 절을 읽는다
3. 단계를 마친 커밋에 진행 표의 해당 줄을 「완료」로 고치는 변경을 함께 넣는다

## 지킬 것

- 아래 「정답지」의 사례는 3단계 전에 고치지 않는다. 도구가 이 사례를 잡는지로 품질을 확인한다
- 작업 중에 새로 찾은 문서 위반도 그 자리에서 고치지 않는다. 「기록」에 적고 정답지에 더한다
- 문체는 이 도구가 보지 않는다. 문체 점검은 `/korean-md-style`이 맡는다
- ADR은 본문을 고치지 않는다. 날짜를 적은 덧붙임만 제안한다
- 진행 표의 「검증」에는 실제로 돌린 명령과 종료 코드만 적는다

## 진행

상태: 대기 · 진행 · 완료 · 막힘

| # | 단계 | 상태 | 검증 |
|---|---|---|---|
| 1 | 기계 검사 스크립트 `check_docs.py` | 완료 | `check_docs.py --self-test` 10건 통과 종료 코드 0 · `check_docs.py` 위반 4건 종료 코드 1(정답지 1·2·7번, 오탐 0건) · `check_conventions.py --self-test` 종료 코드 0 · PR #188 CI 두 잡 통과 |
| 2 | `my-doc-gardening` 스킬 | 완료 | 실행 검증은 3단계의 `--all` 실행으로 한다 |
| 3 | 첫 전수 실행, 기존 위반 수정, CI 실제 검사 켜기 | 대기 | — |

## 결정

| 결정 | 틀렸다는 신호 |
|---|---|
| 외부 도구를 들여오지 않고 직접 만든다. 저장소의 `code-review-matt` 골격과 `korean-md-style` 진단 절차를 재사용한다 | 판정 기준 파일의 유지 비용이 외부 도구를 고쳐 쓰는 비용을 넘는다 |
| 검사 축은 드리프트와 정책 둘이다. 문체는 넣지 않는다 | 두 축의 리포트가 같은 항목을 반복해서 함께 올린다 |
| 기본 범위는 `origin/dev`와의 merge-base 이후 바뀐 부분이다 | 전수 실행에서만 잡히는 낡은 문서가 반복해서 나온다 |
| 참조 검사(`section-ref`, `path-ref`, `backlog-ref`)는 ADR과 work 파일을 보지 않는다. ADR은 본문을 고칠 수 없어서 위반이 0건이 되지 않는다 | ADR의 끊긴 참조 때문에 잘못된 판단을 한 일이 생긴다 |
| 실행은 사람이 슬래시 명령으로 할 때만이다 | 사람이 몇 주씩 실행하지 않는 사이에 낡은 문서가 쌓인다 |
| 진행 상태는 이 파일의 진행 표가 원천이다 | 진행 표와 PR 상태가 어긋난 채 발견된다 |

## 정답지

착수 전 조사(2026-10-03)에서 찾은 낡은 내용과 정책 위반이다. 「기계」 열은 `check_docs.py`가 잡아야 하는 사례다.

| # | 위치 | 문제 | 기계 |
|---|---|---|---|
| 1 | `docs/ARCHITECTURE.md` 「이렇게 안 한 이유」, `docs/backlog.md` 1번 항목 | 규범 문서가 `docs/references/`를 가리킨다 | ✓ |
| 2 | `docs/build.md` 「Rider MCP 상세」, `docs/adr/0002`의 툴 계열 표, `docs/testing.md` 「아직 없는 것」 | 없는 `CLAUDE.md` 「도구 라우팅」 절을 가리키고, `CLAUDE.md`에 없는 「완료 기준」 인용문을 쓴다 | `build.md`만 |
| 3 | `docs/build.md` 「인증 서버」 | eslint 설정과 lint 스크립트가 없다고 쓰지만 실제로 있다 | |
| 4 | `docs/ARCHITECTURE.md`, `docs/folder-structure.md` | 손으로 쓰는 핸들러 `.cpp`를 둘이라고 쓰지만 `docs/codegen.md`는 셋을 나열한다 | |
| 5 | `docs/ARCHITECTURE.md` 머리말과 클래스 계층 | `최종 수정: 2026-09-XX` 자리표시자, 「이후 구조 변경 없음」, `Object → Creature`가 서버 코드의 `Entity`와 다르다 | |
| 6 | `docs/testing.md` 커버리지 표와 seam 표 | `CombatTests` 등 스위트가 빠졌고 「전투 판정 seam 없음」이 지금 코드와 다르다 | |
| 7 | `docs/adr/0003`, `P1/Scripts/Backup-UeContent.ps1`, `docs/backlog.md` 8번 항목 | 없는 backlog 5번과 지금은 다른 항목인 7번을 가리키고, backlog 안의 「10번(에디터 에셋 검증기)」는 실제로 11번이다 | backlog 안의 참조만 |
| 8 | `docs/adr/0002` 끝부분 | DB 연결이 쓰기 계정이라고 쓰지만 지금은 `claude_ro` 읽기 전용이다. 날짜를 적은 덧붙임이 필요하다 | |
| 9 | `.gitignore`의 `C_TestJsonfile.json` 주석 | 근본 원인을 적었다는 tech-debt 항목이 없다 | |
| 10 | `docs/work/2026-09-19-convention-migration.md` | 진행 상태의 원천을 GitHub Issues로 적어 `CLAUDE.md`의 「작업 방식」과 다르다 | |
| 11 | ADR-0009, ADR-0007 | 판단 필요. `DefaultGame.ini`의 에셋 경로가 ADR-0009 위반인지, ADR-0007 표의 서버 `Combat` 「없음」이 지금과 다른지 | |

## 기록

- 2026-10-03: #185에서 `check_docs.py`를 처음 돌렸다. 위반 5건 중 `docs/backlog.md`의 「엔진 제약」 참조 1건은 오탐이었다. `docs/build.md`의 제목이 `엔진 제약 — 무언가를 계획하기 전에…`처럼 줄표 뒤에 부연을 달고, 참조는 앞부분만 적는다. 제목의 줄표 앞부분도 별칭으로 인정하도록 고쳤고, 남은 4건은 모두 정답지 사례다
- 2026-10-03: `path-ref`를 설계하면서 잰 오탐 원인은 세 가지였다. `.gitignore` 대상(`P1/Saved/`, `docs/reports/`), 중괄호 확장(`Server/Binary/{Debug,Release}/`), 모듈 기준 상대 경로(`P1/Network`)다. 무시 대상은 실재로 치고, 중괄호는 펼치고, 확장자도 끝 `/`도 없는 토큰은 판정하지 않는다
