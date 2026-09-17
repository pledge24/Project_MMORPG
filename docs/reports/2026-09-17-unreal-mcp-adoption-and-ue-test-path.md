# 언리얼 MCP 도입 판단과 UE 테스트 실행 경로 구축

> 날짜: 2026년 9월 17일
> 브랜치: `chore/gate-unreal-mcp-by-hook` (기준 `dev`)
> 이 세션이 만든 커밋: `4684203` ~ `2664aa2` (12개)
> 이 세션 이전 커밋: `fd17d25` (직전 세션이 만들었다. 이 브랜치에 포함되지만 여기서 한 일이 아니다)

아래에 적힌 수치는 전부 이 세션에서 실제로 명령을 돌려 얻은 값이다. 돌리지 않은 것은
「6. 검증」의 「돌리지 않은 것」에 따로 적었다.

---

## 1. 한눈에

언리얼 에디터에 내장된 MCP 서버를 계속 쓸지 판단하려고 실경로를 측정했고, 그 과정에서
**UE 클라이언트 테스트를 사람 손 없이 돌리는 경로**를 찾아 구축했다. 핵심 발견은
`UnrealEditor-Cmd`가 **테스트가 실패해도 종료 코드 `0`을 돌려준다**는 것이다. 이 저장소의
「종료 코드가 판정」 규약이 이 티어에만 통하지 않는다는 뜻이라, 리포트 JSON을 읽어 판정하는
래퍼 스크립트를 만들어 구멍을 막았다.

결과로 UE 클라이언트에 첫 자동화 테스트 1개가 생겼고, 에디터를 띄우지 않고 한 줄로 돌아간다.
MCP 허용 명단은 조회·진단 계열까지 76개 조합으로 넓혔고, 통과한 호출을 기록하는 감사 로그와
`P1/Content` 스냅샷 스크립트를 함께 넣었다.

**리뷰어가 먼저 알아야 할 것**: ADR-0003의 `status`가 `accepted`에서 `proposed`로 내려갔다.
도입 판단이 아직 끝나지 않았다는 뜻이고, 이 브랜치는 판단에 필요한 재료를 모으는 성격이다.

---

## 2. 무엇이 바뀌었나

### 2.1 UE 테스트 실행 경로가 생겼다

`P1/Scripts/Run-UeTests.ps1` (신규)

이전에는 UE 클라이언트 테스트를 돌릴 방법이 사람이 에디터에서 `Window > Test Automation`을
여는 것뿐이었다. `docs/backlog.md` 5번이 그 제약을 "사람 손이 섞인다"로 적어 두고 있었다.

`UnrealEditor-Cmd`에 `-ExecCmds="Automation RunTests ..."`와 `-unattended`를 주면 에디터 창
없이 돈다는 것을 확인했다. 다만 그대로 쓰면 안 되는 이유를 두 가지 측정했다.

| 상황 | `UnrealEditor-Cmd`의 종료 코드 | 리포트 |
| --- | --- | --- |
| 테스트 1개 통과 | `0` | `index.json` 생성 |
| 필터가 아무것도 못 맞춤 | `0` | **생성 안 됨** |
| 테스트 1개 실패 | `0` | `index.json` 생성, `failed: 1` |

**세 경우가 전부 `0`이다.** 종료 코드로 판정하면 오타 난 필터도, 깨진 테스트도 초록으로
보인다. 그래서 스크립트가 `-ReportExportPath`의 `index.json`을 읽어 아래 네 조건을 전부
만족할 때만 `0`으로 끝난다.

1. `index.json`이 존재한다
2. `succeeded`가 1 이상이다
3. `failed`가 0이다
4. `notRun`과 `inProcess`가 0이다

실행 직전에 `index.json`을 먼저 지운다. 지우지 않으면 지난 실행이 남긴 초록 리포트를 이번
결과로 읽는다.

### 2.2 UE 클라이언트에 첫 자동화 테스트가 생겼다

`P1/Source/P1/Tests/PacketFramingTest.cpp` (신규)

모든 송신이 거치는 `ClientPacketHandler::MakeSerializedPacket`의 바이트 배치를 고정한다.
헤더 4바이트에 `size`와 `id`가 이 순서로 실리는 것, 본문이 헤더 바로 뒤에서 왕복하는 것,
빈 메시지가 헤더만 싣는 경계를 검사한다.

인자 하나짜리 공개 오버로드(`MakeSerializedPacket(Protocol::C_CHAT&)`)를 부른다. 그래서
메시지 타입이 어느 패킷 id로 가는지도 함께 걸린다.

서버 쪽 짝은 `GameServerTests`의 `PacketSerialization` 스위트다. 이 저장소는 클라와 서버의
클래스 계층이 대칭인 구조라, 같은 규약을 양쪽에서 잡는 형태가 된다.

### 2.3 언리얼 MCP 허용 명단을 넓혔다

`.claude/hooks/guard_dangerous_cmd.py`

이 서버는 MCP 수준에 도구를 3종만 노출하고 **830개 도구가 전부 `call_tool`의 인자로
들어온다.** 그래서 `permissions`로는 구분되지 않고, 훅의 `UE_ALLOWED_TOOLS`가 유일한 통제
지점이다. 명단에 없으면 막힌다.

