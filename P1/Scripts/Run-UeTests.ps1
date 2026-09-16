#Requires -Version 7.0
<#
.SYNOPSIS
    UE 자동화 테스트를 에디터 없이 돌리고 리포트로 판정한다.

.DESCRIPTION
    UnrealEditor-Cmd 는 매칭된 테스트가 하나도 없어도 종료 코드 0 을 돌려준다
    (2026년 9월 16일 실측). 그래서 종료 코드만으로는 판정할 수 없고, 필터 오타가
    초록으로 보인다. 이 스크립트가 -ReportExportPath 의 index.json 을 읽어
    판정하고 스스로 0 또는 1 로 끝난다.

    판정 조건은 네 가지다. 하나라도 어긋나면 1 로 끝낸다.
      1. index.json 이 존재한다
      2. succeeded 가 1 이상이다
      3. failed 가 0 이다
      4. notRun 과 inProcess 가 0 이다

    에디터가 떠 있어도 같이 돌 수 있다. 헤드리스 인스턴스는 MCP 서버 포트를
    빼앗지 못하고 그냥 넘어간다.

.PARAMETER Filter
    Automation RunTests 에 넘길 필터다. 전체 테스트 경로의 부분 문자열로 맞춘다.

.PARAMETER WithRhi
    -nullrhi 를 빼고 돌린다. 렌더링이 필요한 테스트에 쓴다.

.EXAMPLE
    pwsh P1/Scripts/Run-UeTests.ps1
    프로젝트 테스트(P1)를 전부 돌린다.

.EXAMPLE
    pwsh P1/Scripts/Run-UeTests.ps1 -Filter System.Core.Math.Bitcast
    엔진 테스트 한 건으로 실행 경로만 확인한다.
#>
[CmdletBinding()]
param(
    [string]$Filter = 'P1',

    # 엔진 설치 경로는 머신마다 다르다. 환경 변수로 덮을 수 있다.
    [string]$EngineRoot = $(if ($env:UE_ENGINE_ROOT) { $env:UE_ENGINE_ROOT } else { 'D:\Unreal\Editor\Launcher\UE_5.8' }),

    [string]$ProjectPath = (Join-Path $PSScriptRoot '..' 'P1.uproject'),

    # Saved/ 는 .gitignore 대상이다. 자동화가 비우는 Saved/Automation/ 바깥에 둔다.
    [string]$ReportPath = (Join-Path $PSScriptRoot '..' 'Saved' 'UeTestReport'),

    [switch]$WithRhi
)

$ErrorActionPreference = 'Stop'

function Write-Fail([string]$Message) {
    Write-Host "[실패] $Message"
}

# --- 선행 조건 ---------------------------------------------------------------

$editorCmd = Join-Path $EngineRoot 'Engine' 'Binaries' 'Win64' 'UnrealEditor-Cmd.exe'
if (-not (Test-Path $editorCmd)) {
    Write-Fail "UnrealEditor-Cmd.exe 가 없다: $editorCmd"
    Write-Host "       -EngineRoot 로 엔진 경로를 넘기거나 UE_ENGINE_ROOT 를 설정한다."
    exit 1
}

if (-not (Test-Path $ProjectPath)) {
    Write-Fail "P1.uproject 가 없다: $ProjectPath"
    exit 1
}

$ProjectPath = (Resolve-Path $ProjectPath).Path
New-Item -ItemType Directory -Force $ReportPath | Out-Null
$ReportPath = (Resolve-Path $ReportPath).Path

$indexJson = Join-Path $ReportPath 'index.json'
$logPath = Join-Path $ReportPath 'run.log'

# 지난 실행의 리포트로 판정하지 않도록 판정 대상 파일을 먼저 지운다.
# 테스트가 하나도 안 돌면 UE 가 index.json 을 아예 만들지 않기 때문에,
# 남아 있던 파일이 그대로 초록으로 읽힌다.
if (Test-Path $indexJson) { Remove-Item -Force $indexJson }

# --- 실행 -------------------------------------------------------------------

$runArgs = @(
    $ProjectPath
    "-ExecCmds=Automation RunTests $Filter"
    '-unattended'
    '-nopause'
    '-nosplash'
    '-testexit=Automation Test Queue Empty'
    '-log'
    '-stdout'
    "-ReportExportPath=$ReportPath"
)
if (-not $WithRhi) { $runArgs += '-nullrhi' }

Write-Host "필터: $Filter"
Write-Host "리포트: $ReportPath"

$started = Get-Date
& $editorCmd @runArgs *> $logPath
$editorExit = $LASTEXITCODE
$elapsed = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)

# --- 판정 -------------------------------------------------------------------

# 에디터의 종료 코드는 기록만 한다. 판정에 쓰지 않는다.
Write-Host "에디터 종료 코드: $editorExit (판정에 쓰지 않는다)"
Write-Host "소요: ${elapsed}초"

if (-not (Test-Path $indexJson)) {
    Write-Fail "테스트가 하나도 돌지 않았다. index.json 이 없다."
    Write-Host "       필터 '$Filter' 가 아무것도 맞추지 못했을 가능성이 높다."
    Write-Host "       로그: $logPath"
    exit 1
}

# index.json 은 BOM 이 붙어서 나온다. ConvertFrom-Json 앞에서 떼어 낸다.
$raw = (Get-Content $indexJson -Raw) -replace '^\uFEFF', ''
$report = $raw | ConvertFrom-Json

$succeeded = [int]$report.succeeded
$failed = [int]$report.failed
$notRun = [int]$report.notRun
$inProcess = [int]$report.inProcess
$withWarnings = [int]$report.succeededWithWarnings

Write-Host "결과: 성공 $succeeded · 실패 $failed · 미실행 $notRun · 진행중 $inProcess · 경고동반성공 $withWarnings"

if ($failed -gt 0) {
    foreach ($test in $report.tests) {
        if ($test.state -ne 'Success') {
            Write-Host "  [$($test.state)] $($test.fullTestPath)"
            foreach ($entry in $test.entries) {
                if ($entry.event.type -eq 'Error') {
                    Write-Host "      $($entry.event.message)"
                }
            }
        }
    }
}

if ($succeeded -lt 1) {
    Write-Fail "성공한 테스트가 없다."
    Write-Host "       로그: $logPath"
    exit 1
}

if ($failed -gt 0 -or $notRun -gt 0 -or $inProcess -gt 0) {
    Write-Fail "실패·미실행·미완료 테스트가 있다."
    Write-Host "       로그: $logPath"
    Write-Host "       리포트: $(Join-Path $ReportPath 'index.html')"
    exit 1
}

Write-Host "[통과] 테스트 $succeeded 건이 전부 성공했다."
exit 0
