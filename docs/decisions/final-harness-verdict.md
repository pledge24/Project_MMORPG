# Final_Harness 판정표 (2026-08-19, 세션 1 완료 시점)

범례: ✅ 반영됨 · 🔜 예정(시점 명시) · 💬 토론 필요 · ❌ 폐기 제안
확정되면 이 문서를 docs/decisions/에 ADR로 커밋한다.

---

## 소스 1 — ue-server-tooling-setup.html

| # | 항목 | 판정 | 근거 |
|---|---|---|---|
| 01 | Rider MCP Server | ✅ | 세션 0 도입, 세션 1에서 실검증. 툴 이름 2건 정정됨 (build_solution_start+폴링, execute_tool) |
| 02 | Claude Code [Beta] 플러그인 | ✅ | 세션 0 도입 |
| 03 | Unreal MCP + AllToolsets | 🔜 | HTML 도입 순서 5번: 커밋 습관 정착 후. UE 5.8이라 경로 열림. 클라 리팩토링 착수 전 도입 검토 |
| 04 | Epic 공식 UE 스킬 플러그인 | 🔜 | 03과 함께 |
| 05 | clangd-lsp | ❌ | Rider MCP 전용 결정으로 헤드리스/CI 경로 없음. CI 도입 시 재검토 |
| 06 | lldb MCP | ❌ | CI 크래시 덤프 배치가 없음. 원문도 우선순위 최하 |
| 07 | Catch2 / Low-Level Tests | 🔜 | 세션 2 (UE 클라 L1) |
| 08 | GoogleTest + GMock | 🔜 | 세션 2. 단 통합 방식은 HTML의 CMake FetchContent가 아니라 vcxproj + vcpkg manifest (세션 1 조사 — VS 솔루션이라 HTML 전제가 깨짐) |
| 09 | CTest | ❌ | CMake 아님. 대체: 테스트 exe 직접 실행 + Rider Run Configuration |
| 10 | 새니타이저 TSan/ASan/UBSan | 💬 | HTML은 clang 전제. MSVC는 TSan 미지원 — "게임 서버 멀티스레드라 TSan이 값어치" 논리의 핵심이 무너짐. ASan만 보류로 두는 안 |
| 11 | Gauntlet (L4) | 🔜 | 프로토콜 안정 후. 선행: DummyClient 승격 (세션 1이 로드맵 제시: 인자화→실토큰→봇 상태머신→종료코드) |
| 12 | Automation Driver (L3) | 🔜 | 상점 UI 확정 후. 리팩토링으로 UI 흔들리는 동안 태깅 금지 |
| — | 안전 설정 | ✅ | 세션 0. DB 툴군은 off가 아닌 Router-only로 수정 채택 (MSSQL 실존 때문) |
| — | 작업별 라우팅 표 | ✅ | CLAUDE.md 도구 라우팅 섹션. 툴 이름 정정 반영 |
| — | 테스트 4계층 | ✅ | CLAUDE.md 골격 반영. 구축은 L1→L2→L4→L3 순 |
| — | CLAUDE.md 예시 | ✅ | 병합본에 흡수 |
| — | 도입 순서 | ✅ | 따르는 중 (1일차 완료 상태) |

## 소스 2 — harness_notion.md

