# 언리얼 엔진 MCP 서버가 할 수 있는 것

> 대상: 언리얼 엔진 5.8 런처 설치본에 내장된 `ModelContextProtocol` 플러그인
> 조사일: 2026년 9월 17일
> 방법: 실행 중인 에디터의 서버(`http://127.0.0.1:8000/mcp`)에 JSON-RPC로 직접 붙어
> 툴셋 52개를 모두 `describe_toolset`으로 조회하고 도구 830개의 이름과 설명을 집계했다

---

## 이 서버는 무엇인가

**에디터 프로세스 안에서 도는 원격 조작 API다.** 에디터를 띄우면 HTTP 서버가 함께 뜨고,
붙은 쪽에서 에셋과 레벨과 에디터 상태를 읽고 바꿀 수 있다. 사람이 에디터 UI로 하는 일의
상당 부분에 대응하는 함수가 있다.

에디터가 꺼져 있으면 아무것도 되지 않는다. 별도의 헤드리스 서비스가 아니라 **에디터의
부속물**이다.

---

## 규모

| 항목 | 값 |
| --- | --- |
| 툴셋 | 52 |
| 툴셋 소속 도구 | 830 |
| 최상위 MCP 도구 | 3 |
| **합계** | **833** |

도구 수가 툴셋마다 크게 다르다. 상위 5개가 전체의 43%를 차지한다.

| 툴셋 | 도구 수 |
| --- | --- |
| `SequencerTools` | 140 |
| `SequencerControlRigTools` | 72 |
| `BlueprintTools` | 53 |
| `NiagaraToolset_System` | 46 |
| `ControlRigTools` | 44 |
| `PCGToolset` | 30 |
| `UMGToolSet` | 23 |
| `SkeletalMeshTools` · `MaterialTools` · `DataflowAgentToolset` · `SequencerKeyframingTools` | 각 22 |
| `AssetTools` · `EditorAppToolset` | 각 21 |
| 나머지 40개 툴셋 | 314 |

---

## 접속 구조

MCP 수준에 노출되는 이름은 세 개뿐이다.

```
list_toolsets      툴셋 52개의 이름과 설명
describe_toolset   툴셋 하나의 도구 목록과 입출력 스키마
call_tool          실제 호출. 830개가 전부 여기로 들어온다
```

830개는 전부 `call_tool`의 **인자**로 구분된다.

```
call_tool(
  toolset_name = "editor_toolset.toolsets.object.ObjectTools",
  tool_name    = "list_properties",
  arguments    = { "instance": { "refPath": "..." } }
)
```

이 구조가 뒤에서 다룰 통제 문제의 뿌리다.

---

## 능력 지도

### 오브젝트와 에셋 — 27개

`ObjectTools` 6 · `AssetTools` 21

`.uasset`은 바이너리라 텍스트로 읽을 수 없는데, 이 경로로는 내용이 보인다. 오브젝트의 속성
목록과 값을 읽고(`list_properties`, `get_properties`), 쓰고(`set_properties`), 인스턴스
오버라이드를 걷어낸다(`reset_properties`). 클래스 계층을 훑는 `search_subclasses`도 있다.

**값을 읽을 때는 CDO(Class Default Object) 경로를 넘긴다.** 블루프린트 생성 클래스 경로를
`get_properties`에 넘기면 `could not be read`로 실패하고, `Default__`가 붙은 경로를 넘겨야
값이 나온다. `list_properties`는 클래스 경로로도 동작해서 이 차이가 잘 드러나지 않는다.

`AssetTools`는 에셋을 찾고(`find_assets`) 옮기고 지우고 복제한다(`move`, `delete`,
`duplicate`). **`delete`와 `move`는 에셋이 아니라 폴더도 받는다.** 텍스트 파일을 읽고 쓰는
`read_file`과 `write_file`도 있는데, 쓰기는 `/Game/`과 활성 플러그인의 `Content/`, 프로젝트
`Saved/`로 제한된다.

의존 관계를 양방향으로 조회하는 `get_dependencies`와 `get_referencers`가 있다. 에셋을 고치기
전에 무엇이 깨지는지 알 수 있다.

### 블루프린트 — 53개

`BlueprintTools`

**그래프를 텍스트 DSL로 읽고 쓴다.** `read_graph_dsl`과 `write_graph_dsl`이 그 축이고,
`get_graph_dsl_docs`가 문법을 알려준다.

