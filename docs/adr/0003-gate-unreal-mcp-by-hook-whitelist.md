---
status: accepted
---

# 언리얼 엔진 MCP 서버를 훅 허용 명단으로 통제한다

언리얼 에디터에 내장된 MCP 서버를 `unreal`이라는 이름으로 `.mcp.json`에 등록한다.
이 서버는 도구를 개별로 노출하지 않아서 ADR-0002가 쓴 `permissions.deny`로는 하나도
구분해 낼 수 없다. 대신 `.claude/hooks/guard_dangerous_cmd.py`가 유일한 통제 지점이 된다.
**허용 명단에 없는 툴셋은 막는다.**

**2026-09-22에 판정 구조를 도구 단위 단층에서 툴셋 단위 네 층으로 바꿨다.** 이 문서의
측정값은 대부분 2026년 9월 16~17일 것이고, 바뀐 자리에는 개정 날짜를 함께 적었다.

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
세 개뿐이다. **52개 툴셋에 걸린 830개의 도구가 전부 `call_tool`의 인자로 들어온다.**
2026년 9월 17일에 툴셋 52개를 모두 `describe_toolset`으로 조회해 합산한 값이다. 최상위
3개까지 더하면 이 서버가 내놓는 도구는 833개다.

가장 큰 툴셋 다섯이 전체의 절반에 가깝다.

| 툴셋 | 도구 수 |
| --- | --- |
| `SequencerTools` | 140 |
| `SequencerControlRigTools` | 72 |
| `BlueprintTools` | 53 |
| `NiagaraToolset_System` | 46 |
| `ControlRigTools` | 44 |

시퀀서 계열 여섯 툴셋만 합쳐도 275개인데 이 프로젝트는 하나도 쓰지 않는다. 명단을 좁게
잡는 비용이 크지 않은 이유다.

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

범위는 세 번에 걸쳐 정했다. 첫 판은 조회 계열로 좁게 잡았고, 헤드리스 실행 경로를 확인한
뒤에 진단 계열까지 넓혔다. **2026-09-22에 단위를 툴셋으로 바꾸고 쓰기를 열었다.**

넓힌 근거는 역할이 갈렸기 때문이다. **테스트 판정은 이 서버가 하지 않는다.**
`P1/Scripts/Run-UeTests.ps1`이 `UnrealEditor-Cmd`로 에디터 없이 돌리고 리포트로 판정한다.
이 서버는 그 경로가 깨졌을 때 로그와 화면을 보는 진단 수단이다.

### 현행 판정 (2026-09-22 개정)

| 층 | 무엇을 보는가 | 지금 값 |
| --- | --- | --- |
| 1층 | 툴셋이 `UE_ALLOWED_TOOLSETS`에 있는가 | 27개. 모르는 툴셋은 막는다 |
| 2층 | 그 안에서 개별로 막은 도구인가(`UE_DENIED_TOOLS`) | 비어 있다. 사고가 나면 한 줄 적는다 |
| 3층 | 조회가 아닌 도구가 `/Game/External`을 인자로 받았는가 | 보관소 가드(ADR-0006). 예외 둘 |
| 4층 | 슬레이트 조작과 `execute_tool_script`인가 | 아래 두 항목 |

**뺀 43개 툴셋은 시퀀서와 나이아가라와 컨트롤 리그와 GAS 계열이다.** 이 프로젝트가 쓰지
않는다. 최상위 도구는 `list_toolsets`와 `describe_toolset`만 열고 `call_tool` 재귀 호출은 뺀다.

**3층에 예외가 둘 있다**(2026-09-22 추가). `duplicate`의 `path`와 `import_file`의
`source_file`은 검사하지 않는다. 훅의 `UE_EXTERNAL_SOURCE_ARGS`가 그 목록이다.
— 이 둘은 보관소를 읽기만 한다. `duplicate`는 원본을 남기고 `import_file`은 디스크의 이미지를
읽는다. 목적지 인자(`new_path`, `folder_path`)는 그대로 검사하므로 보관소를 **목적지로** 쓰는
것은 계속 막힌다. `move`는 출발지를 비우므로 예외에 넣지 않았다.

**이 예외는 직접 호출에만 적용된다.** `execute_tool_script` 안의 호출은 인자가 JSON 문자열이라
어느 쪽이 목적지인지 정적으로 가릴 수 없다. 그래서 보관소에서 복사할 때는 스크립트로 묶지 않고
한 건씩 부른다. #81에서 139개를 이 방식으로 복사했다.

4층이 둘을 따로 본다.

- **슬레이트 조작 8종은 워킹 트리가 깨끗할 때만 통과한다.** `ref`가 익명이라
  `Click(ref="i1")`은 로그로 복원되지 않는다. 직전 상태가 커밋되어 있으면 결과를 diff로 읽는다.
