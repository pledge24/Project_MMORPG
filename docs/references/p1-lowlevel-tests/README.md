# P1 Low-Level Tests — 냉동 보관 (세션 2, 2026-08-27)

> **이 폴더는 지금 필요하지 않다.** UE 클라의 테스트 경로는 **L2(Simple Automation Test)** 이고,
> 그건 지금 쓰는 런처 설치본 엔진에서 별도 타깃 없이 바로 된다.
> 여기 있는 것은 "나중에 소스 빌드 엔진을 쓰게 되면 꺼내 쓸" 물건이지 **다음 할 일이 아니다.**

## 지금 UE 쪽에서 할 것 — L2

`IMPLEMENT_SIMPLE_AUTOMATION_TEST`가 설치본
`Engine/Source/Runtime/Core/Public/Misc/AutomationTest.h:4297`에 있다(실측).
별도 빌드 타깃이 필요 없고 `P1` 모듈에 그대로 컴파일된다. P1은 아직 하나도 쓰지 않고 있다.

```
P1/Source/P1/Tests/ 에 테스트 .cpp 추가
  → build_solution_start(rootFolder=".../P1")  로 에디터 타깃 빌드
  → 에디터 Window > Test Automation (사람)
     또는  -ExecCmds="Automation RunTests <이름>"
```

실행에 에디터가 필요하므로 사람 손이 섞인다. 착수 전에 계획을 세울 것.

---

## 여기 있는 파일들

`[B] blocked — P1Tests 타깃 빌드 / 런처 설치본(Installed Build) 엔진`

UE 클라의 L1 계층(Catch2 기반 Low-Level Tests)을 세우려고 작성한 파일 3개다.
**파일 자체는 문제가 없다** — 엔진 예제(`Engine/Source/Programs/LowLevelTests/{FoundationTests,MathCoreTests}`)의
구조를 그대로 따랐다. 막은 것은 이 머신의 **엔진 설치 형태**다.

```
> Build.bat P1Tests Win64 Development -Project=.../P1.uproject
Targets with a unique build environment cannot be built with an installed engine.
Result: Failed (RulesError)                                         (exit 8)
```

## 왜 설정으로 우회할 수 없나

표면의 거부는 두 줄이다 (UBT 소스. 로컬 5.7.4 소스 트리 기준 — 설치본 5.8에는 UBT 소스가 없다).

| 위치 | 내용 |
|---|---|
| `TargetRules.cs:2690-2693` | Program 타입 타깃이 `.uproject` 폴더 아래에 있으면 **조건 없이** `TargetBuildEnvironment.Unique` |
| `RulesAssembly.cs:677-680` | `Unique` + `Unreal.IsEngineInstalled()` → 예외를 던지고 중단 |

`TargetRules.cs:2678-2704`의 게터는 `BuildEnvironmentOverride`를 **가장 먼저** 보므로,
타깃에서 `BuildEnvironment = Shared`를 지정하면 저 분기를 실제로 건너뛴다.
커뮤니티에 도는 workaround가 그것이다. **그런데 LLT에는 통하지 않는다.**

`TestTargetRules.SetupCommonProperties`가 조건 없이 이렇게 잡기 때문이다:

```csharp
Type = TargetType.Program;   LinkType = TargetLinkType.Monolithic;
bCompileAgainstEngine = false;      bCompileAgainstEditor = false;
bBuildWithEditorOnlyData = false;   bCompileICU = false;   bBuildDeveloperTools = false;
GlobalDefinitions.Add("STATS=0");  // 외 다수
```

이 목록이 곧 "빌드 환경"의 정의다. 설치본이 주는 프리빌트 바이너리는 **정반대 설정으로**
컴파일돼 있으니, `Shared`로 강제하는 것은 UBT에게 그 바이너리를 재사용하라고 시키는 셈이다.
검사를 통과시켜도 정의가 어긋난 산출물이 나온다. 커뮤니티 workaround는 *굳이 `Unique`를 선언할
필요가 없던* 타깃(CookedEditor류)용이지, 환경이 다른 것 자체가 존재 이유인 LLT용이 아니다.

## 왜 P1/Source/ 에 두지 않았나

`ProjectFileGenerator.cs:3176`이 **프로젝트 파일 생성 시에도** 같은 검증
(`TargetRulesValidationOptions.ValidateTargetAndSDK`)을 수행한다. 파일을 `P1/Source/`에 남겨두면
P1의 프로젝트 파일 재생성이 같은 예외에 걸릴 수 있다. 원인을 이 작업과 연결하기 어려운 형태의
고장이라, 지뢰를 남기는 대신 UBT가 스캔하지 않는 여기로 옮겼다.

## 해동 절차 (소스 빌드 엔진을 쓰게 되었을 때)

`Unreal.IsEngineInstalled()`가 false가 되면 검증을 통과한다. 소스 빌드는
Epic 계정과 GitHub 계정 연결 → 저장소 clone → `Setup.bat` → `GenerateProjectFiles.bat` → 빌드
순서이고, 디스크와 시간 비용이 큰 작업이다. **LLT 하나를 위해 치를 값은 아니다** —
다른 이유로 소스 엔진이 필요해졌을 때 덤으로 가져가는 것이 맞다.

| 이 폴더의 파일 | 되돌릴 위치 |
|---|---|
| `P1Tests.Target.cs` | `P1/Source/P1Tests.Target.cs` |
| `P1Tests.Build.cs` | `P1/Source/P1Tests/P1Tests.Build.cs` |
| `SmokeTest.cpp` | `P1/Source/P1Tests/Private/SmokeTest.cpp` |

```
"<엔진>/Engine/Build/BatchFiles/Build.bat" P1Tests Win64 Development -Project="<리포>/P1/P1.uproject"
# 산출물: P1/Binaries/Win64/P1Tests/P1Tests.exe
```

LLT는 **전용 타깃의 별도 exe**다. Rider의 Unit Tests 창은 L2용이라 이걸 인식하지 못한다 —
그쪽으로 시도하지 말 것.