노드 단위 조작도 전부 있다. 노드를 만들고(`create_node`) 배선하고(`connect_pins`,
`break_pins`) 지우고(`delete_node`), 핀 값을 읽고 쓰고(`get_pin_value`, `set_pin_value`),
자동 정렬한다(`arrange_nodes`).

변수는 타입별로 추가한다(`add_variable`, `add_struct_variable`, `add_object_variable`).
카테고리, 리플리케이션, 인스턴스 편집 가능 여부를 각각 설정한다. 함수 그래프와 이벤트를
만들고(`add_function_graph`, `add_event`) 파라미터를 붙인다. 부모 클래스도 바꾼다
(`set_parent`).

탐색 쪽으로는 `find_nodes`, `find_node_types`, `get_connected_subgraph`,
`list_compatible_event_functions`가 있다. 마지막으로 `compile_blueprint`로 컴파일한다.

**그래프를 짜는 일 전체가 프로그램으로 가능하다.** 이 서버에서 가장 깊은 편집 능력이다.

### UMG 위젯 — 23개

`UMGToolSet`

위젯 블루프린트를 만들고(`CreateWidgetBlueprint`), 위젯 트리를 짠다. 위젯을 붙이고
(`AddWidget`) 옮기고(`MoveWidget`) 지우고(`RemoveWidget`) 이름을 바꾼다(`RenameWidget`).
다른 위젯으로 감싸는 `WrapWidgets`, 자식으로 치환하는 `ReplaceWidgetWithChild`,
네임드 슬롯을 다루는 `SetNamedSlotContent`도 있다.

이벤트 프로퍼티에 바인딩하고(`BindToEventProperty`), 위젯을 변수로 승격하고
(`ToggleWidgetAsVariable`), 컴파일한다(`CompileWidgetBlueprint`).

이 툴셋의 설명에 **작업 순서가 못 박혀 있다.** 위젯과 슬롯의 속성 이름은 클래스마다 달라서
추측할 수 없으므로, `ObjectTools.list_properties`로 정확한 이름을 먼저 얻고 `get_properties`와
`set_properties`를 쓰라고 적혀 있다. 이 단계를 건너뛰면 `set_properties`가 조용히 실패하거나
엉뚱한 속성을 바꾼다.

### 레벨과 액터 — 41개

`SceneTools` 20 · `ActorTools` 17 · `PrimitiveTools` 4

레벨을 열고(`load_level`) 액터를 배치한다. 클래스에서 만들거나
(`add_to_scene_from_class`) 에셋에서 만들고(`add_to_scene_from_asset`) 지운다
(`remove_from_scene`). 아웃라이너 폴더를 만들고 옮기고 지운다. 액터를 병합하고
(`merge_actors`), 레벨 인스턴스를 만들고 편집하고 커밋한다. 월드에 레이캐스트를 쏘는
`trace_world`와 콜리전 채널 조회도 있다.

액터 쪽은 트랜스폼과 라벨과 태그를 읽고 쓰고, 컴포넌트를 붙이고 떼고(`add_component`,
`remove_component`), 부모 컴포넌트를 바꾸고, 특정 방향을 보게 한다(`look_at`).

`PrimitiveTools`는 큐브·구·실린더·콘을 액터에 붙인다. 프로토타이핑용이다.

### 데이터 에셋 — 34개

`DataTableTools` 10 · `CurveTableTools` 9 · `StringTableTools` 8 · `DataRegistryTools` 7 ·
`DataAssetTools` 1

데이터 테이블의 행을 읽고 쓰고 이름을 바꾸고 지운다. 스키마를 조회하고(`get_schema`),
행 구조체를 검색하고(`search_row_structs`), 파일에서 가져온다(`import_file`).
커브 테이블은 행에 더해 키 단위 조작(`get_keys`, `set_keys`, `add_key`)이 있다.
스트링 테이블은 키와 항목, 네임스페이스를 다룬다.

`DataRegistryTools`는 조회만 있다. 레지스트리와 데이터 소스, 아이템과 스키마를 나열한다.

### 메시·머티리얼·텍스처·피직스 — 92개

`SkeletalMeshTools` 22 · `MaterialTools` 22 · `StaticMeshTools` 16 ·
`PhysicsAssetToolset` 17 · `MaterialInstanceTools` 13 · `TextureTools` 2