- **`execute_tool_script`는 스크립트를 `ast`로 파싱해 안의 호출에 1~3층을 다시 적용한다.**
  런타임에 조합한 문자열은 잡지 못한다. 막으려는 것은 의도적 우회가 아니라 오판이다.

조회가 아닌데 넣은 것이 둘이다.

- `SetVerbosity`는 메모리상의 로그 상세도만 바꾼다. 에셋이나 설정 파일에 남지 않는다.
- `StartPIE`와 `StopPIE`는 UE 클라가 게임 서버와 인증 서버에 실제로 붙는 3티어 통합
  스모크를 무인으로 돌리기 위한 것이다. 에셋은 바꾸지 않는다.

`AutomationTestToolset`은 실행 도구까지 전부 넣었다. 헤드리스 경로가 생긴 뒤로 판정
수단은 아니지만, 에디터가 이미 떠 있을 때 프로세스 시작 비용을 치르지 않고 테스트만 다시
돌리는 값이 남는다. 관련 항목은 `docs/backlog.md`의 「5. UE L2 Automation Test」다.

### 왜 쓰기를 열었는가 (2026-09-22)

**LFS로 전환하면서 `P1/Content/` 아래 501개가 전부 git에 추적되기 시작했다.** 쓰기를 막아
둔 근거가 되돌림 비용이었는데 그 비용이 내려갔다. 아래 「개정 전에 열지 않은 것」이 예고한
재판단이다.

| 항목 | 개정 전 | 개정 후 |
| --- | --- | --- |
| 판정 단위 | 도구 (830개 중 74개) | 툴셋 (52개 중 27개) |
| 에셋 쓰기 | 막았다 | 연다 |
| `ProgrammaticToolset`과 슬레이트 조작 | 막았다 | 4층 판정을 붙여 연다 |

검증은 단위 테스트 93건 통과와 실경로에서 네 층을 각각 확인한 것이다. 근거는 PR #96이다.

### 개정 전에 열지 않은 것 (2026-09-17 기준)

**아래 세 문단 중 앞의 둘은 지금도 유효하다.** 그래서 4층이 생겼다. 셋째 문단의 판단은
2026-09-22에 뒤집혔다.

**`ProgrammaticToolset`과 `SlateInspector`의 조작 8종은 그냥 열면 이 명단 자체가 무의미해진다.**

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
수 없어서 잘못 쓴 것을 사람이 눈으로 찾아내야 한다.

**2026-09-22에 이 두 문단의 판단을 뒤집었다.** LFS 전환으로 501개가 전부 추적되면서 전제가
사라졌다. 위 「왜 쓰기를 열었는가」에 경위가 있다.

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
- 대신 새 툴셋이 필요할 때마다 사람이 훅 파일을 고쳐야 한다. 이 마찰은 의도한 것이다.
  **열린 27개 툴셋 안에서는 쓰기도 돈다**(2026-09-22 개정).
- **통제가 세 곳으로 갈라진다.** Rider MCP는 `.claude/settings.json`의 `permissions.deny`가
  막고, 언리얼 MCP는 훅이 막는다. 그리고 훅 파일 자체를 에이전트가 고치려 하면 Claude
  Code의 auto mode classifier가 막는다. 세 번째 계층은 저장소가 만든 것이 아니라서 저장소
  쪽에서 끌 수 없다. **허용 명단을 넓히려면 사람이 직접 파일을 고치거나 권한 규칙을
  추가해야 한다.** 도구가 왜 막혔는지 찾을 때 세 곳을 봐야 한다.
- 서버는 에디터를 띄우면 자동으로 뜬다. 사람이 콘솔에 `ModelContextProtocol.StartServer`를
  입력하지 않아도 된다. **2026-09-22에 출처를 확인했다.** 이 머신의
  `P1/Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini`에 `bAutoStartServer=True`와
  `ServerPortNumber=8000`이 있다. 이 파일은 per-user라 git이 추적하지 않으므로, 다른 클론에서
  같은 동작이 보장되지 않는다는 판단은 그대로다.
- 에디터가 꺼져 있으면 도구 전체가 연결 오류로 돌아온다. 헤드리스 테스트 경로는 이것과
  무관하게 돈다.
- 에디터 로그가 `LogModelContextProtocol` 항목으로 디스패치된 도구 이름을 전부 남긴다.
  인자는 남기지 않고, 로그는 `P1/Saved/`에 있어 저장소에 남지 않는다.
- 엔진 쪽 구현은 `Engine/Plugins/Experimental/`에 있다. 5.9에서 API가 바뀔 수 있다.
