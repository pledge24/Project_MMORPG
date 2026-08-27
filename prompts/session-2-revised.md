# 세션 2 프롬프트 (개정판) — 검증 인프라 구축 + 첫 버그 수정

정리 세션이 끝난 뒤(dev 머지 + ADR 커밋 완료 상태에서) 사용한다.
프로젝트 루트에서 claude 실행, Plan Mode로 시작.

---

이 세션의 목표는 L1 테스트 인프라 구축과, 그 인프라로 잡는 첫 버그 수정이다.
시작 전에 다음을 읽어라: docs/decisions/2026-08-19-final-harness-verdict.md (특히
편입 4건과 §9), docs/tech-debt.md의 D-01, docs/references/batch-mode-archive.html이
있다면 그 안의 step 설계 원칙.

계획을 docs/plans/active/verification-infra.md에 먼저 써라. 계획은 확정된 관행을
따른다: 단계별 검증 가능한 완료 기준(AC)을 명시하고, 막히면 [B] blocked로 표기 후
인계하며, 완료 항목에는 summary 한 줄을 남긴다.

## A. 게임 서버 — GoogleTest (세션 1 조사안 기반)
1. 세션 1에서 권장한 통합 방식을 공식 문서로 재확인해라:
   Server/GameServerTests/GameServerTests.vcxproj (콘솔 exe) 추가 + gtest는 vcpkg
   manifest. GameServer가 exe라 링크 불가하므로 테스트 대상 .cpp를 테스트 프로젝트에
   직접 포함(UE LLT와 같은 패턴). 재확인 결과가 다르면 대안을 제시하고 승인 후 진행.
2. vcpkg가 이 머신에 없으면 설치 경로/방법을 제안하고 승인 후 진행해라.
3. **파이프라인 증명 테스트**: 결합도 0인 패킷 직렬화 왕복 1~2개로 작성→빌드→실행→
   결과 확인 루프가 도는 것을 먼저 증명해라. 실행은 Rider Run Configuration으로
   등록하고 execute_run_configuration으로 호출한다.
4. **첫 실전: Inventory (D-01 버그를 TDD로)**
   (1) Inventory.cpp:44의 SLOT_TYPE_INVENTORY_MISC → ITEM_TYPE_GEAR 혼용 버그를
       드러내는 실패 테스트를 먼저 작성하고, 실패를 확인해라 (빈 Player + 손으로
       시드한 ItemDataTable — 세션 1 조사대로 결합이 얕다).
   (2) 버그를 수정하고 테스트 통과를 확인해라. 이 수정은 "계획된 작업에서만 수정"
       원칙에 부합한다 (판정 세션에서 세션 2로 예약된 건).
   (3) 직렬화 테스트 포함 전체 재실행으로 회귀 없음을 확인해라.
   (4) D-14, D-15도 같은 영역이면 동일 루프로 이어가되, 범위가 커지면 [B] 없이
       계획 파일에 다음 세션 후보로 기록하고 멈춰라.
5. 직렬화 왕복을 40개 메시지 전체로 확장할지는 비용을 보고 판단해 제안해라
   (생성기 재실행 회귀 그물이 목적 — 자동 생성 가능하면 확장, 수작업이면 대표만).

## B. UE 클라 — Low-Level Tests 모듈 (시간·컨텍스트 여유 시)
A가 끝난 시점에 컨텍스트 여유를 자가 평가해라. 빠듯하면 B·C를 계획 파일에
다음 세션으로 넘기고 D로 건너뛴다 (이 분할 판단 자체를 보고할 것).
1. Source/P1Tests/ 모듈: Build.cs는 TestModuleRules, Target.cs는 TestTargetRules.
2. LLT는 전용 타깃의 별도 exe다 — Rider Unit Tests 창(L2용)으로 돌리려 하지 말 것.
   실행 명령을 Run Configuration "LLT: P1Tests"로 등록하고 호출로 검증.
3. 테스트 1개로 파이프라인 전체를 증명해라.

## C. 인증 서버 — 최소 테스트 (여유 시)
package.json 관례에 맞는 러너로 npm test 스모크 1개. eslint 미연결(D 항목)은
이 세션 범위 밖 — 건드리지 말 것.

## D. 마무리
1. CLAUDE.md 테스트 계층 절에 실제 실행 경로(Run Configuration 이름 포함)를 확정
   기입하고, "테스트 존재 영역 = 테스트 통과까지가 완료" 기준이 이제 서버에
   적용됨을 명시해라.
2. 이번 결정(vcpkg 채택, 프로젝트 구성, .cpp 직접 포함 등)을 docs/decisions/ ADR로.
3. 계획 파일을 summary 관행대로 채우고 completed/로 이동해라.
4. 숫자 보고: 작성한 테스트 수, 테스트가 잡은 버그/회귀 수(D-01이 1호), 훅 차단
   누적 건수.
5. 다음 세션 후보를 제안해라 — 남은 B/C, DummyClient 승격(인자화→실토큰→봇 상태머신
   →종료코드), 리팩토링 착수(grilling + improve-codebase-architecture 도입) 중
   진단 근거와 함께.