| | 이전 | 지금 |
| --- | --- | --- |
| 툴셋 | 4 / 52 | 9 / 52 |
| 조합 | 35 | 76 |
| 열린 툴셋 도구 비율 | — | 74 / 830 (8.9%) |

새로 연 것은 아래 다섯 툴셋이다.

| 툴셋 | 연 것 | 뺀 것 |
| --- | --- | --- |
| `EditorToolset.LogsToolset` | 4종 전부 | 없음 |
| `EditorToolset.EditorAppToolset` | 조회 13종과 `StartPIE`, `StopPIE` | UI 조작 6종 |
| `SlateInspectorToolset` | 관찰 6종 | 조작 8종 |
| `editor_toolset.toolsets.asset.AssetTools` | 조회 14종 | 쓰기 7종 |
| `ToolsetRegistry.AgentSkillToolset` | `ListSkills`, `GetSkills` | `CreateSkill`, `UpdateSkill` |

**조회가 아닌데 넣은 것이 셋이다.**

- `SetVerbosity`는 메모리상의 로그 상세도만 바꾼다. 에셋이나 설정 파일에 남지 않는다.
- `StartPIE`와 `StopPIE`는 3티어 통합 스모크를 무인으로 돌리기 위한 것이다. 에셋은 바꾸지
  않는다.

훅 테스트에 15건을 더해 무엇을 열고 무엇을 닫았는지를 고정했다.

### 2.4 통과한 호출을 기록하는 감사 로그를 넣었다

`.claude/hooks/guard_dangerous_cmd.py`의 `_record_ue_call`

기존 `block_counter.log`는 **막힌 호출만** 센다. 그런데 사고는 통과한 호출에서 난다.
무엇이 통과했는지 기록이 없으면 사후에 되짚을 근거가 없다. 명단을 76개로 넓힌 직후라 이
구멍이 커졌다.

`ue_audit.log`에 TSV 한 줄로 시각, 툴셋, 도구, 인자 요약을 남긴다.

```
2026-09-17T01:07:45+09:00	EditorToolset.LogsToolset	GetLogCategories	{"filter": "ModelContextProtocol"}
```

인자 요약은 400자에서 자른다. 기록에 실패해도 조용히 넘어가고 판정을 뒤집지 않는다. 기존
`_record_block`과 같은 원칙이다.

`.gitignore`에 넣었다. 머신 로컬 기록이고 브랜치를 오갈 때 합쳐질 수 없는 성질의 파일이다.

### 2.5 `P1/Content` 스냅샷 스크립트를 넣었다

`P1/Scripts/Backup-UeContent.ps1` (신규)

`P1/.gitignore`의 `Content/*` 규칙 때문에 디스크의 `.uasset`·`.umap` 3,438개 중 git이
추적하는 것은 47개뿐이다. 나머지는 지워지면 되돌릴 방법이 없다. 커밋이나 셸브를 먼저
확인하라는 저장소의 안전 규칙도 여기서는 작동하지 않는다. 추적되지 않는 파일은 커밋에 없기
때문이다.

`robocopy /MIR`로 저장소 밖에 한 벌을 둔다. 기본 경로는 `D:\Unreal\Snapshots\P1-Content`이고
환경 변수 `P1_CONTENT_SNAPSHOT`으로 덮는다. `-Restore`로 되돌린다.

**미러가 한 벌이라 가장 큰 위험은 사고가 난 뒤에 스냅샷을 덮어쓰는 것이다.** 그러면 그물이
사라진다. 그래서 원본 파일 수가 지난 스냅샷보다 10% 넘게 줄었으면 거부하고 `-Restore`를
안내한다.

`docs/backlog.md` 7번의 LFS 전환이 끝나면 이 스크립트는 버린다. git이 같은 일을 더 잘한다.

### 2.6 인증 서버 CI를 올렸다

`.github/workflows/ci.yml` (신규)

`push(dev)`와 `pull_request`에서 `windows-latest` + Node 22로 `npm test`와 `npm run lint`를
돌린다.

**세 티어 중 하나만 들어갔다.** 나머지 둘은 호스티드 러너에서 빌드되지 않는다.

| 티어 | 막는 것 |
| --- | --- |
| 게임 서버 | `.vcxproj`의 `PlatformToolset`이 `v145`. GitHub `windows-2025` 러너는 VS 2022(17.14)이고 툴셋이 `v143`까지다 |
| UE 클라 | UE 5.8 런처 설치본이 필요하다. 러너에 엔진이 없다 |

우회하지 않고 멈췄다. 막는 지점과 선택지는 `docs/backlog.md` 4번에 적었다.

### 2.7 문서를 실측에 맞췄다