**스켈레탈 메시**는 본 계층을 훑고(`get_bone_names`, `get_bone_parent`, `get_bone_children`),
소켓을 추가·이동·삭제하고, 머티리얼 슬롯을 다루고, 피직스 에셋을 연결한다
(`assign_physics_asset`). 모프 타깃 이름도 얻는다.

**스태틱 메시**는 LOD를 생성하고 임계값을 설정하고(`generate_lods`, `set_lod_thresholds`),
볼록 콜리전을 만들고 지우고, Nanite를 켜고 끈다. 정점 수와 삼각형 수를 센다.

**머티리얼**은 그래프 편집이다. 표현식 노드를 추가·삭제하고(`add_expression`,
`delete_expression`), 서로 연결하고 출력에 잇고(`connect_expressions`, `connect_to_output`),
자동 정렬하고(`layout_expressions`), 쓰지 않는 표현식을 정리하고(`delete_unused_expressions`),
재컴파일한다. 파라미터 컬렉션과 머티리얼 함수도 만든다.

**머티리얼 인스턴스**는 스칼라·벡터·텍스처·스태틱 스위치 파라미터를 읽고 쓰고, 부모를
바꾸고, 오버라이드를 걷어낸다.

**피직스 에셋**은 메시에서 생성하고(`CreateFromMesh`), 바디와 제약을 추가·삭제하고, 모양을
캡슐·구·박스로 바꾸고, 질량 배율과 물리 모드를 설정한다.

**텍스처**는 크기 조회와 파일 가져오기 두 개뿐이다.

### 시네마틱과 리깅 — 319개

`SequencerTools` 140 · `SequencerControlRigTools` 72 · `ControlRigTools` 44 ·
`SequencerKeyframingTools` 22 · `SequencerOutlinerTools` 18 · `SequencerConditionTools` 9 ·
`SequencerCustomBindingTools` 8 · `SequencerImportExportTools` 6

**전체의 38%가 여기에 있다.** 이 서버가 가장 깊게 파고든 영역이다.

`SequencerTools`는 시퀀스를 만들고 열고 재생하고(`create_level_sequence`, `play`, `pause`,
`set_playhead_frame`), 액터를 바인딩하고(`add_actors`, `add_spawnable_from_class`), 트랙과
섹션을 추가·삭제하고, 섹션의 범위·이징·블렌드 타입을 설정한다. 폴더로 정리하고, 마크된
프레임을 관리하고, 서브 시퀀스를 오간다. 카메라를 만들고 카메라 컷을 건다. 복사·붙여넣기
계열만 여덟 개다.

`SequencerKeyframingTools`는 채널에 키를 찍는다(`add_key_float`, `add_key_bool`,
`add_key_integer`, `add_key_string`). 커브 에디터를 열고 키를 선택하고 굽는다.

`SequencerControlRigTools`는 시퀀서 안에서 컨트롤 리그를 애니메이션한다. 컨트롤 값을
타입별로 읽고 쓰고(`set_position`, `set_rotator`, `set_euler_transform` 등 20여 개), 키를
찍고, 스페이스를 전환하고(`set_space`, `bake_space`), 애님 레이어를 추가·병합·재정렬하고,
포즈를 미러링하고, 트윈과 블렌드를 건다. FBX를 리그로 넣고 뺀다.

`ControlRigTools`는 리그 에셋 자체를 만든다. 계층에 본과 널과 컨트롤을 추가하고, 그래프에
노드를 놓고 배선하고, 변수를 만들고, 포워드 솔브·백워드 솔브·인터랙션 그래프를 다룬다.

`SequencerOutlinerTools`는 아웃라이너 트리의 접기·펼치기, 뮤트·솔로·잠금·핀 상태를 다룬다.

### 이펙트 — 56개

`NiagaraToolset_System` 46 · `NiagaraToolset_Component` 4 · `NiagaraToolset_Assets` 3 ·
`NiagaraToolset_Blueprint` 2 · `NiagaraToolset_Info` 1

나이아가라 시스템을 만들고(`CreateNiagaraSystem`), 이미터와 모듈과 렌더러를 추가·삭제하고,
스택 입력 값을 읽고 쓴다. 스키마와 토폴로지를 조회하는 도구가 유난히 많은데, 스택 구조를
모르면 값을 어디에 넣을지 알 수 없기 때문으로 보인다. `GetStackIssues`로 문제를 찾고
`ApplyStackIssueFix`로 고치는 쌍도 있다.