| 항목 | 판정 | 근거 |
|---|---|---|
| TDD 작업 방식 (작은 변경→테스트→커밋) | 🔜 | 세션 2에서 테스트 인프라가 생기는 순간부터 적용 가능 |
| TODO.md 운영 | ❌ | docs/plans/active/와 역할 중복. 기록 시스템이 둘이면 한쪽이 반드시 부패한다. plans가 상위호환(체크박스+결정로그) |
| AI 지식 = 내 지식 동기화 | ✅ | 세션 1의 "문서 대 코드 대조"가 이 원칙의 실행. 경로 9건 정정이 증거 |
| Grill me 기법 | 💬 | mattpocock grilling 채택과 한 몸으로 결정 (아래 소스 4) |
| 깊은 모듈 구조 | ✅(부분) | 리팩토링 목표에 녹음. improve-codebase-architecture 채택 시 방법론까지 확보 |
| 세션 기억 소실·자기평가 편향 대응 | ✅/🔜 | plans 파일(✅), code-reviewer 서브에이전트(🔜 2단계) |
| Planner-Generator-Evaluator 구조 | 💬 | 원형은 1인+현 단계에 과함. 축소형 제안: Plan Mode=Planner, 본 세션=Generator, code-reviewer 서브에이전트=Evaluator. 원형 폐기 + 축소형 채택 |
| /doctor (하네스가 모델 자유도 억압하는지 점검) | 💬 | 개념은 좋으나 별도 커맨드는 과함. /gc에 "CLAUDE.md 규칙 중 낡은 것 탐지" 항목으로 통합 제안 |
| 단순하게, 필요한 부분만 수정 | ✅ | CLAUDE.md 기존 코드베이스 방침 |
| AI 역할 분리 (Researcher/Planner/Reviewer) | 💬 | P-G-E와 동일 묶음으로 결정 |
| 프롬프트·툴 로그 남기기 | 💬 | 별도 로그 시스템은 과함. Claude Code가 transcript를 자체 보관하고, 사람용 기록은 docs/reports/ 세션 리포트 관행(이미 1건 발생)으로 갈음 제안 |
| 프롬프트 획일화 | ✅ | prompts/ 세션 프롬프트가 실체 |
| (1) CLAUDE.md 목차 유지 | ✅ | |
| (2) docs/ 기록 시스템 | ✅ | decisions/ plans/ tech-debt.md 전부 가동, 세션 1에서 실사용 |
| (3) 계획을 파일로 | ✅ | |
| (4) 반복 지침→Skill, 강제 규칙→hook | ✅ | 원칙 반영. skills는 승격 대기(의도적 빈 자리) |
| (5) 레이어 종속성 기계 강제 | 💬 | dependency-cruiser/eslint-boundaries는 JS 도구. C++/UE엔 등가물 부재 (UBT 모듈 의존성이 일부 대행). 보류하고 code-reviewer 리뷰 항목으로 대체 제안 |
| (7) 단일 로그 파일 + 검증 스크립트 | 💬(부분) | 검증 명령은 Rider MCP 경로로 대체 결정 완료. "서버 구조화 로그를 에이전트가 읽는다"는 부분만 살아있음 — 세션 3+ 후보 (지금 서버 로그 체계 확인 필요) |
| (8) 브라우저 MCP | ❌ | UI가 웹이 아님. AuthServer는 API 전용 |
| (9) code-reviewer 서브에이전트 1개 | 🔜 | 2단계. .claude/agents/ 자리 확보됨 |
| (10) /gc 주 1회 | 🔜 | 2단계. /doctor·revise-claude-md 기능 통합 후보 |
| 포트폴리오: ADR | ✅ | 가동 중 |
| 포트폴리오: 린트 규칙 승격 서사 | ✅ | 1호 확보 — 훅이 자기 커밋 메시지를 막은 사건 → git commit -F 규칙 승격 |
| 포트폴리오: CLAUDE.md 변경 이력 | ✅ | git으로 쌓이는 중 |
| 포트폴리오: 숫자 기록 | 💬 | 시작할지, 무엇을 셀지. 제안: 훅 차단 건수, 테스트가 잡은 회귀 수, 문서 정정 건수를 세션 리포트에 누적 |
| AGENTS.md 목차+규범 | ✅ | CLAUDE.md로 흡수 (명칭 충돌은 공식 우선으로 해소) |
| ARCHITECTURE.md | ✅ | 존재 + 세션 1 검증·보강 |
| plans active/completed 2단 | ✅ | |
| 두 번이면 문서, 세 번이면 규칙 | ✅ | CLAUDE.md 작업 방식 |
| 100줄 예산 | ✅(변형) | 병합으로 초과 → "비대해지면 서술은 docs/로" 규칙로 변형 채택 |
| 주기적 문서-코드 대조 | ✅ | 세션 1이 1회차. /gc에 정례화 예정 |
| 스킬: Understand-Anything | ❌ | 세션 1 리포트가 도해 6개 생성 — 유사 효과를 이미 자체 확보 |
| 스킬: revise-claude-md | 💬 | 개념 채택하되 외부 스킬 대신 /gc 통합 제안 |
| 스킬: claude-md-improver | 💬 | 위와 중복 — 하나로 |
| 스킬: Im-not-ai | ❌ | 문체 교정은 코드 프로젝트와 무관 |
| 스킬: Insane-search | ❌ | 필요가 증명된 적 없음. 필요 발생 시 재검토 |

## 소스 3 — harness_framework (본인 레포)

| 항목 | 판정 | 근거 |
|---|---|---|
| CLAUDE.md 템플릿 | ✅ | 구조를 흡수한 뒤 대체됨 |
| Stop 훅 (lint+build+test) | ❌ | 확정 폐기 — UE 빌드는 분 단위라 매 응답 실행 불가 (충돌 2에서 결정) |
| PreToolUse 위험명령 훅 | ✅(재작성) | 개념 채택. 원본의 $CLAUDE_TOOL_INPUT 방식은 공식 스펙(stdin JSON) 불일치로 폐기, 세션 1에서 파일 기반으로 재작성·강화 |
| docs/PRD.md, UI_GUIDE.md | ❌ | 웹 템플릿 잔재 |
| docs/ADR.md 단일 파일 | ✅(변형) | decisions/ 폴더 방식으로 변형 채택 |
| commands/review.md | 🔜 | 2단계 code-reviewer와 함께 내용 재작성 |
| commands/harness.md | 💬 | 내용 확인 필요 — 본인이 판단해서 알려줄 것 |
| scripts/execute.py | ❌(추정) | 웹 스택용으로 추정. 아니라면 알려줄 것 |

## 소스 4 — mattpocock/skills

| 항목 | 판정 | 근거 |
|---|---|---|
| grilling / grill-me | 💬 | 채택 권장. 용도: 리팩토링 계획(docs/plans)을 구현 전에 심문. notion의 Grill me와 동일 개념이라 한 번에 결정. 시점: 리팩토링 착수(세션 3) 직전 |
| improve-codebase-architecture | 💬 | 채택 권장(수정 필요). CONTEXT.md/docs/adr 의존을 우리 docs/ 구조로 치환. tech-debt D-01~D-21의 우선순위 산정 도구로 정확히 맞음 |
| handoff | ❌(보류) | 세션 연속성은 plans 파일이 이미 담당. 중복 시스템 회피 원칙 적용. plans가 부족하다고 느껴지면 재검토 |
| git-guardrails | ✅(흡수) | 우리 훅이 이미 동등 이상. 차이 하나: 원본은 git push 전체 차단, 우리는 --force만. PR 워크플로우상 push가 필요하므로 현행 유지 |
| tdd, code-review, domain-modeling 등 나머지 | ❌(보류) | 필요가 증명되면 개별 재검토. 지금 들이면 무게만 늘음 |