| 파일 | 무엇이 바뀌었나 |
| --- | --- |
| `docs/adr/0003-...md` | `status`를 `proposed`로. 허용 명단 표를 9툴셋 기준으로. 「열지 않은 것」 절 신설. 통제 계층을 둘에서 셋으로. StartServer 서술 정정 |
| `CLAUDE.md` | StartServer 자동 시작, 열린 범위에 PIE 추가, 명단을 에이전트가 못 고친다는 사실, 완료 기준에 UE 패킷 프레이밍 추가, UE 테스트의 종료 코드 예외 |
| `docs/testing.md` | UE 클라 L2 행 추가, 종료 코드 판정의 예외 명시, seam 표에 클라 패킷 프레이밍 추가, TDD 범위 표 갱신 |
| `docs/backlog.md` | 4번(CI)에 막는 조건 실측치, 5번(UE L2)의 선행 조건 제거 |
| `docs/tech-debt.md` | 항목 1개 추가 (16개 → 17개) |

---

## 3. 지금은 어떻게 도는가

### UE 클라이언트 테스트

```
이전
  ↓
사람이 에디터를 연다 → Window > Test Automation → 눈으로 초록·빨강을 본다

지금
  ↓
pwsh P1/Scripts/Run-UeTests.ps1 → 종료 코드로 판정한다
```

출력은 이런 형태다.

```
필터: P1
리포트: D:\Unreal\Projects\Project_MMORPG\P1\Saved\UeTestReport
에디터 종료 코드: 0 (판정에 쓰지 않는다)
소요: 17.1초
결과: 성공 1 · 실패 0 · 미실행 0 · 진행중 0 · 경고동반성공 0
[통과] 테스트 1 건이 전부 성공했다.
```

「에디터 종료 코드: 0 (판정에 쓰지 않는다)」를 출력에 직접 박아 두었다. 이 줄을 보고 에디터
종료 코드를 판정에 쓰려는 시도를 막는다.

실패하면 실패한 테스트 경로와 단언 메시지가 함께 나온다.

```
결과: 성공 0 · 실패 1 · 미실행 0 · 진행중 0
  [Fail] P1.Network.SendBuffer.Boundary
      Expected '새 버퍼의 WriteSize' to be 1, but it was 0.
[실패] 성공한 테스트가 없다.
```

**남은 사람 손은 하나다.** 빌드에는 에디터를 닫아야 한다. `docs/build.md:141`이 적어 둔
제약으로, 에디터가 떠 있으면 `UnrealEditor-P1.dll`을 덮어쓸 수 없어 빌드가 실패한다.
테스트 실행 자체는 에디터와 무관하다.

### 언리얼 MCP 도구 호출

```
이전
  ↓
조회 35개 조합만 통과. 차단만 기록에 남는다.

지금
  ↓
조회·진단 76개 조합이 통과. 통과한 호출도 ue_audit.log 에 인자와 함께 남는다.
에셋 쓰기, 임의 스크립트 실행, UI 조작은 여전히 막힌다.
```

### 에셋 사고 대비

```
이전
  ↓
되돌릴 수단 없음. 3,438개 중 47개만 git 이 안다.

지금
  ↓
pwsh P1/Scripts/Backup-UeContent.ps1 로 저장소 밖에 미러를 둔다.
사고 후 다시 뜨려 하면 거부하고 -Restore 를 안내한다.
```

---

## 4. 중요한 결정

### 4.1 테스트 판정을 MCP가 아니라 헤드리스 커맨드렛에 맡긴다

**결정**: `UnrealEditor-Cmd` + 래퍼 스크립트를 판정 경로로 삼고, 언리얼 MCP는 진단용으로
남긴다.

**이유**: MCP 경로는 에디터가 떠 있어야 하고, 에디터가 떠 있으면 빌드가 실패한다. 즉 코드를
고치고 테스트하는 한 바퀴가 MCP 경로만으로는 닫히지 않는다. 헤드리스 경로는 에디터와 무관하게
돌고 CI로 그대로 이어진다.

**함께 검토한 대안**: 언리얼 MCP의 `AutomationTestToolset`만으로 판정하는 안. 세션 초반에는
이쪽을 본선으로 보고 `RunTests`까지 허용 명단에 열어 두었다. 헤드리스 경로가 14.5초와 47.3초에
끝나는 것을 측정한 뒤 역할을 바꿨다.

**결과**: 허용 명단에서 `AutomationTestToolset`은 그대로 열려 있지만 역할이 "판정"에서
"에디터가 이미 떠 있을 때의 빠른 반복"으로 내려갔다. 명단을 넓힐 때 `LogsToolset`과
`SlateInspector` 관찰 도구의 우선순위가 그만큼 올라갔다.

### 4.2 종료 코드를 판정에 쓰지 않는다

**결정**: 래퍼가 `index.json`을 읽어 판정하고 자기 종료 코드를 낸다. 에디터의 종료 코드는
화면에 기록만 한다.

**이유**: 세 가지 경우를 측정했는데 전부 `0`이었다. 특히 **테스트가 실제로 실패한 경우에도
`0`**이다. 이것은 세션 중반까지 몰랐고, 첫 UE 테스트를 일부러 빨강으로 써 본 뒤에야 드러났다.

**결과**: 이 저장소의 「판정은 종료 코드다」 규약에 티어 하나가 예외로 생겼다. 그 예외를
`CLAUDE.md` 「완료 기준」과 `docs/testing.md` 양쪽에 적었다. 규약을 아는 사람이 이 티어에서
습관대로 판정하면 틀린다.

