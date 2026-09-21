#Requires -Version 7.0
<#
.SYNOPSIS
    클라이언트를 빌드한다. 에디터가 떠 있으면 막고, 명시했을 때만 닫는다.

.DESCRIPTION
    에디터가 떠 있으면 UBT 가 UnrealEditor-P1.dll 을 덮어쓰지 못해 빌드가 실패한다.
    그 조건을 사람의 기억에 맡기지 않고 빌드 전에 확인한다.

    기본 동작은 "떠 있으면 무엇이 떠 있는지 알리고 1 로 끝낸다" 이다. 에디터에
    저장하지 않은 에셋 변경이 있으면 강제 종료가 그것을 없애고, 이 저장소는 에셋을
    Git LFS 로 추적하므로 그 손실이 실재한다. -CloseEditor 를 명시한 호출에서만
    닫는다.

    닫는 절차는 두 단계다. 먼저 CloseMainWindow() 로 정상 종료를 요청하고,
    -CloseTimeoutSeconds 안에 응답하지 않을 때만 Stop-Process -Force 로 넘어간다.
    저장 여부를 묻는 대화상자가 떠서 종료가 멈춘 것을 강제 종료로 덮어 버리지
    않도록, 강제 종료로 넘어간 사실을 출력에 남긴다.

    빌드 출력은 파일로 보낸다. 전체 빌드는 출력이 커서 호출한 쪽에서 절단될 수
    있기 때문이다. 실패하면 그 파일과 UBT 로그에서 오류 줄만 추려서 보여준다.

.PARAMETER Target
    빌드 타깃이다. 에디터 모듈은 P1Editor 다.

.PARAMETER Platform
    빌드 플랫폼이다.

.PARAMETER Configuration
    빌드 구성이다.

.PARAMETER EngineRoot
    엔진 설치 경로다. 머신마다 다르므로 UE_ENGINE_ROOT 로 덮을 수 있다.

.PARAMETER CloseEditor
    떠 있는 에디터를 닫고 빌드한다. 이 스위치가 없으면 닫지 않고 1 로 끝낸다.

.PARAMETER Relaunch
    빌드가 끝난 뒤 에디터를 다시 띄운다. 빌드 성공 여부와 무관하게 띄운다.

.PARAMETER CloseTimeoutSeconds
    정상 종료를 기다리는 시간이다. 넘기면 강제 종료로 넘어간다.

.EXAMPLE
    pwsh P1/Scripts/Invoke-UeBuild.ps1
    에디터가 떠 있으면 무엇이 떠 있는지 알리고 1 로 끝낸다.

.EXAMPLE
    pwsh P1/Scripts/Invoke-UeBuild.ps1 -CloseEditor -Relaunch
    에디터를 닫고 빌드한 뒤 다시 띄운다.
#>
[CmdletBinding()]
param(
    [string]$Target = 'P1Editor',

    [string]$Platform = 'Win64',

    [string]$Configuration = 'Development',

    # 엔진 설치 경로는 머신마다 다르다. 환경 변수로 덮을 수 있다.
    [string]$EngineRoot = $(if ($env:UE_ENGINE_ROOT) { $env:UE_ENGINE_ROOT } else { 'D:\Unreal\Editor\Launcher\UE_5.8' }),

    [string]$ProjectPath = (Join-Path $PSScriptRoot '..' 'P1.uproject'),

    # Saved/ 는 .gitignore 대상이다.
    [string]$LogDir = (Join-Path $PSScriptRoot '..' 'Saved' 'UeBuild'),

    # 에디터가 떠 있으면 닫는다. 이 스위치가 없으면 닫지 않고 실패로 끝낸다.
    [switch]$CloseEditor,

    # 빌드가 끝난 뒤 에디터를 다시 띄운다.
    [switch]$Relaunch,

    [int]$CloseTimeoutSeconds = 30
)

$ErrorActionPreference = 'Stop'

function Write-Fail([string]$Message) {
    Write-Host "[실패] $Message"
}

# UBT 와 빌드 로그에서 오류로 읽을 줄의 패턴이다. Unable to delete 는 에디터가
# 떠 있을 때 나타나는 잠금 실패의 문구다. 스위치 없이 빌드해서 실패했을 때
# 원인을 바로 알아보게 한다.
$ErrorPattern = 'error [A-Z]+\d+|: error|Unable to delete'

function Show-ErrorLines([string]$Path, [string]$Label) {
    if (-not (Test-Path $Path)) { return }
    $lines = Select-String -Path $Path -Pattern $ErrorPattern | Select-Object -Last 20
    if ($lines.Count -eq 0) { return }
    Write-Host "  --- $Label ---"
    foreach ($line in $lines) {
        Write-Host "  $($line.Line.Trim())"
    }
}

# --- 선행 조건 ---------------------------------------------------------------

