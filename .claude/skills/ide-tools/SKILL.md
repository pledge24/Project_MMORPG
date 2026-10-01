---
name: ide-tools
description: Rider MCP와 언리얼 MCP(`unreal`) 툴의 이 저장소 전용 사용법과 함정. 심볼·호출자 탐색, 심볼 리네임, UE 에셋과 블루프린트 조회, 언리얼 에디터 연결 확인에 이 툴들을 쓰기 전에 사용한다.
---

# Rider MCP와 언리얼 MCP

두 서버의 통제 수단이 다르다. Rider는 `.claude/settings.json`의 `permissions.deny`가, 언리얼은 훅
`.claude/hooks/guard_dangerous_cmd.py`의 툴셋 허용 명단이 막는다. 판단 기준은 문서가 아니라 세션에
실제로 노출된 툴 목록이다. IDE 화면의 체크 상태도 근거가 아니다 — 이 엔드포인트에 반영되지 않는다.

## Rider MCP

- 모든 툴에 `rootFolder`를 명시한다(파라미터 이름이 `projectPath`가 아니다). 솔루션이 하나만 열려
  있으면 생략해도 에러 없이 실행되므로 빠뜨린 것을 알아채지 못한다. 열린 프로젝트 목록은 인자 없이
  `get_run_configurations`를 부르면 에러 메시지로 돌아온다.
- 심볼 탐색은 `skill_search`의 `mode=symbol`로 한다. 돌려주는 좌표는 `1행 1열`로 고정되므로 파일
  경로만 쓴다.
- 호출자 확인은 `search_text`로 한다. `analyze_calls`는 C++ 심볼을 색인하지 않아 막아 두었다.
- 린트와 진단은 `lint_files`, `get_file_problems`로 한다.
- 빌드에 Rider MCP를 쓰지 않는다. 빌드는 터미널에서 돌리고 종료 코드로 판정한다(`CLAUDE.md`).

### 심볼 리네임

텍스트 치환이 아니라 `rename_refactoring`을 쓴다. `applied: true`는 반영을 뜻하지 않는다.

1. 시작 전에 사람에게 에디터 탭을 닫아 달라고 요청한다. 열린 탭은 저장되지 않으면서 성공을 보고한다.
2. 한 건마다 디스크의 파일을 열어 바뀌었는지 확인한다.
3. 파일 묶음이 끝나면 빌드로 판정한다.
4. `no_renamable_symbol`로 거부되면 우회하지 않고 사람에게 넘긴다.

실패 형태 셋과 사례는 `docs/adr/0002-control-mcp-tools-via-permissions.md`의
「`rename_refactoring`의 실패 형태 셋」에 있다.

### UE 에셋

- 에셋 조회는 `get_class_hierarchy`와 `search_assets`로 한다. `search_assets`는 `baseClass`만 쓴다.
  `query`는 빈 결과만 돌려준다.
- 에셋 속성은 Rider로 읽지 않는다. Rider의 `get_asset_properties`는 블루프린트 CDO에
  `properties: []`를 돌려준다. 언리얼 MCP의 `ObjectTools.list_properties`로 읽는다.
- Rider의 에디터 조작 툴은 막혀 있다. 에디터 조작은 언리얼 MCP로 하거나 사람에게 요청한다.

## 언리얼 MCP

- 에디터가 떠 있어야 붙는다. 에디터를 띄우면 `127.0.0.1:8000`이 자동으로 열린다. 연결 확인은
  `netstat`로 8000 포트를 보거나 `list_toolsets`를 한 번 부른다.
- `call_tool` 하나로 830개 툴이 들어오므로 훅이 판정한다. 1층은 툴셋 허용 명단(`UE_ALLOWED_TOOLSETS`)이고,
  아래 세 층은 보관소 경로(`Content/External/`), 슬레이트 조작, `execute_tool_script`다. 에셋 쓰기는
  열려 있다. 명단은 사람만 고친다. 층별 판정과 근거는 `docs/adr/0003-gate-unreal-mcp-by-hook-whitelist.md`.
- 블루프린트의 부모 클래스는 `BlueprintTools.get_parent`로, 그래프는 `list_graphs`와 `read_graph_dsl`로
  읽는다. DSL에는 노드 수가 나오지 않는다.
- UE 자동화 테스트는 언리얼 MCP와 무관하다. `P1/Scripts/Run-UeTests.ps1`이 에디터 없이 돌린다.