### 4.3 쓰기 도구를 열지 않는다

**결정**: 조회와 진단만 연다. 에셋 쓰기, `ProgrammaticToolset`, `SlateInspector`의 조작 8종,
`AssetTools`의 `delete`와 `move`는 닫아 둔다.

**이유**: 둘로 나뉜다.

- **앞의 둘은 열면 명단 자체가 무의미해진다.** `execute_tool_script`는 스크립트 안에서 다른
  도구를 부르는 것이 기능이라, 훅에는 호출 한 건으로 보이고 내용은 보이지 않는다.
  `SlateInspector`의 조작은 우회 범위가 더 넓다. 에디터 UI로 사람이 할 수 있는 전부가
  가능해지고 여기에는 MCP로 노출되지 않은 메뉴도 들어간다. `Click(ref="w123")`은 기록에
  남아도 무엇을 눌렀는지 복원할 수 없다.
- **`delete`와 `move`는 되돌릴 수 없다.** 두 도구는 에셋이 아니라 폴더를 통째로 받는다.
  그런데 `P1/Content`는 3,438개 중 47개만 git이 추적한다.

**함께 검토한 대안**: `SlateInspector`만 열고 `ProgrammaticToolset`은 닫는 안을 검토했다.
조사해 보니 위험 순서가 반대였다. 조작 계열의 우회 범위가 `ProgrammaticToolset`보다 넓다.
그래서 툴셋 단위가 아니라 도구 단위로 갈라 관찰 6종만 열었다.

**결과**: LFS 전환(`docs/backlog.md` 7번)이 끝나면 "되돌릴 수 없다"는 근거가 사라지므로
`delete`와 `move`, `BlueprintTools` 쓰기를 다시 판단할 수 있다. ADR-0003에 그렇게 적었다.

### 4.4 테스트 대상을 `SendBuffer`에서 패킷 프레이밍으로 바꿨다

**결정**: 첫 UE 테스트의 대상을 `ClientPacketHandler::MakeSerializedPacket`으로 잡았다.

**이유**: 처음에는 `SendBuffer`의 `Append`와 `Copy`를 잡고 빨강·초록을 모두 확인했다. 그 뒤에
**두 함수를 부르는 곳이 저장소에 없다**는 것을 확인했다. 모든 송신은 `Buffer()`에 직접 쓰고
`Close()`로 끝낸다. 죽은 코드를 고정하는 테스트는 남길 값어치가 없다.

**결과**: 실제로 모든 송신이 거치는 경로를 잡게 됐고 서버 스위트와 대칭이 됐다. `SendBuffer`의
죽은 코드는 `docs/tech-debt.md`에 기록했다.

### 4.5 ADR-0003의 `status`를 `proposed`로 내렸다

**결정**: 사용자 지시에 따라 `accepted`에서 `proposed`로 바꿨다.

**이유**: 저장소는 이미 도입한 상태로 커밋돼 있었지만(`.mcp.json` 등록, 훅 matcher 추가) 도입
판단 자체는 끝나지 않은 상태였다. 문서가 사실과 어긋나 있었다.

**결과**: 이 브랜치는 "도입 확정"이 아니라 "판단 재료 수집"으로 읽어야 한다. 머지 전에
도입 여부를 사람이 정한다.

---

## 5. 발견

### 5.1 `UnrealEditor-Cmd`는 실패해도 종료 코드 `0`이다

이 세션에서 가장 중요한 발견이다. 세 경우를 측정했고 전부 `0`이었다. 「2.1」의 표에 있다.

「매칭 0건」일 때 `0`이 나오는 것은 세션 중반에 알았고, **실제 테스트 실패에서도 `0`**이라는
것은 첫 UE 테스트를 빨강으로 써 본 뒤에야 드러났다. 순서가 반대였다면 래퍼를 만들지 않았을
가능성이 크다.

### 5.2 헤드리스 인스턴스와 떠 있는 에디터가 공존한다

에디터가 떠 있는 상태에서 헤드리스 인스턴스를 띄웠을 때 충돌하지 않았다. 로그에 한 줄이
남는다.

```
LogHttpListener: Error: HttpListener unable to bind to 127.0.0.1:8000
```

두 번째 인스턴스도 MCP 서버를 자동으로 띄우려다 포트가 이미 잡혀 있어 실패하고 그냥
진행한다. 여기서 두 가지가 동시에 확인된다.

- **MCP 서버는 에디터를 띄우면 자동으로 뜬다.** 사람이 콘솔에 `ModelContextProtocol.StartServer`를
  입력할 필요가 없다. 세 문서가 반대로 적고 있어서 고쳤다.
- 헤드리스 실행이 떠 있는 에디터의 8000 포트를 빼앗지 않는다.

**자동 시작 설정은 `P1/Config/`에 없다.** 엔진 기본값이거나 이 머신의 에디터 설정이다. 다른
클론에서 같은 동작이 보장되는지는 확인하지 못했다.

### 5.3 허용 명단을 에이전트가 고칠 수 없다

