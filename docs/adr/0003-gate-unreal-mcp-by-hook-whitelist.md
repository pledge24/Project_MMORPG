---
status: proposed
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

범위는 두 번에 걸쳐 정했다. 첫 판은 조회 계열로 좁게 잡았고, 헤드리스 실행 경로를 확인한
뒤에 진단 계열까지 넓혔다.

넓힌 근거는 역할이 갈렸기 때문이다. **테스트 판정은 이 서버가 하지 않는다.**
`P1/Scripts/Run-UeTests.ps1`이 `UnrealEditor-Cmd`로 에디터 없이 돌리고 리포트로 판정한다.
이 서버는 그 경로가 깨졌을 때 로그와 화면을 보는 진단 수단이다.

| 툴셋 | 허용 | 뺀 것 |
| --- | --- | --- |
| (최상위) | `list_toolsets`, `describe_toolset` | `call_tool` 재귀 호출 |
| `ObjectTools` | 조회 4종 | `set_properties`, `reset_properties` |
| `BlueprintTools` | 조회 22종 | 쓰기 31종 |
| `AutomationTestToolset` | 7종 전부 | - |
| `LogsToolset` | 4종 전부 | - |
| `EditorAppToolset` | 조회 13종과 `StartPIE`, `StopPIE` | UI 조작 6종 |
| `SlateInspectorToolset` | 관찰 6종 | 조작 8종 |
| `AssetTools` | 조회 14종 | 쓰기 7종 |
| `AgentSkillToolset` | `ListSkills`, `GetSkills` | `CreateSkill`, `UpdateSkill` |

합계 76개 조합이다. 나머지 43개 툴셋은 통째로 막힌다.

조회가 아닌데 넣은 것이 셋이다.

- `SetVerbosity`는 메모리상의 로그 상세도만 바꾼다. 에셋이나 설정 파일에 남지 않는다.
- `StartPIE`와 `StopPIE`는 UE 클라가 게임 서버와 인증 서버에 실제로 붙는 3티어 통합
  스모크를 무인으로 돌리기 위한 것이다. 에셋은 바꾸지 않는다.

`AutomationTestToolset`은 실행 도구까지 전부 넣었다. 헤드리스 경로가 생긴 뒤로 판정
수단은 아니지만, 에디터가 이미 떠 있을 때 프로세스 시작 비용을 치르지 않고 테스트만 다시
돌리는 값이 남는다. 관련 항목은 `docs/backlog.md`의 「5. UE L2 Automation Test」다.

### 열지 않은 것

**`ProgrammaticToolset`과 `SlateInspector`의 조작 8종은 열면 이 명단 자체가 무의미해진다.**

`execute_tool_script`는 스크립트 안에서 다른 도구를 부르는 것이 기능이다. 훅에는 호출
한 건으로 보이고 무엇을 불렀는지는 보이지 않는다. 파이썬 자체는 모듈 허용 목록이 있는
샌드박스라서 "임의의 파이썬 실행"은 정확한 표현이 아니다. 위험한 것은 언어가 아니라
도구 단위 통제를 건너뛴다는 점이다.

`SlateInspector`의 `Click`과 `Type`, `PressKey` 같은 조작은 우회 범위가 더 넓다. 에디터
UI로 사람이 할 수 있는 전부가 가능해지고, 거기에는 MCP 도구로 노출되지 않은 메뉴도
들어간다. `Click(ref="w123")`은 기록에 남아도 무엇을 눌렀는지 복원할 수 없다.

**`AssetTools`의 `delete`와 `move`는 에셋이 아니라 폴더를 통째로 받는다.** 그런데
`P1/Content/`는 디스크의 `.uasset`과 `.umap` 3,438개 중 47개만 git이 추적한다
(`P1/.gitignore`의 `Content/*`). 커밋이나 셸브를 먼저 확인하라는 안전 규칙이 여기서는
작동하지 않는다. 추적되지 않는 파일은 커밋에 없기 때문이다. `docs/backlog.md` 7번의 LFS
전환 뒤에 다시 판단한다.

`BlueprintTools`의 쓰기 31종을 뺀 이유도 같은 되돌림 비용이다. `.uasset`은 diff로 검토할
수 없어서 잘못 쓴 것을 사람이 눈으로 찾아내야 한다. 쓰기가 필요해지면 사람 승인을 받고
건별로 `UE_ALLOWED_TOOLS`에 추가한다.

## 실경로에서 무엇을 확인했는가

위의 측정은 `curl`로 던진 것이라 Claude Code 계층이 빠져 있었다. 세션을 재시작한 뒤
같은 날 Claude Code의 MCP 클라이언트로 같은 경로를 다시 쟀다.

| 확인 항목 | 결과 |
| --- | --- |
| `describe_toolset`이 툴셋 정보를 돌려준다 | 통과 |
| `list_properties`가 `BP_MonsterBase`의 속성을 돌려준다 | 통과 (115개, `curl` 측정값과 같다) |
| 훅이 `set_properties`를 막는다 | 통과 (차단 메시지가 돌아왔다) |

