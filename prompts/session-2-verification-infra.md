# 세션 2 프롬프트 — 검증 인프라 구축 (L1)

세션 1의 진단이 승인된 뒤 사용한다. 리팩토링보다 이게 먼저다:
검증 수단 없이 뒤죽박죽 코드를 리팩토링하는 것이 에이전트에게 가장 위험한 작업이기 때문이다.

---

이 세션의 목표는 L1 테스트 인프라 구축이다. docs/plans/active/verification-infra.md에 계획을 먼저 쓰고 시작해라.

## A. 게임 서버 (C++) — GoogleTest + CTest
1. 세션 1에서 확인한 빌드 시스템에 GoogleTest를 붙여라.
   - CMake라면: FetchContent로 GIT_TAG를 특정 버전에 고정 (main 금지),
     Windows MSVC면 gtest_force_shared_crt ON, enable_testing() + gtest_discover_tests().
   - CMake가 아니라면: 현재 빌드 시스템에 맞는 통합 방법을 먼저 제안하고 내 승인 후 진행.
2. 첫 테스트는 세션 1에서 제안한 "입력 없는 순수 로직" 후보 중 하나로 작성해라.
   외부 의존성(Redis, DB)은 GMock으로 끊어라.
3. 실행 진입점을 확정해라 (예: ctest --output-on-failure) → Rider Run Configuration으로 등록.
4. 빌드 환경이 github.com에 접근 가능한지 먼저 확인해라. 막혀 있으면 vendoring으로 전환.

## B. UE 클라이언트 — Low-Level Tests 모듈
1. Source/{프로젝트명}Tests/ 모듈 생성:
   - {프로젝트명}Tests.Build.cs → TestModuleRules 상속
   - {프로젝트명}Tests.Target.cs → TestTargetRules 상속, 테스트 대상 모듈 지정
2. 주의: LLT는 전용 타깃으로 빌드되는 별도 실행 파일이다. Rider Unit Tests 창(L2 Automation Test용)으로
   돌리려 하면 계속 실패한다. 실행 명령줄을 Rider Run Configuration "LLT: {프로젝트명}Tests"로 등록하고
   execute_run_configuration으로 호출해라.
3. 첫 테스트 1개를 작성해서 파이프라인 전체(작성→빌드→실행→결과 확인)가 도는 것을 증명해라.

## C. 로그인 서버 (Node.js)
1. 테스트 러너가 없다면 최소 구성으로 붙여라 (기존 package.json 관례를 따를 것).
2. npm test로 실행되는 스모크 테스트 1개 작성.

## D. 마무리
1. CLAUDE.md의 "테스트 계층" 섹션에 실제 실행 경로를 확정 기입해라 (Run Configuration 이름 포함).
2. 이번 작업에서 내린 결정(GoogleTest 버전, 모듈 구성 방식 등)을 docs/decisions/에 ADR로 남겨라.
3. docs/plans/active/verification-infra.md를 completed/로 이동해라.
4. 다음 세션은 L2 이동 테스트 1개(투자 대비 회수 최대 지점) 또는 리팩토링 첫 타깃이다 — 어느 쪽이
   먼저여야 하는지 진단 결과 기준으로 의견을 내라.
