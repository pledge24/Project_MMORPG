# googletest (벤더링)

| 항목 | 값 |
|---|---|
| 버전 | v1.18.0 |
| 커밋 | `063de7e` (Prepare for v1.18.0, #5055) |
| 출처 | https://github.com/google/googletest |
| 라이선스 | BSD-3-Clause (`LICENSE`) |
| 포함 범위 | `googletest/include/`, `googletest/src/`, `LICENSE`. **gmock은 제외** |

## 왜 벤더링인가

이 리포는 서드파티를 전부 벤더링한다(`Libraries/include/{google,nlohmann,sw,hiredis}` +
`Libraries/Libs/**`). gtest만 별도 획득 메커니즘(vcpkg/NuGet)을 들이면 의존성 경로가 둘이 된다.
또 프리빌트 바이너리는 툴셋(v145)·CRT 조합을 맞춰줘야 하는데, 소스를 같이 컴파일하면
그 문제가 원천적으로 사라진다. 상세 근거와 기각안은
`docs/decisions/2026-08-27-l1-test-infra.md`.

## 어떻게 쓰이나

`Server/GameServerTests/GameServerTests.vcxproj`가 `src/gtest-all.cc`와 `src/gtest_main.cc`
**두 파일만** 컴파일한다. `gtest-all.cc`가 나머지 `src/*.cc`를 전부 `#include`하는 단일 TU다.

필요한 include 경로는 두 개다. 하나라도 빠지면 컴파일이 깨진다.

- `$(SolutionDir)Libraries\googletest\include` — 사용자 코드의 `#include <gtest/gtest.h>`
- `$(SolutionDir)Libraries\googletest` — `gtest-all.cc` 안의 `#include "src/gtest.cc"` 해석용

두 파일은 PCH를 쓰지 않는다(`PrecompiledHeader=NotUsing`). 서버 pch는 `using namespace std;`를
전역에 풀어놓기 때문에 gtest 소스에 섞이면 안 된다.

## 버전 올리기

```
git clone --depth 1 --branch <새-태그> https://github.com/google/googletest <임시경로>
```
`<임시경로>/googletest/{include,src}`와 `LICENSE`로 이 폴더를 통째로 교체하고, 위 표의
버전·커밋을 갱신한다. 교체 후 `GameServerTests`를 빌드·실행해 통과를 확인하기 전에는 커밋하지 않는다.