훅 파일의 `UE_ALLOWED_TOOLS`를 편집하려 했을 때 Claude Code의 auto mode classifier가 막았다.

```
Permission for this action was denied by the Claude Code auto mode classifier.
```

자기를 제한하는 통제 파일을 스스로 넓히는 편집이라 막힌 것으로 보인다. 우회하지 않고 사람이
직접 붙여넣었다.

**이 결과로 통제 계층이 둘이 아니라 셋이라는 것이 드러났다.** `permissions`, 훅, 그리고 auto
mode classifier다. 세 번째는 저장소가 만든 것이 아니라서 저장소 쪽에서 끌 수 없다. ADR-0003과
`CLAUDE.md`에 적었다.

주석 편집은 통과하고 명단 편집만 막혀서, 한동안 주석과 실제 명단이 어긋난 상태가 있었다.
지금은 맞춰져 있다.

### 5.4 `SendBuffer`의 `Append`와 `Copy`는 죽은 코드다

모든 송신이 `MakeSerializedPacket`을 거치는데 이 함수는 `Buffer()`로 받은 포인터에 직접 쓰고
`Close()`로 기록 위치만 맞춘다. 저장소 전체에서 두 함수를 부르는 곳이 없다.

가드 조건도 서로 다르다. `Copy`는 `Len()`을, `Append`는 `FreeSize()`를 본다. 쓰는 쪽이 없어서
이 차이가 의도인지 실수인지 확인할 수 없다.

`Append`에는 경계 결함도 있다. 버퍼가 가득 찬 상태(`_writePos == Len()`)에서 `len`이 0 이하면
가드를 통과한 뒤 `WritePos()`가 `_buffer[Num()]`을 인덱싱한다. 지금은 부르는 곳이 없어
드러나지 않는다.

`docs/tech-debt.md`에 기록만 하고 고치지 않았다.

### 5.5 `get_properties`는 클래스 경로로 실패한다

`list_properties`는 `BP_MonsterBase_C` 같은 클래스 경로로 동작하지만, `get_properties`는 같은
경로에 `could not be read`로 실패한다. 값은 CDO에 있어서 `Default__BP_MonsterBase_C`를 넘겨야
나온다. `list_properties`가 클래스 경로로도 되기 때문에 이 차이가 잘 드러나지 않는다.

ADR-0003에 적었다.

### 5.6 훅이 자기 문서를 설명하는 명령도 막는다

`Remove-Item -Recurse`와 `New-Item -Force`가 한 명령 문자열 안에 함께 들어가자 훅이
`Remove-Item -Recurse -Force` 패턴으로 판정해 막았다. 훅은 명령 문자열 전체를 보고 선행 탐색을
쓰기 때문이다.

`CLAUDE.md` 「안전」이 이미 적어 둔 성질이다. 예외를 추가하지 않고 명령을 다시 구성해서
풀었다.

### 5.7 CI에서 두 티어가 막힌다

| 확인한 것 | 값 |
| --- | --- |
| GitHub `windows-2025` 러너 | Visual Studio Enterprise 2022 17.14, 툴셋 v142·v143 |
| 이 저장소 서버 프로젝트 | `PlatformToolset` `v145` |
| 저장소 공개 여부 | **PUBLIC** |

게임 서버는 툴셋이 맞지 않아서, UE 클라는 엔진이 없어서 호스티드 러너에서 빌드되지 않는다.
둘을 풀려면 self-hosted 러너가 필요한데, **저장소가 공개라 포크에서 온 PR이 그 머신에서
코드를 실행하게 된다.** 이 판단이 먼저라 손대지 않았다.

### 5.8 서버가 내놓는 도구는 830개다

세션 마지막에 툴셋 52개를 전부 `describe_toolset`으로 조회해 합산했다. ADR-0003이 「수백
개」로만 적고 있던 자리에 실측치를 넣었다.

| 구분 | 개수 |
| --- | --- |
| 최상위 MCP 도구 | 3 |
| 툴셋 소속 도구 | 830 |
| 합계 | 833 |

가장 큰 툴셋은 `SequencerTools` 140개다. 시퀀서 계열 여섯 툴셋만 합쳐도 275개인데 이
프로젝트는 하나도 쓰지 않는다. 명단을 좁게 잡는 비용이 크지 않은 이유가 여기서 드러난다.

이 숫자가 기존 문서와 어긋나지 않는지도 함께 확인했다. `BlueprintTools`가 53개이고 22개를
열었으니 뺀 것이 31개인데, ADR-0003이 「쓰기 31종」으로 적어 둔 값과 일치한다. `AssetTools`
21개 중 14개, `SlateInspectorToolset` 14개 중 6개, `EditorAppToolset` 21개 중 15개,
`ObjectTools` 6개 중 4개도 명단과 맞는다.

---

## 6. 검증

### 돌린 것

**훅**

```
py -3 .claude/hooks/test_guard_dangerous_cmd.py
```

- 명단 확장 직후: 73/73 통과, 종료 코드 0 (신규 15건)
- 감사 로그 추가 직후: **78/78 통과, 종료 코드 0** (신규 5건). 이것이 마지막으로 돌린 결과다

