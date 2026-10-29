# recastnavigation (벤더링)

| 항목 | 값 |
|---|---|
| 버전 | v1.6.0 |
| 커밋 | `6dc1667` |
| 출처 | https://github.com/recastnavigation/recastnavigation |
| 라이선스 | zlib (`License.txt`) |
| 포함 범위 | `Recast/{Include,Source}`, `Detour/{Include,Source}`, `License.txt`. DetourCrowd, DetourTileCache, DebugUtils, RecastDemo, 테스트는 넣지 않았다 |

소스는 손대지 않았다.

## 어떻게 쓰이나

| 프로젝트 | 컴파일하는 것 | 쓰는 곳 |
|---|---|---|
| `Tools/NavBuilder/NavBuilder.vcxproj` | Recast, Detour | 추출한 충돌 형상으로 navmesh 파일을 빌드한다 |
| `Server/GameServer/GameServer.vcxproj` | Detour | `Game/Navigation/`이 navmesh를 읽고 질의한다 |
| `Server/GameServerTests/GameServerTests.vcxproj` | Recast, Detour | 빌드 도구의 빌드 함수와 서버의 질의를 함께 테스트한다 |

세 프로젝트 모두 이 소스에 PCH를 쓰지 않는다(`PrecompiledHeader=NotUsing`). 서버 pch는 `using namespace std;`를 전역에 풀어놓기 때문이다.
include 경로는 `Recast/Include`와 `Detour/Include`다.

**빌드 도구와 게임 서버는 반드시 같은 소스를 컴파일한다.** navmesh 파일의 본문은 Detour 타일 데이터를 그대로 담는다. 두 쪽의 Detour가 다르면
서버가 읽은 navmesh가 조용히 틀릴 수 있다. 근거는 `docs/adr/0024-build-server-navmesh-from-extracted-geometry.md`에 있다.

## 버전 올리기

```
git clone --depth 1 --branch <새-태그> https://github.com/recastnavigation/recastnavigation <임시경로>
```

1. `<임시경로>/Recast/{Include,Source}`, `<임시경로>/Detour/{Include,Source}`, `License.txt`로 이 폴더를 통째로 바꾸고, 위 표의 버전과 커밋을 고친다.
   소스 파일이 늘거나 줄었으면 위 세 프로젝트와 `.filters`의 목록도 고친다
2. `Server/GameServer/Game/Navigation/NavMeshFile.h`의 `FORMAT_VERSION`을 올린다
3. 서버 솔루션을 빌드하고 추출 스크립트(`P1/Scripts/Invoke-PlacementExtract.ps1`)를 돌려 navmesh를 다시 만든다
4. `GameServerTests`가 통과하기 전에는 커밋하지 않는다