`NiagaraToolset_Assets`는 에셋 레지스트리에서 스크립트를 찾는다. `LoadObject` 없이 도는 것이
특징이다. `NiagaraToolset_Blueprint`는 시스템을 감싸는 블루프린트 래퍼를 만든다.

### 절차적 생성 — 53개

`PCGToolset` 30 · `DataflowAgentToolset` 22 · `PCGSpatialToolset` 1

둘 다 그래프 편집 툴셋이다. 노드를 추가·삭제·이동하고 핀을 잇고 끊고, 코멘트 박스를 붙이고,
그래프 파라미터를 다룬다. PCG는 그래프 인스턴스를 스폰하고 실행하는 것까지
(`SpawnGraphInstance`, `ExecuteGraphInstance`) 된다. `DrawSpline`으로 스플라인을 그리는
도구도 있다.

### 게임플레이 프레임워크 — 45개

`GameplayCueToolset` 8 · `StateTreeTools` 9 · `BehaviorTreeTools` 7 ·
`ConversationTools` 7 · `GameplayTagsToolset` 6 · `AbilitySystemInspectorToolset` 4 ·
`AttributeSetToolset` 2 · `WorldConditionTools` 2

**대부분 조회 전용이다.** 비헤이비어 트리, 스테이트 트리, 대화 그래프, 월드 컨디션은 읽기만
된다. 구조를 훑고 노드를 나열하는 데까지다. 편집은 지원하지 않는다.

게임플레이 태그는 예외로 추가·삭제·이름 변경이 되고, 태그로 참조자를 찾는
`FindReferencersByTag`가 있다. 게임플레이 큐도 태그를 추가·삭제하고 노티파이 에셋을 만들고
선택된 액터에 큐를 실행한다.

`AbilitySystemInspectorToolset`은 런타임 상태를 본다. 부여된 어빌리티, 어트리뷰트 값, 활성
태그, 활성 이펙트다. 액터 포인터를 받으므로 PIE 중에 쓰는 도구다.

### 에디터 상태와 화면 — 21개

`EditorAppToolset`

**화면을 이미지로 받을 수 있다.** 뷰포트 캡처(`CaptureViewport`)는 격자와 액터 라벨을
오버레이하는 옵션이 있다. 3D 공간에 좌표 격자를 투영하고 액터마다 십자선과 지시선 라벨을
그려서, 시각 인식이 되는 쪽이 공간을 파악하고 배치를 지시할 수 있게 만든 것이다. 에디터 창
전체 캡처(`CaptureEditorImage`)와 에셋 썸네일 렌더(`CaptureAssetImage`)도 있다.

**PIE를 제어한다.** `StartPIE`는 PIE와 Simulate를 고를 수 있고, 스폰 트랜스폼을 덮을 수 있고,
`WarmupSeconds`로 초기화가 끝날 때까지 기다린다. 엔진이 `PostPIEStarted`를 쏜 뒤에 완료되므로
"떴다"는 신호가 확실하다. 아웃오브프로세스 모드는 인프로세스로 강등된다.

그 밖에 선택 상태와 열린 에셋과 보이는 액터를 조회하고, 콘텐츠 브라우저 경로를 옮기고,
뷰포트 카메라를 설정하고, 화면 좌표와 월드 좌표를 변환한다(`ScreenCoordsToWorld`는 레이캐스트
결과를 준다).

**CVar는 검색만 된다.** `SearchCVars`가 있고 값을 바꾸는 도구는 없다.

### 에디터 UI 자동화 — 14개

`SlateInspectorToolset`

Playwright 방식으로 에디터 UI를 다룬다. **접근성 스냅샷으로 위젯 트리를 텍스트로 읽고
(`Snapshot`), ref를 얻어 그 위젯을 클릭하고 입력한다.** 클릭(`Click`), 타이핑(`Type`),
키 입력(`PressKey`), 콤보박스 선택(`SelectOption`), 폼 일괄 입력(`FillForm`),
드래그(`Drag`), 호버(`Hover`)가 있다. 창을 나열하고 앞으로 가져오고 닫는 `Windows`도 있다.

`Observe`로 특정 서브트리에 관찰자를 등록하면 약 100밀리초마다 트리를 걸으며 새로 나타난
위젯에 ref를 부여한다. 다 쓰면 `Unobserve`로 해제한다.