$buildBat = Join-Path $EngineRoot 'Engine' 'Build' 'BatchFiles' 'Build.bat'
if (-not (Test-Path $buildBat)) {
    Write-Fail "Build.bat 이 없다: $buildBat"
    Write-Host "       -EngineRoot 로 엔진 경로를 넘기거나 UE_ENGINE_ROOT 를 설정한다."
    exit 1
}

if (-not (Test-Path $ProjectPath)) {
    Write-Fail "P1.uproject 가 없다: $ProjectPath"
    exit 1
}

$ProjectPath = (Resolve-Path $ProjectPath).Path
New-Item -ItemType Directory -Force $LogDir | Out-Null
$LogDir = (Resolve-Path $LogDir).Path
$buildLog = Join-Path $LogDir 'build.log'

$editorExe = Join-Path $EngineRoot 'Engine' 'Binaries' 'Win64' 'UnrealEditor.exe'

# --- 에디터 확인 -------------------------------------------------------------

# 이 프로젝트를 연 에디터만 찾는다. 다른 UE 프로젝트의 에디터는 이 DLL 을 잠그지
# 않는다. UnrealEditor-Cmd 도 포함한다. 자동화 테스트가 남긴 인스턴스가 같은
# 모듈을 잠그기 때문이다.
$projectLeaf = [IO.Path]::GetFileName($ProjectPath)
$running = @(
    Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%.exe'" |
        Where-Object { $_.CommandLine -and $_.CommandLine -match [regex]::Escape($projectLeaf) }
)

if ($running.Count -gt 0) {
    Write-Host "이 프로젝트를 연 에디터 $($running.Count)개가 떠 있다."
    foreach ($proc in $running) {
        Write-Host "  PID $($proc.ProcessId)  $($proc.Name)"
    }

    if (-not $CloseEditor) {
        Write-Fail "에디터가 떠 있어 빌드하지 않는다."
        Write-Host "       UnrealEditor-P1.dll 을 덮어쓸 수 없어 어차피 실패한다."
        Write-Host "       저장하지 않은 에셋 변경이 없다면 -CloseEditor 를 붙여 다시 부른다."
        exit 1
    }

    foreach ($proc in $running) {
        $handle = Get-Process -Id $proc.ProcessId -ErrorAction SilentlyContinue
        if (-not $handle) { continue }

        $closed = $false
        if ($handle.MainWindowHandle -ne 0) {
            [void]$handle.CloseMainWindow()
            $closed = $handle.WaitForExit($CloseTimeoutSeconds * 1000)
        }

        if (-not $closed) {
            # 창이 없거나(UnrealEditor-Cmd), 저장 여부를 묻는 대화상자에서 멈춘
            # 경우다. 후자라면 저장하지 않은 변경이 여기서 사라진다.
            Write-Host "  PID $($proc.ProcessId) 가 ${CloseTimeoutSeconds}초 안에 닫히지 않아 강제 종료한다."
            Stop-Process -Id $proc.ProcessId -Force -ErrorAction SilentlyContinue
            $handle.WaitForExit(10 * 1000) | Out-Null
        }
        else {
            Write-Host "  PID $($proc.ProcessId) 를 정상 종료했다."
        }
    }
}
else {
    Write-Host "떠 있는 에디터가 없다."
}

# --- 빌드 -------------------------------------------------------------------

Write-Host "빌드: $Target $Platform $Configuration"
Write-Host "로그: $buildLog"

$started = Get-Date
& $buildBat $Target $Platform $Configuration "-Project=$ProjectPath" -WaitMutex *> $buildLog
$buildExit = $LASTEXITCODE
$elapsed = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)

Write-Host "소요: ${elapsed}초"

# --- 재시작 -----------------------------------------------------------------

# 빌드 성패와 무관하게 띄운다. 실패했다고 에디터를 닫아 둔 채로 두면 부른 쪽이
# 스위치를 준 의도와 어긋난다.
if ($Relaunch) {
    if (Test-Path $editorExe) {
        Write-Host "에디터를 다시 띄운다."
        Start-Process -FilePath $editorExe -ArgumentList $ProjectPath | Out-Null
    }
    else {
        Write-Host "[경고] UnrealEditor.exe 가 없어 다시 띄우지 못했다: $editorExe"
    }
}

# --- 판정 -------------------------------------------------------------------

if ($buildExit -ne 0) {
    Write-Fail "빌드가 종료 코드 $buildExit 로 끝났다."
    Show-ErrorLines $buildLog '빌드 출력'
    Show-ErrorLines (Join-Path $env:LOCALAPPDATA 'UnrealBuildTool' 'Log.txt') 'UBT 로그'
    Write-Host "       전체 출력: $buildLog"
    exit 1
}

Write-Host "[통과] 빌드가 성공했다."
exit 0
