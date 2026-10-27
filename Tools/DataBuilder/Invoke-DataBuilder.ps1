#Requires -Version 7.0
<#
.SYNOPSIS
    DataBuilder를 돌린다. 기획 데이터 원본을 검사하고 서버가 읽는 JSON과 C++ 타입을 생성한다.

.DESCRIPTION
    첫 실행에 이 폴더의 .venv에 가상 환경을 만들고 requirements.txt의 고정 버전 패키지를 설치한다.
    requirements.txt가 바뀌면 다시 설치한다. 머신에 설치된 패키지 버전이 산출물을 바꾸지 않게 하려는 것이다.

    DataBuilder는 임시 폴더에 생성하고, 검사와 생성이 모두 성공해야 산출물 폴더에 배포한다.
    실패하면 산출물을 바꾸지 않고 1로 끝난다.

.PARAMETER Test
    생성하지 않고 DataBuilder 테스트를 돌린다.
#>
[CmdletBinding()]
param(
    [switch]$Test
)

$ErrorActionPreference = 'Stop'

$toolDir = $PSScriptRoot
$venvDir = Join-Path $toolDir '.venv'
$python = Join-Path $venvDir 'Scripts/python.exe'
$requirements = Join-Path $toolDir 'requirements.txt'
$stamp = Join-Path $venvDir 'requirements.sha256'

# 가상 환경을 만든다. 이 머신의 python3 는 MS Store 별칭 스텁이라 py -3 을 쓴다.
if (-not (Test-Path $python)) {
    Write-Host '[DataBuilder] 가상 환경을 만든다'
    & py -3 -m venv $venvDir
    if ($LASTEXITCODE -ne 0) {
        Write-Error "[DataBuilder] 가상 환경을 만들지 못했다(종료 코드 $LASTEXITCODE)"
        exit 1
    }
}

# 고정한 패키지를 설치한다. requirements.txt 의 해시가 지난 설치와 같으면 건너뛴다.
$wanted = (Get-FileHash $requirements -Algorithm SHA256).Hash
$installed = if (Test-Path $stamp) { (Get-Content $stamp -Raw).Trim() } else { '' }
if ($wanted -ne $installed) {
    Write-Host '[DataBuilder] 고정한 패키지를 설치한다'
    & $python -m pip install --disable-pip-version-check --quiet --require-virtualenv -r $requirements
    if ($LASTEXITCODE -ne 0) {
        Write-Error "[DataBuilder] 패키지를 설치하지 못했다(종료 코드 $LASTEXITCODE)"
        exit 1
    }
    Set-Content -Path $stamp -Value $wanted -NoNewline
}

$env:PYTHONUTF8 = '1'
Push-Location $toolDir
try {
    if ($Test) {
        & $python -m unittest discover --start-directory tests --top-level-directory .
    }
    else {
        & $python (Join-Path $toolDir 'build_design_data.py')
    }
    $code = $LASTEXITCODE
}
finally {
    Pop-Location
}
exit $code
