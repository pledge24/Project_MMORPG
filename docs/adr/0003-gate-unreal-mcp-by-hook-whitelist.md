---
status: accepted
---

# 언리얼 엔진 MCP 서버를 훅 허용 명단으로 통제한다

언리얼 에디터에 내장된 MCP 서버를 `unreal`이라는 이름으로 `.mcp.json`에 등록한다.
이 서버는 도구를 개별로 노출하지 않아서 ADR-0002가 쓴 `permissions.deny`로는 하나도
구분해 낼 수 없다. 대신 `.claude/hooks/guard_dangerous_cmd.py`의 `UE_ALLOWED_TOOLS`가
유일한 통제 지점이 된다. **명단에 없는 도구는 막는다.**

## 무엇을 재고 결정했는가

2026년 9월 16일에 측정했다. 엔진은 5.8.1이고, 서버 주소는 `http://127.0.0.1:8000/mcp`다.
Claude Code 계층을 배제하려고 `curl`로 JSON-RPC를 직접 던졌다. ADR-0002가 Rider를
판정할 때 쓴 방법과 같다.

| 측정 항목 | 미리 적어 둔 예상값 | 실측 |
| --- | --- | --- |
| `tools/list`의 도구 수 | 200종 이상 | **3종** |
| 툴셋 수 | 30개 이상 | 52개 |
| `BP_MonsterBase`의 CDO 속성 수 | 1개 이상 | **115개** |

세 번째 줄이 도입을 결정한 근거다. ADR-0002는 Rider의 `get_asset_properties`가 같은
블루프린트에서 `properties: []`를 돌려준다고 기록해 두었다. 엔진 내장 서버는 115개를
돌려준다. 그 안에는 이 저장소가 직접 정의한 `monsterData`, `attackSystemComponent`,
`creatureName`이 들어 있고, `monsterData`에는 `"고정된 데이터"`라는 이 저장소의 한국어
UPROPERTY 메타데이터가 그대로 실려 있다. 스텁이 아니라 실제 CDO를 읽는다.

`.uasset`은 바이너리라 파일로 읽을 수 없다. 이 구멍에는 대체 경로가 없다.

## permissions로는 통제되지 않는다

서버가 MCP 수준에 노출하는 도구는 `list_toolsets`, `describe_toolset`, `call_tool`
세 개뿐이다. 52개 툴셋에 걸린 수백 개의 도구는 전부 `call_tool`의 인자로 들어온다.

```
call_tool(toolset_name="...ObjectTools", tool_name="list_properties", arguments={...})
```

`permissions`는 `mcp__unreal__call_tool`이라는 이름 하나만 본다. 그래서 선택지가
전부 허용과 전부 차단 두 가지로 줄어든다. 세분화가 가능한 지점은 인자를 실제로 읽는
PreToolUse 훅뿐이다.

## 차단 명단이 아니라 허용 명단을 쓴다

ADR-0002는 65종을 골라 막는 차단 명단을 썼다. 이 서버에는 그 방식을 쓰지 않는다.

**모르는 도구는 막는다.**
— 엔진이 업데이트되면 툴셋과 도구가 늘어나고, 늘어난 것들도 전부 `call_tool` 하나로
들어온다. 차단 명단은 그때마다 반드시 뒤처진다. ADR-0002가 "새 도구가 생기면 자동으로
허용된다"고 적어 둔 약점이 여기서는 훨씬 빠르게 벌어진다.

## 무엇을 허용했는가

첫 판은 조회 계열로 좁게 잡는다. 실측으로 값어치가 확인된 것이 조회뿐이기 때문이다.

| 툴셋 | 허용 | 뺀 것 |
| --- | --- | --- |
| (최상위) | `list_toolsets`, `describe_toolset` | `call_tool` 재귀 호출 |
| `ObjectTools` | 조회 4종 | `set_properties`, `reset_properties` |
| `BlueprintTools` | 조회 22종 | 쓰기 31종 |
| `AutomationTestToolset` | 7종 전부 | - |

나머지 49개 툴셋은 통째로 막힌다. `ProgrammaticToolset`도 여기 들어간다.
— 이 툴셋의 `execute_tool_script`는 에디터 프로세스 안에서 임의의 파이썬을 실행한다.
Epic의 README가 직접 경고하는 경로다.

`BlueprintTools`의 쓰기 31종을 뺀 이유는 되돌림 비용이다. `.uasset`은 diff로 검토할 수
없어서, 잘못 쓴 것을 사람이 눈으로 찾아내야 한다. 쓰기가 필요해지면 사람 승인을 받고
건별로 `UE_ALLOWED_TOOLS`에 추가한다.

`AutomationTestToolset`은 실행 도구인 `RunTests`, `RunTestsByFilter`, `StopTests`까지
넣었다. UE 클라이언트 테스트를 무인으로 돌리는 것이 이 서버를 들인 목적 중 하나이기
때문이다. 근거는 `docs/backlog.md`의 클라이언트 L1 항목이다.

## 무엇을 확인하지 못했는가

**`AutomationTestToolset`이 실제로 테스트를 돌리는지 확인하지 않았다.**
— 툴셋이 목록에 있다는 사실만 봤다. CLAUDE.md 「완료 기준」의 "존재 ≠ 가능"에 그대로
해당한다. `docs/backlog.md`의 클라이언트 L1 제약이 풀렸다고 쓰지 않는다.

**파생 블루프린트의 오버라이드 조회를 측정하지 않았다.**
— 속성 조회 결과가 판정을 확정해서 세 번째 기준을 재지 않았다. ADR-0002가 기록한
`find_default_value_overrides`의 빈 결과가 이 경로에서도 재현되는지는 모른다.

## 결과

- 엔진이 업데이트되어 도구가 늘어나도 통제가 뒤처지지 않는다. 허용 명단 방식이라 새
  도구는 기본으로 막힌다.
- 대신 쓰기 작업을 하려면 매번 훅 파일을 고쳐야 한다. 이 마찰은 의도한 것이다.
- **통제가 `permissions`와 훅 두 곳으로 갈라진다.** Rider MCP는 `.claude/settings.json`의
  `permissions.deny`가 막고, 언리얼 MCP는 훅이 막는다. 도구가 왜 막혔는지 찾을 때 두 곳을
  봐야 한다.
- 서버는 사람이 에디터 콘솔에서 `ModelContextProtocol.StartServer`를 입력해야 뜬다.
  에디터가 꺼져 있으면 도구 전체가 연결 오류로 돌아온다.
- 엔진 쪽 구현은 `Engine/Plugins/Experimental/`에 있다. 5.9에서 API가 바뀔 수 있다.
