# P1 Low-Level Tests — 보류 중인 모듈 (세션 2, 2026-08-27)

`[B] blocked — P1Tests 타깃 빌드 / 런처 설치본(Installed Build) 엔진`

UE 클라의 L1 계층(Catch2 기반 Low-Level Tests)을 세우려고 작성한 파일 3개다.
**파일 자체는 문제가 없다** — 엔진 예제(`Engine/Source/Programs/LowLevelTests/{FoundationTests,MathCoreTests}`)의
구조를 그대로 따랐다. 막은 것은 이 머신의 **엔진 설치 형태**다.

## 무엇이 막았나

```
> Build.bat P1Tests Win64 Development -Project=.../P1.uproject
Targets with a unique build environment cannot be built with an installed engine.
Result: Failed (RulesError)                                         (exit 8)
```

UBT 소스에서 원인을 확인했다 (버전은 로컬 5.7.4 소스 트리 기준. 설치본 5.8에는 UBT 소스가 없다):

| 위치 | 내용 |
|---|---|
| `TargetRules.cs:2690-2693` | `Type == TargetType.Program && ProjectFile != null && 타깃 파일이 프로젝트 폴더 아래` → **조건 없이** `TargetBuildEnvironment.Unique` |
| `RulesAssembly.cs:677-680` | `BuildEnvironment == Unique && Unreal.IsEngineInstalled()` → 예외를 던지고 중단 |

LLT 타깃은 `TargetTargetRules`가 아니라 `TestTargetRules`를 상속하는 **Program 타입**이고,
`.uproject` 폴더 안에 있으므로 첫 번째 조건에 무조건 걸린다.
**설정으로 우회할 수 있는 종류가 아니다** — 플래그가 아니라 검증 예외다.

## 왜 P1/Source/ 에 두지 않았나

`ProjectFileGenerator.cs:3176`이 **프로젝트 파일 생성 시에도** 같은 검증
(`TargetRulesValidationOptions.ValidateTargetAndSDK`)을 수행한다. 파일을 `P1/Source/`에 남겨두면
P1의 프로젝트 파일 재생성이 같은 예외에 걸릴 수 있다. 원인을 이 세션과 연결하기 어려운 형태의
고장이라, 지뢰를 남기는 대신 UBT가 스캔하지 않는 여기로 옮겼다.

## 해제 조건

**UE 5.8을 소스 빌드로 설치**하면 `Unreal.IsEngineInstalled()`가 false가 되어 검증을 통과한다.
그 시점에 아래로 되돌리면 된다.

| 이 폴더의 파일 | 되돌릴 위치 |
|---|---|
| `P1Tests.Target.cs` | `P1/Source/P1Tests.Target.cs` |
| `P1Tests.Build.cs` | `P1/Source/P1Tests/P1Tests.Build.cs` |
| `SmokeTest.cpp` | `P1/Source/P1Tests/Private/SmokeTest.cpp` |

빌드·실행:

```
"<엔진>/Engine/Build/BatchFiles/Build.bat" P1Tests Win64 Development -Project="<리포>/P1/P1.uproject"
# 산출물: P1/Binaries/Win64/P1Tests/P1Tests.exe
```

LLT는 **전용 타깃의 별도 exe**다. Rider의 Unit Tests 창은 L2용이라 이걸 인식하지 못한다 —
그쪽으로 시도하지 말 것.

## 소스 빌드 전에 할 수 있는 것

UE 쪽 테스트가 통째로 막힌 것은 아니다. **L2(Simple Automation Test)는 별도 타깃이 필요 없다** —
게임/에디터 모듈 안에 `WITH_DEV_AUTOMATION_TESTS`로 컴파일되어 에디터 안에서 돈다.
설치본 엔진에서 그대로 동작하므로, 소스 빌드를 들이기 전까지 UE 쪽 테스트는 L2부터 시작하는 편이
비용 대비 효과가 낫다.