입력은 `AutomationDriver`가 아니라 Slate 이벤트 API를 직접 호출한다. 툴셋 설명에 이유가
적혀 있다. `AutomationDriver`의 동기 API가 게임 스레드에서 호출되면 교착에 빠지는데, MCP 도구
호출이 바로 게임 스레드에서 실행되기 때문이다.

**이 툴셋은 다른 모든 도구의 상위 집합에 가깝다.** UI로 할 수 있는 일이면 MCP로 노출되지
않은 메뉴 항목도 클릭할 수 있다.

### 로그 — 4개

`LogsToolset`

세션 로그를 읽는다. 카테고리로 거르고 정규식으로 맞추고 개수를 제한한다
(`GetLogEntries`). 등록된 카테고리를 나열하고(`GetLogCategories`), 카테고리별 상세도를 읽고
바꾼다(`GetVerbosity`, `SetVerbosity`).

### 자동화 테스트 — 7개

`AutomationTestToolset`

Session Frontend가 쓰는 `IAutomationControllerManager`를 그대로 감싼다. 정해진 순서가 있다.

```
DiscoverTests → ListTests → RunTests 또는 RunTestsByFilter → GetTestStatus / GetTestResults
```

`DiscoverTests`는 워커를 찾고 테스트 목록을 올린다. 수 초가 걸리고 세션당 한 번이면 된다.
`RunTestsByFilter`는 이름 목록을 넘기는 `RunTests`보다 빠르다. 엔진이 리포트 트리를 한 번에
좁히기 때문이다. 필터 문법은 접두사(`StartsWith:` 또는 `^`), 접미사(`$`), 부분 문자열, 그리고
ini에 정의된 그룹(`Group:`)을 `+`로 잇는다.

결과는 테스트별 상태와 소요 시간, 오류와 경고 목록이 담긴 JSON이다. 판정에 사람 눈이
필요하지 않다.

### 프로젝트 인프라 — 32개

`PluginToolset` 17 · `ConfigSettingsToolset` 8 · `GameFeaturesToolset` 7

플러그인을 만들고(`CreatePlugin`) 켜고 끄고(`SetPluginEnabled`), 디스크립터를 읽고 쓰고,
의존성을 추가·삭제하고, 의존하는 쪽과 의존받는 쪽을 조회한다. 어떤 에셋이 어느 플러그인
소속인지도 알려준다(`GetPluginForAsset`).

`ConfigSettingsToolset`은 프로젝트 설정 섹션을 다룬다. 스키마와 현재 값을 읽고, 속성을 쓰고,
저장하고, 기본값으로 되돌린다. `.ini`를 바꾸는 경로다.

`GameFeaturesToolset`은 Game Feature 플러그인의 상태를 조회하고 활성화·비활성화한다.

### 검색 — 2개

`SemanticSearchToolset`

SemanticSearch 플러그인의 벡터 + BM25 하이브리드 검색을 노출한다. 자연어로 에셋을 찾는
`Search`와 비슷한 에셋을 찾는 `FindSimilar` 둘이다. 이름 규칙이 일정하지 않은 프로젝트에서
값이 크다.

### 에이전트 스킬 — 4개

`ToolsetRegistry.AgentSkillToolset`

프로젝트 안에 `AgentSkill` 에셋을 만들고 고친다(`CreateSkill`, `UpdateSkill`). 경로가
`/Game/Skills/MySkill.MySkill_C` 형태이므로 Claude Code의 스킬이 아니라 **프로젝트 에셋**이다.
목록과 상세를 조회하는 `ListSkills`, `GetSkills`도 있다.

두 쓰기 도구의 설명에 **"사용자의 명시적 지시나 허락을 받은 뒤에만 부를 것"**이 엔진 쪽에서
직접 적혀 있다.

### 도구 오케스트레이션 — 2개

`ProgrammaticToolset`

**다른 도구 호출을 파이썬 스크립트 하나로 묶는다.** `execute_tool_script`가 그것이고,
`get_execution_environment`가 쓸 수 있는 모듈과 작성 규칙을 알려준다. 스크립트는 `run()`
함수를 정의해야 하고 딕셔너리를 돌려줘야 한다.

**임의의 파이썬 실행이 아니다.** 모듈 허용 목록이 있고 허용되지 않은 모듈을 import하면
`ValueError`로 거부된다. 툴셋 설명도 "목적은 도구 오케스트레이션이지 범용 파이썬 실행이
아니다"라고 못 박는다.

