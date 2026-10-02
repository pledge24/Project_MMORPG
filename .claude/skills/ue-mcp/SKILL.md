---
name: ue-mcp
description: 언리얼 MCP(`unreal`)의 이 저장소 전용 함정. `mcp__unreal__call_tool`로 에디터의 에셋과 블루프린트를 조회하거나 고치거나 저장하기 전에 사용한다. 범용 사용법은 플러그인 스킬 `unreal-mcp`에 있다.
---

# 언리얼 MCP

범용 사용법은 플러그인 스킬 `unreal-mcp`에 있다. 아래는 이 저장소에서만 걸리는 것이다.

- `call_tool`의 `toolset_name`에는 전체 이름을 쓴다(`editor_toolset.toolsets.blueprint.BlueprintTools`).
  `unreal-mcp`의 예시처럼 짧은 이름(`BlueprintTools`)을 쓰면 훅의 허용 명단과 맞지 않아 막힌다. 전체 이름은
  `list_toolsets`가 돌려준다.
- `call_tool`은 훅이 판정한다. 1층은 툴셋 허용 명단(`UE_ALLOWED_TOOLSETS`)이고, 아래 세 층은 보관소 경로
  (`Content/External/`), 슬레이트 조작, `execute_tool_script`다. 에셋 쓰기는 열려 있다. 명단은 사람만 고친다.
  층별 판정과 근거는 `docs/adr/0003-gate-unreal-mcp-by-hook-whitelist.md`.
- 변경 표시(dirty)가 없는 에셋은 `AssetTools.save_assets`가 `true`를 돌려주면서 저장을 건너뛴다. 저장 뒤에는
  파일 수정 시각이나 내용으로 확인한다. 리다이렉트로 불러온 에셋에는 변경 표시가 붙지 않고, 같은 부모로
  `set_parent`를 다시 불러도 붙지 않는다. 위젯 하나의 변수 표시를 `UMGToolSet.ToggleWidgetAsVariable`로 켰다
  끄면 값은 그대로 두고 표시만 붙는다.
- `AssetTools.update_metadata_tags`의 `remove_tags`는 `could not convert incoming function input params Json
  to a UStruct`로 실패한다. 붙인 태그를 지울 수 없으니 변경 표시를 붙이려고 태그를 쓰지 않는다.
- `ObjectTools.set_properties`는 배열 요소를 바꾸면서 개수도 줄이는 변경을 `ArrayRemove: elements changed
  alongside the size change`로 거부한다. `reset_properties`로 비운 뒤 새 요소를 넣는다.
