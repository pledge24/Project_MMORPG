#Requires -Version 7.0
<#
.SYNOPSIS
    P1/Content 를 저장소 밖에 미러로 떠 둔다. 되돌릴 수단이 없는 구간의 임시 그물이다.

.DESCRIPTION
    P1/.gitignore 의 Content/* 규칙 때문에 디스크의 에셋 3,442개 중 git 이 추적하는 것은
    47개뿐이다(2026년 9월 17일 실측). 나머지는 지워지면 되돌릴 방법이 없다. 커밋이나 셸브를
    먼저 확인하라는 안전 규칙도 여기서는 작동하지 않는다. 추적되지 않는 파일은 커밋에 없기
    때문이다.

    docs/backlog.md 7번의 LFS 전환이 끝나면 이 스크립트는 버린다. git 이 같은 일을 더 잘한다.

    미러는 한 벌만 둔다. 그래서 가장 큰 위험은 사고가 난 뒤에 스냅샷을 덮어쓰는 것이다.
    원본이 스냅샷보다 크게 줄었으면 거부한다(-ShrinkGuardPercent).

.PARAMETER Restore
    스냅샷을 P1/Content 로 되돌린다. 확인을 묻는다.

.PARAMETER ShrinkGuardPercent
    원본 파일 수가 스냅샷보다 이 비율 넘게 줄었으면 스냅샷을 거부한다. 기본 10.

.PARAMETER Force
    줄어듦 검사를 건너뛴다. 의도적으로 에셋을 정리한 직후에만 쓴다.

.EXAMPLE
    pwsh P1/Scripts/Backup-UeContent.ps1
    스냅샷을 뜬다. 두 번째부터는 바뀐 것만 복사한다.

.EXAMPLE
    pwsh P1/Scripts/Backup-UeContent.ps1 -Restore
    스냅샷을 되돌린다.
#>
[CmdletBinding(SupportsShouldProcess, ConfirmImpact = 'High')]
param(
    [string]$ContentPath = (Join-Path $PSScriptRoot '..' 'Content'),

    [string]$SnapshotPath = $(if ($env:P1_CONTENT_SNAPSHOT) { $env:P1_CONTENT_SNAPSHOT } else { 'D:\Unreal\Snapshots\P1-Content' }),

    [switch]$Restore,

    [ValidateRange(0, 100)]
    [int]$ShrinkGuardPercent = 10,

    [switch]$Force
)

$ErrorActionPreference = 'Stop'

function Write-Fail([string]$Message) {
    Write-Host "[실패] $Message"
}

function Measure-Tree([string]$Path) {
    $files = Get-ChildItem -Recurse -File -LiteralPath $Path -ErrorAction SilentlyContinue
    $sum = ($files | Measure-Object -Property Length -Sum).Sum
    return [pscustomobject]@{
        Count = @($files).Count
        Bytes = [int64]($sum ?? 0)
    }
}

function Invoke-Mirror([string]$From, [string]$To) {
    # /MIR 는 대상에만 있는 파일을 지운다. 그래서 위에서 대상 경로를 먼저 검사한다.
    # /NFL /NDL 은 파일·폴더 목록을 끈다. 3,442개가 그대로 찍히면 출력이 쓸모없어진다.
    & robocopy $From $To /MIR /NFL /NDL /NJH /NJS /NP /R:1 /W:1 | Out-Null
    # robocopy 는 0~7 이 정상이고 8 이상이 실패다. 0 은 변경 없음, 1 은 복사함.
    return $LASTEXITCODE
}

# --- 경로 검사 ---------------------------------------------------------------
# /MIR 은 대상 폴더를 원본에 맞춰 지운다. 대상을 잘못 주면 그게 사고다.

if (-not (Test-Path $ContentPath)) {
    Write-Fail "P1/Content 가 없다: $ContentPath"
    exit 1
}
$ContentPath = (Resolve-Path $ContentPath).Path.TrimEnd('\')

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..' '..')).Path.TrimEnd('\')

$snapFull = [System.IO.Path]::GetFullPath($SnapshotPath).TrimEnd('\')
$manifestPath = "$snapFull.manifest.json"

if ($snapFull -eq $ContentPath) {
    Write-Fail "스냅샷 경로가 원본과 같다: $snapFull"
    exit 1
}
if ($snapFull.StartsWith($repoRoot, [StringComparison]::OrdinalIgnoreCase)) {
    Write-Fail "스냅샷 경로가 저장소 안이다: $snapFull"
    Write-Host "       저장소 밖에 둔다. -SnapshotPath 나 P1_CONTENT_SNAPSHOT 로 지정한다."
    exit 1
}
if ([string]::IsNullOrEmpty([System.IO.Path]::GetDirectoryName($snapFull))) {
    Write-Fail "스냅샷 경로가 드라이브 루트다: $snapFull"
    exit 1
}

# --- 되돌리기 ----------------------------------------------------------------

if ($Restore) {
    if (-not (Test-Path $snapFull)) {
        Write-Fail "스냅샷이 없다: $snapFull"
        exit 1
    }
    $snap = Measure-Tree $snapFull
    $cur = Measure-Tree $ContentPath

    Write-Host "스냅샷: $snapFull"
    if (Test-Path $manifestPath) {
        $m = Get-Content $manifestPath -Raw | ConvertFrom-Json
        Write-Host "  뜬 시각: $($m.takenAt)"
    }
    Write-Host ("  스냅샷 {0:N0}개 / 지금 Content {1:N0}개" -f $snap.Count, $cur.Count)

    $target = "$ContentPath (파일 {0:N0}개)" -f $cur.Count
    if (-not $PSCmdlet.ShouldProcess($target, "스냅샷으로 되돌린다. 스냅샷에 없는 파일은 지워진다")) {
        Write-Host "취소했다."
        exit 1
    }

    $rc = Invoke-Mirror $snapFull $ContentPath
    if ($rc -ge 8) {
        Write-Fail "robocopy 실패 (코드 $rc)"
        exit 1
    }
    $after = Measure-Tree $ContentPath
    Write-Host ("[완료] 되돌렸다. 파일 {0:N0}개" -f $after.Count)
    exit 0
}

# --- 스냅샷 ------------------------------------------------------------------

$cur = Measure-Tree $ContentPath
if ($cur.Count -eq 0) {
    Write-Fail "원본에 파일이 없다. 경로를 확인한다: $ContentPath"
    exit 1
}

# 사고가 난 뒤에 스냅샷을 덮어쓰면 그물이 사라진다. 원본이 크게 줄었으면 멈춘다.
if ((Test-Path $manifestPath) -and -not $Force) {
    $prev = Get-Content $manifestPath -Raw | ConvertFrom-Json
    $prevCount = [int]$prev.fileCount
    if ($prevCount -gt 0) {
        $drop = [math]::Round((1 - ($cur.Count / $prevCount)) * 100, 1)
        if ($drop -gt $ShrinkGuardPercent) {
            Write-Fail ("원본이 지난 스냅샷보다 {0}% 줄었다. 덮어쓰지 않는다." -f $drop)
            Write-Host ("       지난 스냅샷 {0:N0}개 ({1}) → 지금 {2:N0}개" -f $prevCount, $prev.takenAt, $cur.Count)
            Write-Host "       의도한 정리라면 -Force 를 붙인다."
            Write-Host "       사고라면 -Restore 로 되돌린다. 지금 덮어쓰면 되돌릴 수 없다."
            exit 1
        }
    }
}

New-Item -ItemType Directory -Force $snapFull | Out-Null

Write-Host "원본: $ContentPath"
Write-Host "스냅샷: $snapFull"
Write-Host ("복사 대상: {0:N0}개 / {1:N1} GB" -f $cur.Count, ($cur.Bytes / 1GB))

$started = Get-Date
$rc = Invoke-Mirror $ContentPath $snapFull
$elapsed = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)

if ($rc -ge 8) {
    Write-Fail "robocopy 실패 (코드 $rc)"
    exit 1
}

$snap = Measure-Tree $snapFull
if ($snap.Count -ne $cur.Count) {
    Write-Fail ("파일 수가 맞지 않는다. 원본 {0:N0}개 / 스냅샷 {1:N0}개" -f $cur.Count, $snap.Count)
    exit 1
}

@{
    takenAt   = (Get-Date).ToString('yyyy-MM-ddTHH:mm:ssK')
    source    = $ContentPath
    fileCount = $snap.Count
    bytes     = $snap.Bytes
} | ConvertTo-Json | Set-Content -Path $manifestPath -Encoding utf8

Write-Host ("[완료] {0:N0}개 / {1:N1} GB · {2}초 · robocopy 코드 {3}" -f $snap.Count, ($snap.Bytes / 1GB), $elapsed, $rc)
exit 0
