#Requires -Version 7.0
<#
.SYNOPSIS
    게임 맵의 레벨과 크리처의 에셋에서 서버가 읽는 추출 데이터를 쓴다. 에디터를 띄우지 않는다.

.DESCRIPTION
    UnrealEditor-Cmd 로 두 커맨드렛을 차례로 돌린다.

    1. 배치 추출(P1PlacementExtract): 프로젝트 설정의 맵 표(MapLevels)에 있는 레벨을 모두 열고, 마커와 기획 데이터를
       대조해 문제가 없을 때만 Server/GameServer/Game/Data/Extracted/ 에 맵마다 Placement_<맵 번호>.json 을 쓴다.
    2. 크리처 추출(P1CreatureCombatExtract): 몬스터 에셋 테이블과 직업마다의 플레이어 블루프린트(CharacterClassPlayers)에서
       캡슐 크기와 공격 몽타주의 타이밍, 루트 모션을 읽어, 문제가 없을 때만 같은 폴더에 CreatureCombat.json 을 쓴다.

    판정은 커맨드렛이 남긴 결과 줄이다. 문제가 있으면 커맨드렛이 오류를 한 줄씩 출력하고 산출물을 바꾸지 않으며,
    이 스크립트는 1 로 끝난다. 커맨드렛이 실행되지 못하면(엔진이 커맨드렛을 찾지 못함) 결과 줄이 없으므로 역시 1 이다.
    엔진의 종료 코드는 판정에 쓰지 않는다. 에디터가 떠 있으면 커맨드렛 프로세스의 MCP 서버가 같은 포트를 잡지 못해
    오류 로그를 남기고, UnrealEditor-Cmd 는 추출과 무관한 그 오류 때문에도 1 을 돌려준다(2026년 10월 29일 실측).
    앞 단계가 실패해도 다음 단계를 돌려 오류를 한 번에 보여 준다. 각 단계의 산출물은 그 단계가 통과했을 때만 바뀐다.

    레벨이나 공격 몽타주, 몬스터 에셋 테이블을 고쳤으면 이 스크립트를 돌리고 산출물을 에셋과 같은 커밋에 담는다.
    클라이언트를 빌드한 뒤에 돌린다. 내비게이션 티켓이 navmesh 빌드를 이 스크립트에 잇는다.

.EXAMPLE
    pwsh P1/Scripts/Invoke-PlacementExtract.ps1
#>
[CmdletBinding()]
param(
    # 엔진 설치 경로는 머신마다 다르다. 환경 변수로 덮을 수 있다.
    [string]$EngineRoot = $(if ($env:UE_ENGINE_ROOT) { $env:UE_ENGINE_ROOT } else { 'D:\Unreal\Editor\Launcher\UE_5.8' }),

    [string]$ProjectPath = (Join-Path $PSScriptRoot '..' 'P1.uproject')
)

$ErrorActionPreference = 'Stop'

$editorCmd = Join-Path $EngineRoot 'Engine' 'Binaries' 'Win64' 'UnrealEditor-Cmd.exe'
if (-not (Test-Path $editorCmd)) {
    Write-Host "[실패] UnrealEditor-Cmd.exe 가 없다: $editorCmd"
    Write-Host "       -EngineRoot 로 엔진 경로를 넘기거나 UE_ENGINE_ROOT 를 설정한다."
    exit 1
}
if (-not (Test-Path $ProjectPath)) {
    Write-Host "[실패] P1.uproject 가 없다: $ProjectPath"
    exit 1
}
$ProjectPath = (Resolve-Path $ProjectPath).Path

$logDir = Join-Path $PSScriptRoot '..' 'Saved' 'PlacementExtract'
New-Item -ItemType Directory -Force $logDir | Out-Null
$logDir = (Resolve-Path $logDir).Path

# 엔진은 표준 출력에 UTF-8을 쓴다. 콘솔 코드 페이지(cp949)로 읽으면 한국어 오류 줄이 깨지고 아래 검색에 걸리지 않는다.
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

# 단계마다 커맨드렛 이름, 로그 머리말, 끝까지 돌았을 때 남기는 결과 줄.
$steps = @(
    @{ Commandlet = 'P1PlacementExtract'; Tag = '배치 추출'; Finished = '맵 \d+개의 배치 데이터를 썼다'; Log = 'run.log' }
    @{ Commandlet = 'P1CreatureCombatExtract'; Tag = '크리처 추출'; Finished = '몬스터 \d+개와 직업 \d+개의 판정 데이터를 썼다'; Log = 'creature.log' }
)

$allPassed = $true
foreach ($step in $steps) {
    $logPath = Join-Path $logDir $step.Log
    $runArgs = @(
        $ProjectPath
        "-run=$($step.Commandlet)"
        '-unattended'
        '-nopause'
        '-nosplash'
        '-nullrhi'
        '-stdout'
        '-FullStdOutLogOutput'
    )

    $started = Get-Date
    & $editorCmd @runArgs *> $logPath
    $exitCode = $LASTEXITCODE
    $elapsed = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)

    # 커맨드렛이 남긴 줄만 보여 준다. 전체 로그는 단계마다의 로그 파일에 있다.
    $tag = [regex]::Escape("[$($step.Tag)]")
    Select-String -Path $logPath -Pattern $tag | ForEach-Object { $_.Line }
    Write-Host "엔진 종료 코드: $exitCode (판정에 쓰지 않는다) · 소요: ${elapsed}초 · 로그: $logPath"

    $finished = Select-String -Path $logPath -Pattern "$tag $($step.Finished)" -Quiet
    $failed = Select-String -Path $logPath -Pattern "$tag 오류 \d+건" -Quiet
    if ($failed -or -not $finished) {
        Write-Host "[실패] $($step.Tag)이 끝나지 않았다. 이 단계의 산출물을 바꾸지 않았다."
        $allPassed = $false
    }
}

if (-not $allPassed) {
    exit 1
}

Write-Host "[통과] 배치 데이터와 크리처 판정 데이터를 썼다. 산출물을 에셋과 같은 커밋에 담는다."
exit 0
