@echo off
setlocal
pushd %~dp0

pyinstaller --noconfirm --onefile PacketHandlerGenerator.py
IF ERRORLEVEL 1 (
    echo [ERROR] PyInstaller 빌드 실패
    PAUSE
    EXIT /b 1
)

MOVE /Y ".\dist\PacketHandlerGenerator.exe"         "./"
IF ERRORLEVEL 1 (
    echo [ERROR] 실행 파일 이동 실패
    PAUSE
    EXIT /b 1
)

RD /S /Q ".\build"
RD /S /Q ".\dist"
DEL /F /Q ".\PacketHandlerGenerator.spec"

XCOPY /Y PacketHandlerGenerator.exe		            "../../Common/Protobuf/bin"
robocopy ".\Templates"                              "../../Common/Protobuf/bin/Templates" /E

echo [INFO] 완료되었습니다.
PAUSE
popd
endlocal