목적은 왕복 횟수와 컨텍스트를 줄이는 것이다. 열 번 부를 것을 한 번에 끝낸다.

---

## 조회만 되는 영역

도구 이름 접두사로 판정하면 **9개 툴셋 43개가 조회 전용**이다.

| 툴셋 | 도구 수 |
| --- | --- |
| `StateTreeTools` | 9 |
| `BehaviorTreeTools` | 7 |
| `ConversationTools` | 7 |
| `DataRegistryTools` | 7 |
| `AbilitySystemInspectorToolset` | 4 |
| `NiagaraToolset_Assets` | 3 |
| `AttributeSetToolset` | 2 |
| `SemanticSearchToolset` | 2 |
| `WorldConditionTools` | 2 |

`NiagaraToolset_Info`의 `UEnum_Info` 하나까지 세면 10개 툴셋 44개다.

**비헤이비어 트리와 스테이트 트리를 편집할 수 없다는 뜻이다.** AI 로직 에셋은 읽기만 된다.
그래프 편집이 지원되는 것은 블루프린트, 머티리얼, 컨트롤 리그, 나이아가라, PCG, Dataflow
여섯이다.

---

## 할 수 없는 것

830개를 이름과 설명으로 훑어 확인했다.

**애님 몽타주와 애님 시퀀스를 편집할 수 없다.**
— 이름이나 설명에 `montage`가 들어간 도구가 하나도 없다. 애니메이션 관련 도구는 전부
시퀀서와 컨트롤 리그 쪽이고, 애니메이션 에셋 자체를 편집하는 툴셋이 없다. 시퀀서에서
애님 시퀀스로 굽거나(`export_anim_sequence`) 링크하는 것(`link_anim_sequence`)까지다.
`SkeletalMeshTools`도 메시와 소켓과 본까지이고 애니메이션은 다루지 않는다.

**임의의 콘솔 명령을 실행할 수 없다.**
— `EditorAppToolset`의 설명에 "console variables"가 있지만 실제로 노출된 것은
`SearchCVars` 하나이고 값을 바꾸는 도구는 없다. 이름에 `exec`가 들어간 셋은 전부 다른
것이다. `ExecuteCueOnSelectedActor`는 게임플레이 큐, `ExecuteGraphInstance`는 PCG 그래프,
`execute_tool_script`는 도구 오케스트레이션이다.

**C++ 소스를 편집하거나 모듈을 빌드할 수 없다.**
— `compile_blueprint`, `CompileWidgetBlueprint`, `recompile`은 전부 에셋 컴파일이다.

**에디터를 끄거나 켜거나 재시작할 수 없다.**
— `quit`, `restart`, `shutdown`에 해당하는 도구가 없다. 다만 `SlateInspector`의 `Windows`가
최상위 창을 닫으므로 메인 창을 닫는 방식은 남아 있다.

**실행 취소와 다시 실행이 없다.**
— `undo`, `redo`가 없다. 에디터 트랜잭션 스택에 접근하는 경로가 노출되지 않는다.
**잘못 쓴 것을 되돌리는 버튼이 없다는 뜻이다.**

**소스 컨트롤에 쓸 수 없다.**
— 체크아웃 여부와 편집 가능 여부는 조회되지만(`is_checked_out`, `can_edit_asset`), 체크아웃·
제출·되돌리기 도구는 없다.

**쿡·패키징·배포가 없다.**
— `cook`, `package`, `deploy`가 전부 0개다.

**내비메시와 라이팅 빌드가 없다.**
— 해당 이름의 도구가 없다.

---

## 운영상의 성질

**에디터를 띄우면 서버가 자동으로 뜬다.** 사람이 콘솔에 명령을 입력할 필요가 없다. 다만 그
설정이 프로젝트 `Config/`에 있지 않으므로 엔진 기본값이거나 머신의 에디터 설정이다. 다른
환경에서 같은 동작이 보장되는지는 확인하지 못했다.

**포트는 하나다.** 두 번째 에디터 인스턴스를 띄우면 서버가 `127.0.0.1:8000` 바인딩에 실패하고
로그에 한 줄을 남긴 뒤 그냥 진행한다.

```
LogHttpListener: Error: HttpListener unable to bind to 127.0.0.1:8000
```