**언리얼 MCP 실경로** (에디터가 떠 있는 상태에서)

| 호출 | 결과 |
| --- | --- |
| `describe_toolset` (전체 경로 이름) | 툴셋 정보 반환 |
| `ObjectTools.list_properties` (`BP_MonsterBase`) | 속성 115개. 직전 세션의 `curl` 측정값과 일치 |
| `ObjectTools.set_properties` | **훅이 차단.** ADR-0003을 인용한 메시지 반환 |
| `ObjectTools.search_subclasses` | 파생 클래스 13개 |
| `ObjectTools.get_properties` (CDO 경로) | 값 반환. 파생별로 다른 값이 구분됨 |
| `AutomationTestToolset.DiscoverTests` | `{"status": "ready"}` |
| `AutomationTestToolset.ListTests` (필터 없음) | 8,954개 |
| `AutomationTestToolset.ListTests` (`P1`) | 0개 |
| `AutomationTestToolset.RunTests` | `passed: 1`, 0.0075초 |
| `LogsToolset.GetLogEntries` | 로그 5줄 |
| `AssetTools.get_referencers` | `BP_MonsterBase` 참조 12건 |

**도구 총 개수 집계**

서버에 JSON-RPC로 직접 붙어 툴셋 52개를 전부 `describe_toolset`으로 조회하고 합산했다.
결과는 툴셋 소속 830개, 최상위 3개, 합계 833개다. 스크립트는
`scratchpad/count-ue-tools.ps1`에 있고 저장소에는 넣지 않았다.

**헤드리스 테스트 경로**

| 경우 | 스크립트 종료 코드 |
| --- | --- |
| 엔진 테스트 1개 통과 | `0` |
| 필터가 아무것도 못 맞춤 | `1` |
| 프로젝트 테스트 빨강 | `1`, `failed: 1`, 단언 메시지 출력 |
| 프로젝트 테스트 초록 | `0`, `성공 1` |

벽시계 시간은 14.5초와 47.3초, 17.1초, 18.1초로 측정했다. 편차의 원인은 확인하지 못했다.

**빌드**

```
Build.bat P1Editor Win64 Development -Project=... -WaitMutex
```

- 빨강 테스트 포함: 종료 코드 `0`, 33.8초
- 공개 오버로드 오류: 종료 코드 `6`, `error C2248: private 멤버`
- 초록 테스트 포함: 종료 코드 `0`

에디터를 닫은 상태에서 돌렸다.

**스냅샷 스크립트**

가짜 트리 10개로 전 경로를 돌렸다.

| 경우 | 결과 |
| --- | --- |
| 최초 스냅샷 | 성공, robocopy 코드 1 |
| 변경 없이 재실행 | 성공, 코드 0 (증분 동작) |
| 원본이 절반으로 줄어든 상태 | **거부**, 종료 코드 1 |
| `-Restore` | 10개 전부 복구 |
| 스냅샷 경로가 저장소 안 | 거부 |
| 스냅샷 경로가 드라이브 루트 | 거부 |
| 스냅샷 경로가 원본과 같음 | 거부 |

실제 스냅샷도 한 번 떴다. **3,442개 / 7.6 GB, 44.2초.**

**인증 서버와 CI**

| 항목 | 결과 |
| --- | --- |
| `npm test` (로컬) | 통과 2건, 종료 코드 0 |
| `npm run lint` (로컬) | 종료 코드 0 |
| `ci.yml` 구문·구조 | `js-yaml`로 파싱해 확인 |
| 워크플로가 쓰는 `.env` 값 9개 | 환경 변수로 넣고 `npm test` 통과 |

### 돌리지 않은 것

- **CI 워크플로가 러너에서 실제로 도는 것.** 푸시해야 알 수 있다. `npm ci`가 `bcrypt`와
  `msnodesqlv8` 네이티브 모듈을 러너에서 설치하는지가 가장 불확실하다.
- **레벨을 열거나 PIE를 띄우는 테스트.** 헤드리스로 돌린 것은 순수 수치 연산 테스트와 이번에
  쓴 패킷 프레이밍 테스트뿐이다.
- **`-nullrhi`가 어떤 테스트를 걸러내는지.** 재지 않았다.
- **새로 연 도구 중 세 툴셋.** `EditorAppToolset`(`StartPIE` 포함), `SlateInspectorToolset`,
  `AgentSkillToolset`은 허용 명단에 넣었을 뿐 실제로 불러 보지 않았다. 훅이 통과시키는 것만
  단위 테스트로 확인했다.
- **`Backup-UeContent.ps1 -Restore`를 실제 7.6 GB 스냅샷에 대해.** 가짜 트리로만 확인했다.
- **다른 머신에서 MCP 서버가 자동으로 뜨는지.** 자동 시작 설정이 저장소에 없다.
- **`DummyClient` 스모크.** 이 세션은 서버 프로토콜이나 핸들러를 건드리지 않아서 돌리지
  않았다.

### 사람이 직접 한 것