**훅이 실경로에서 막는다.** 이전까지 확인한 것은 훅 로직이 올바른 입력에 올바르게
판정한다는 데까지였다. Claude Code가 그 입력을 훅에 넘기는지는 재지 않았다.

`describe_toolset`에는 툴셋의 전체 경로 이름을 넘긴다.
— `ObjectTools`처럼 짧게 넘기면 소켓이 끊긴다. `list_toolsets`가 돌려주는
`editor_toolset.toolsets.object.ObjectTools`를 그대로 쓴다.

### `AutomationTestToolset`은 테스트를 돌린다

`DiscoverTests`에서 `RunTests`까지 끝까지 돌렸다.

| 호출 | 결과 |
| --- | --- |
| `DiscoverTests` | `{"status": "ready"}` |
| `ListTests` (필터 없음) | 8,954개 |
| `ListTests` (`nameFilter`가 `P1`) | 0개 |
| `RunTests` (`System.Core.Math.FColor.Smoke Test`) | `passed: 1`, `failed: 0`, 0.0075초 |

`RunTests`는 테스트별 상태와 오류 목록, 통과·실패 개수를 JSON으로 돌려준다. 판정에 사람 눈이
필요하지 않다. 이 결과에 맞춰 `docs/backlog.md`의 「5. UE L2 Automation Test」 선행 조건을
고쳤다.

`nameFilter`가 `P1`일 때 0개인 것은 이 저장소가 아직 UE 테스트를 하나도 쓰지 않았기 때문이다.
8,954개는 전부 엔진과 플러그인의 테스트다.

### 파생 블루프린트의 오버라이드는 조회된다

ADR-0002가 기록한 Rider의 빈 결과는 이 서버에서 재현되지 않는다.
`ObjectTools.search_subclasses`가 `BP_MonsterBase`의 파생 클래스 13개를 돌려준다. 값은 파생
클래스마다 따로 읽히고, 서로 다른 값이 그대로 구분된다.

| 대상 | `CharacterMesh0`의 `skeletalMeshAsset` |
| --- | --- |
| `BP_Beginner_MeleeMinion` | `Minion_Lane_Melee_Core_Dusk` |
| `BP_Expert_MeleeMinion` | `Minion_Lane_Melee_Dusk` |

**값을 읽을 때는 클래스가 아니라 CDO 경로를 넘긴다.**
— `get_properties`에 `BP_MonsterBase_C`를 넘기면 `could not be read`로 실패한다.
`Default__BP_MonsterBase_C`를 넘겨야 값이 나온다. `list_properties`는 클래스 경로로도
동작하므로 이 차이가 드러나지 않는다.

## 무엇을 확인하지 못했는가

**어떤 값이 이 단계에서 덮인 것인지 한 번에 알려주는 도구는 찾지 못했다.**
— 부모와 자식의 CDO를 각각 읽어 비교하는 것이 지금 아는 유일한 방법이다. 값이 같으면 물려받은
것인지 같은 값으로 덮은 것인지 구분되지 않는다.

**`RunTests`가 에디터를 크게 건드리는 테스트에서도 도는지는 모른다.**
— 실행해 본 것은 순수 수치 연산 테스트 한 건뿐이다. 레벨을 열거나 PIE를 띄우는 테스트는
재지 않았다.

## 결과

- 엔진이 업데이트되어 도구가 늘어나도 통제가 뒤처지지 않는다. 허용 명단 방식이라 새
  도구는 기본으로 막힌다.
- 대신 쓰기 작업을 하려면 매번 훅 파일을 고쳐야 한다. 이 마찰은 의도한 것이다.
- **통제가 세 곳으로 갈라진다.** Rider MCP는 `.claude/settings.json`의 `permissions.deny`가
  막고, 언리얼 MCP는 훅이 막는다. 그리고 훅 파일 자체를 에이전트가 고치려 하면 Claude
  Code의 auto mode classifier가 막는다. 세 번째 계층은 저장소가 만든 것이 아니라서 저장소
  쪽에서 끌 수 없다. **허용 명단을 넓히려면 사람이 직접 파일을 고치거나 권한 규칙을
  추가해야 한다.** 도구가 왜 막혔는지 찾을 때 세 곳을 봐야 한다.
- 서버는 에디터를 띄우면 자동으로 뜬다. 사람이 콘솔에 `ModelContextProtocol.StartServer`를
  입력하지 않아도 된다. 다만 자동 시작 설정이 `P1/Config/`에 없으므로 엔진 기본값이거나 이
  머신의 에디터 설정이다. 다른 클론에서 같은 동작이 보장되지 않는다.
- 에디터가 꺼져 있으면 도구 전체가 연결 오류로 돌아온다. 헤드리스 테스트 경로는 이것과
  무관하게 돈다.
- 에디터 로그가 `LogModelContextProtocol` 항목으로 디스패치된 도구 이름을 전부 남긴다.
  인자는 남기지 않고, 로그는 `P1/Saved/`에 있어 저장소에 남지 않는다.
- 엔진 쪽 구현은 `Engine/Plugins/Experimental/`에 있다. 5.9에서 API가 바뀔 수 있다.