먼저 뜬 인스턴스의 서버는 영향을 받지 않는다. 헤드리스 커맨드렛과 에디터를 동시에 돌려도
서로 방해하지 않는다.

**호출이 에디터 로그에 남는다.** 디스패치된 도구 이름이 `LogModelContextProtocol` 카테고리에
기록된다. 인자는 남기지 않는다.

```
LogModelContextProtocol: Dispatching toolset tool: 'AutomationTestToolset.AutomationTestToolset.RunTests'
```

**도구 호출은 게임 스레드에서 실행된다.** `SlateInspectorToolset`이 `AutomationDriver`를 쓰지
않는 이유가 이것이다.

**구현이 `Engine/Plugins/Experimental/`에 있다.** 엔진 버전이 올라가면 도구가 늘거나 이름이
바뀔 수 있다.

---

## 통제하기 어려운 이유

이 서버를 도입할 때 미리 알아야 할 구조적 성질이 셋 있다.

### 도구 단위 권한이 작동하지 않는다

MCP 클라이언트가 보는 이름은 `list_toolsets`, `describe_toolset`, `call_tool` 셋뿐이다.
830개가 전부 `call_tool`의 인자로 들어오므로, **도구 이름으로 허용·차단을 나누는 통상적인
수단은 전부 허용이냐 전부 차단이냐로 줄어든다.** 세분화하려면 호출 인자를 읽는 계층이 따로
있어야 한다.

### 우회 경로가 둘 있다

**`ProgrammaticToolset.execute_tool_script`**
— 스크립트 안에서 다른 도구를 부르는 것이 이 도구의 기능이다. 인자를 검사하는 쪽에는 호출
한 건으로 보이고 무엇을 몇 개나 불렀는지는 보이지 않는다. 파이썬 자체는 샌드박스지만 위험한
것은 언어가 아니라 도구 단위 통제를 건너뛴다는 점이다.

**`SlateInspectorToolset`의 조작 도구**
— 우회 범위가 더 넓다. 에디터 UI로 사람이 할 수 있는 전부가 가능해지고, 거기에는 MCP로
노출되지 않은 메뉴도 들어간다. 게다가 `Click(ref="w123")`은 기록에 남아도 무엇을 눌렀는지
복원할 수 없다. ref는 런타임에만 의미가 있기 때문이다.

**둘 중 하나가 열려 있으면 나머지 세분화는 사실상 무의미해진다.**

### 되돌림 비용이 비대칭이다

조회는 되돌릴 것이 없다. 반면 쓰기는 `.uasset`이 바이너리라 diff로 검토할 수 없다. 실행 취소
도구도 없다. 그리고 `AssetTools`의 `delete`와 `move`는 폴더를 통째로 받는다.

Epic의 문서는 **"로컬호스트는 신뢰 경계가 아니다"**라고 직접 경고한다. 서버는
`127.0.0.1:8000`에 바인딩하고 origin 검증을 하지만, 같은 사용자로 도는 프로세스면 붙을 수
있다. 공유 머신에서는 전제가 달라진다.

---

## 조사 방법과 한계

**방법**

실행 중인 에디터의 서버에 JSON-RPC로 붙어(`initialize` → `notifications/initialized` →
`tools/call`) `list_toolsets`로 툴셋 52개를 얻고, 각각에 `describe_toolset`을 불러 `tools`
배열의 이름과 설명 첫 줄을 모았다. 결과는 830줄이다.

**한계**

- **도구 이름과 설명 첫 줄까지만 봤다.** 각 도구가 실제로 무엇을 하고 어떤 인자를 받는지는
  스키마와 구현을 읽어야 확정된다. 이 문서의 「할 수 있는 것」은 그 수준의 근거 위에 있다.
- **실제로 호출해 본 것은 일부다.** 확인한 것은 `ObjectTools` 4종,
  `AutomationTestToolset` 3종, `LogsToolset` 2종, `AssetTools` 1종, 그리고 최상위
  `describe_toolset`과 `list_toolsets`다. 나머지는 스키마만 읽었다.
- **조회 전용 판정은 이름 접두사에 기댄 것이다.** 이름이 `get`이나 `list`로 시작하지 않으면서
  조회만 하는 도구가 있을 수 있다. `NiagaraToolset_Info`의 `UEnum_Info`가 그런 예다.
- **엔진 5.8 기준이다.** Experimental 플러그인이라 다음 버전에서 구성이 바뀔 수 있다.