- **허용 명단 블록을 파일에 붙여넣은 것.** auto mode classifier가 에이전트 편집을 막아서,
  블록을 파일로 만들어 전달하고 사람이 붙여넣은 뒤 훅 테스트를 돌려 58/58 통과를 확인했다.
- **에디터를 닫은 것.** 빌드 전에 사람이 닫았다.

---

## 7. 위험과 열린 질문

### 머지 전에 정해야 하는 것

- **언리얼 MCP를 도입할 것인가.** ADR-0003이 `proposed`다. 이 브랜치는 판단 재료를 모은
  결과물이고, 도입을 확정하는 브랜치가 아니다.
- **Epic의 스킬 플러그인(`EpicGames/unreal-engine-skills-for-claude-code-plugin`)을 쓸
  것인가.** 이 세션에서는 README만 읽고 설치하지 않았다. 그 플러그인은 광범위한 라이브
  접근을 전제로 쓰였고 우리 허용 명단과 충돌한다.
- **CI의 남은 두 티어를 어떻게 할 것인가.** self-hosted 러너를 쓸지, 게임 서버만 `v143`으로
  내릴지다. 공개 저장소라 self-hosted 쪽은 트리거 범위를 먼저 정해야 한다.

### 검증되지 않은 가정

- **CI 워크플로가 러너에서 돈다는 가정.** 로컬 명령과 YAML 구문만 확인했다.
- **`StartPIE`를 열어 두면 3티어 통합 스모크가 된다는 가정.** 근거는 도구 스키마를 읽은
  것까지다. 실제로 PIE를 띄워 서버에 붙여 보지 않았다.
- **CI에서 인증 서버 테스트가 의미를 갖는다는 가정.** `.env`를 워크플로가 만들기 때문에
  「필수 키 존재」 검사는 CI에서 항상 통과한다. 그 검사는 새로 클론한 사람을 잡아 주는 것이
  목적이라 로컬에서만 의미가 있다. CI에서 남는 값은 `parseInt` 파싱 쪽이다. 이 사정을
  워크플로 주석에 적어 두었다.

### 깨지기 쉬운 지점

- **허용 명단이 단일 실패 지점이다.** `permissions`가 이 서버에는 작동하지 않으므로 훅 파일
  하나가 756개 닫힌 도구 앞의 유일한 문이다. `.claude/settings.json`의 matcher가 빠지면
  통제가 조용히 사라진다.
- **엔진 버전에 묶인다.** 구현이 `Engine/Plugins/Experimental/`에 있다. 5.9에서 API가 바뀌면
  허용 명단의 도구 이름이 그대로 깨진다.
- **스냅샷이 한 벌이다.** 줄어듦 방어가 있지만 파일 수가 크게 줄지 않는 형태의 손상(내용만
  덮어쓰는 경우)은 잡지 못한다.
- **테스트가 1개다.** 그물이라기보다 경로가 열렸다는 증거에 가깝다.

### 의도적으로 미룬 것

- `SendBuffer`의 죽은 코드와 경계 결함은 고치지 않고 `docs/tech-debt.md`에 기록만 했다.
- ADR-0003과 훅 주석이 `docs/backlog.md`의 근거를 「클라이언트 L1 항목」으로 가리키는데,
  실제 항목은 「5. UE L2 Automation Test」다. 가리키는 곳이 어긋나 보이지만 계획 밖이라
  손대지 않았다.

---

## 8. PR 리뷰 가이드

### 먼저 볼 곳

**`P1/Scripts/Run-UeTests.ps1`의 판정 부분**

이 스크립트가 존재하는 이유 전체가 여기에 있다. 두 가지를 확인한다.

- `index.json`을 실행 **전에** 지우는 줄. 이게 없으면 지난 실행의 초록 리포트를 이번 결과로
  읽는다. 가짜 트리가 아니라 실제로 이 상황을 만들어 거부되는 것을 확인했다.
- 네 조건을 전부 만족해야 `0`으로 끝나는 부분. 특히 `succeeded > 0` 조건이 「매칭 0건」을
  잡는다.

**`.claude/hooks/guard_dangerous_cmd.py`의 `UE_ALLOWED_TOOLS`**

명단이 35개에서 76개로 늘었다. 무엇을 열고 무엇을 닫았는지가 그 위 주석에 적혀 있다. 특히
확인할 것은 조회가 아닌 셋(`SetVerbosity`, `StartPIE`, `StopPIE`)이 들어간 것에 동의하는지다.
`StartPIE`는 이 세션에서 실제로 불러 보지 않았다.

**`.claude/hooks/guard_dangerous_cmd.py`의 `_record_ue_call`**

기록 실패가 판정을 뒤집지 않는 구조인지 본다. `except Exception: pass`로 삼키는데, 이것은
기존 `_record_block`과 같은 원칙이고 테스트로도 고정했다(「감사 실패해도 통과」).

**`P1/Scripts/Backup-UeContent.ps1`의 경로 검사**

`robocopy /MIR`은 대상 폴더를 원본에 맞춰 지운다. 대상을 잘못 주면 그게 사고다. 저장소 안,
드라이브 루트, 원본과 같은 경로를 각각 거부하는지 확인한다. 셋 다 실행해서 거부되는 것을
봤지만, 놓친 경우가 있는지는 다른 눈이 필요하다.

