#Requires -Version 7.0
<#
.SYNOPSIS
    패킷 스키마로 protobuf 코드와 패킷 핸들러 헤더를 만들고 서버, 더미 클라이언트, 클라이언트에 배포한다.

.DESCRIPTION
    Schema/ 의 .proto 를 모두 읽는다. protoc 가 파일마다 *.pb.h 와 *.pb.cc 를, 핸들러 생성기가
    ServerPacketHandler.h 와 ClientPacketHandler.h 를 임시 폴더에 만든다. 생성이 모두 성공한 뒤에만
    아래 목적지로 복사한다.

      Server/GameServer/Protocol/    *.pb.h, *.pb.cc
      Server/GameServer/Network/     ServerPacketHandler.h
      Server/DummyClient/Protocol/   *.pb.h, *.pb.cc
      Server/DummyClient/Main/       ClientPacketHandler.h
      P1/Source/P1/Network/          *.pb.h, *.pb.cc, ClientPacketHandler.h, *.proto

    어느 단계든 실패하면 1 로 끝난다. 실패를 출력만 하고 0 으로 끝나면 빌드가 옛 생성물로
    통과해서, 스키마와 생성물이 어긋난 것을 알아챌 수 없다.

    패킷 번호는 핸들러 생성기가 매긴다. 스키마 파일마다 `// @packet_id_range <첫 번호> <끝 번호>`
    로 정한 범위 안에서 선언 순서로 매기며, 범위가 겹치거나 모자라면 생성기가 실패한다.

.EXAMPLE
    pwsh Protocol/Generate.ps1
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

function Write-Fail([string]$Message) {
    Write-Host "[실패] $Message"
}

$repoRoot = Split-Path $PSScriptRoot -Parent
$schemaDir = Join-Path $PSScriptRoot 'Schema'
$templateDir = Join-Path $PSScriptRoot 'Templates'
$protoc = Join-Path $repoRoot 'Tools' 'protoc.exe'
$generator = Join-Path $repoRoot 'Tools' 'PacketHandlerGenerator' 'PacketHandlerGenerator.exe'

# --- 선행 조건 ---------------------------------------------------------------

foreach ($tool in @($protoc, $generator)) {
    if (-not (Test-Path $tool -PathType Leaf)) {
        Write-Fail "생성 도구가 없다: $tool"
        exit 1
    }
}

# 이름 순서로 고정한다. 순서는 핸들러 헤더의 include 와 줄 순서만 정하고, 패킷 번호는 바꾸지 않는다.
$schemas = @(Get-ChildItem -Path $schemaDir -Filter '*.proto' -File | Sort-Object Name)
if ($schemas.Count -eq 0) {
    Write-Fail "스키마 파일이 없다: $schemaDir"
    exit 1
}

# 솔루션을 병렬로 빌드하면 GameServer 와 DummyClient 의 재생성 타깃이 이 스크립트를 함께 부른다.
# 같은 목적지에 동시에 복사하면 한쪽이 파일 잠금으로 실패하므로 한 번에 하나만 돈다.
$mutex = [System.Threading.Mutex]::new($false, 'Local\P1ProtocolGenerate')
try {
    [void]$mutex.WaitOne()
}
catch [System.Threading.AbandonedMutexException] {
    # 앞의 실행이 뮤텍스를 쥔 채 죽었다. 소유권은 이쪽으로 넘어왔으므로 그대로 진행한다.
}

$workDir = Join-Path ([System.IO.Path]::GetTempPath()) "P1Protocol-$([guid]::NewGuid().ToString('N'))"
New-Item -ItemType Directory -Path $workDir | Out-Null

try {
    # --- 생성 ----------------------------------------------------------------

    & $protoc "--proto_path=$schemaDir" "--cpp_out=$workDir" @($schemas.FullName)
    if ($LASTEXITCODE -ne 0) {
        Write-Fail "protoc 가 종료 코드 $LASTEXITCODE 로 끝났다"
        exit 1
    }

    $pathArgs = @($schemas | ForEach-Object { "--path=$($_.FullName)" })
    $handlers = @(
        @{ Output = 'ClientPacketHandler'; Recv = 'S_'; Send = 'C_' },
        @{ Output = 'ServerPacketHandler'; Recv = 'C_'; Send = 'S_' }
    )
    foreach ($handler in $handlers) {
        # 생성기는 출력을 UTF-8 로 쓴다. 그 출력을 바로 흘려보내면 읽는 쪽(바깥 셸, MSBuild)의 인코딩에 따라
        # 한국어 오류 문구가 깨지므로, 받는 동안만 UTF-8 로 읽고 받은 줄은 이 스크립트가 다시 쓴다.
        $consoleEncoding = [Console]::OutputEncoding
        [Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
        try {
            $generatorOutput = & $generator @pathArgs "--output=$($handler.Output)" "--recv=$($handler.Recv)" `
                "--send=$($handler.Send)" "--templates=$templateDir" "--out-dir=$workDir" 2>&1
            $generatorExitCode = $LASTEXITCODE
        }
        finally {
            [Console]::OutputEncoding = $consoleEncoding
        }
        $generatorOutput | ForEach-Object { Write-Host "$_" }

        if ($generatorExitCode -ne 0) {
            Write-Fail "핸들러 생성기가 $($handler.Output).h 를 만들다 종료 코드 $generatorExitCode 로 끝났다"
            exit 1
        }
    }

    # --- 배포 ----------------------------------------------------------------

    $pbFiles = @(Get-ChildItem -Path $workDir -File | Where-Object { $_.Name -like '*.pb.h' -or $_.Name -like '*.pb.cc' })
    if ($pbFiles.Count -ne $schemas.Count * 2) {
        Write-Fail "protoc 가 스키마 $($schemas.Count)개에 생성물 $($pbFiles.Count)개를 만들었다. 파일마다 둘이어야 한다"
        exit 1
    }

    $serverHandler = Join-Path $workDir 'ServerPacketHandler.h'
    $clientHandler = Join-Path $workDir 'ClientPacketHandler.h'

    $deployments = @(
        @{ Dir = 'Server/GameServer/Protocol'; Files = @($pbFiles.FullName) },
        @{ Dir = 'Server/GameServer/Network'; Files = @($serverHandler) },
        @{ Dir = 'Server/DummyClient/Protocol'; Files = @($pbFiles.FullName) },
        @{ Dir = 'Server/DummyClient/Main'; Files = @($clientHandler) },
        @{ Dir = 'P1/Source/P1/Network'; Files = @($pbFiles.FullName) + @($clientHandler) + @($schemas.FullName) }
    )

    foreach ($deployment in $deployments) {
        $destination = Join-Path $repoRoot $deployment.Dir
        if (-not (Test-Path $destination -PathType Container)) {
            Write-Fail "배포할 폴더가 없다: $destination"
            exit 1
        }
        Copy-Item -Path $deployment.Files -Destination $destination -Force
        Write-Host "[배포] $($deployment.Dir) ($($deployment.Files.Count)개)"
    }
}
finally {
    Remove-Item -LiteralPath $workDir -Recurse -Force -ErrorAction SilentlyContinue
    $mutex.ReleaseMutex()
    $mutex.Dispose()
}

Write-Host "[완료] 스키마 $($schemas.Count)개로 생성물을 만들어 배포했다"
exit 0