**`P1/Source/P1/Tests/PacketFramingTest.cpp`의 UTF-8 바이트 리터럴**

소스 파일 인코딩에 기대지 않으려고 `"\xEC\x95\x88..."` 형태로 직접 적었다. 이 저장소는
`.proto`와 `.bat`만 cp949이고 나머지는 UTF-8이라, 한글 리터럴을 그냥 쓰면 컴파일러 설정에
따라 달라질 수 있다고 판단했다. 이 판단에 확신이 낮다.

**`.github/workflows/ci.yml`의 `.env` 생성 단계**

CI에서 「필수 키 존재」 검사가 항상 통과하게 되는 구조다. 주석에 그 사정을 적었지만, 이
절충이 받아들일 만한지는 사람이 정할 문제다.

### 확신이 낮은 판단

- **테스트 대상 선택.** `MakeSerializedPacket`을 seam으로 삼은 것은 사람 승인을 받았지만,
  첫 테스트로 이것이 맞는지는 다른 의견이 있을 수 있다. `docs/testing.md`의 seam 표에 행을
  추가했다.
- **`StartPIE`를 지금 여는 것.** 통합 스모크를 위한 선제 조치인데, 그 스모크를 아직 쓰지
  않았다. 쓸 때까지 닫아 두자는 의견도 타당하다.

---

## 9. 바뀐 파일과 영역

### 새로 만든 것

```
P1/Scripts/Run-UeTests.ps1
  UE 자동화 테스트를 에디터 없이 돌리고 index.json 으로 판정한다.

P1/Scripts/Backup-UeContent.ps1
  P1/Content 를 저장소 밖에 미러로 뜬다. 줄어듦 방어와 -Restore 를 포함한다.

P1/Source/P1/Tests/PacketFramingTest.cpp
  UE 클라이언트의 첫 자동화 테스트. 패킷 헤더 배치와 본문 왕복을 고정한다.

.github/workflows/ci.yml
  인증 서버 테스트와 정적 검사. 나머지 두 티어가 왜 없는지도 주석에 적혀 있다.
```

### 고친 것 (통제)

```
.claude/hooks/guard_dangerous_cmd.py
  허용 명단 35 → 76 조합. 통과 호출 감사 로그 추가.

.claude/hooks/test_guard_dangerous_cmd.py
  58 → 78건. 새 툴셋 15건과 감사 로그 5건.

.gitignore
  ue_audit.log 를 추적에서 뺀다.
```

### 고친 것 (문서)

```
docs/adr/0003-gate-unreal-mcp-by-hook-whitelist.md
  status proposed. 실측 결과 절, 열지 않은 것 절, 통제 계층 셋.

CLAUDE.md
  도구 라우팅과 완료 기준. UE 테스트의 종료 코드 예외가 여기 들어갔다.

docs/testing.md
  UE 클라 L2 실행 경로, seam 표, 커버리지, TDD 범위.

docs/backlog.md
  4번 CI 의 막는 조건, 5번 UE L2 의 선행 조건 제거.

docs/tech-debt.md
  SendBuffer 항목 추가 (16 → 17개).
```

### 이 세션이 만들지 않은 것

```
.mcp.json
.claude/settings.json
docs/adr/0003-...md 의 최초 작성분
  전부 fd17d25 (직전 세션)이다. 브랜치 diff 에는 포함되지만 이 세션의 산출물이 아니다.
```

---

## 10. 머지 체크리스트

- [ ] **언리얼 MCP 도입 여부를 정한다.** ADR-0003이 `proposed`다. `accepted`로 올릴지,
      `.mcp.json`과 훅 matcher를 되돌릴지 정한다.
- [ ] **허용 명단 76개 조합에 동의하는지 확인한다.** 특히 `StartPIE`와 `StopPIE`,
      `SetVerbosity`가 조회가 아닌데 들어간 것에 대해.
- [ ] **`Run-UeTests.ps1`이 `index.json`을 실행 전에 지우는 것을 확인한다.** 이 줄이 빠지면
      지난 실행 결과가 초록으로 읽힌다.
- [ ] **CI 워크플로를 실제로 돌려 본다.** 푸시 전까지는 검증되지 않았다. `npm ci`의 네이티브
      모듈 설치가 가장 불확실하다.
- [ ] **CI에서 인증 서버 테스트가 갖는 의미를 받아들일지 정한다.** `.env`를 워크플로가
      만들기 때문에 「필수 키 존재」 검사가 항상 통과한다.
- [ ] **`P1/Content` 스냅샷 방침을 정한다.** LFS 전환(`docs/backlog.md` 7번)까지 이 스크립트를
      쓸지, 전환을 먼저 할지.
- [ ] **CI의 남은 두 티어 방향을 정한다.** self-hosted 러너를 쓸지, 게임 서버만 `v143`으로
      내릴지. 공개 저장소라는 점을 함께 본다.
- [ ] **Epic 스킬 플러그인을 별도로 판단한다.** 이 브랜치의 대상이 아니다.
