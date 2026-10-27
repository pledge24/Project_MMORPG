@echo off
setlocal
pushd %~dp0
..\Tools\ExcelToJsonConverter\ExcelToJsonConverter.exe --path=./Original_Quest.xlsx --s_output=S_Quest.json --c_output=C_Quest.json 

IF ERRORLEVEL 1 PAUSE

MOVE /Y S_Quest.json				    "../Server/GameServer/Game/Data/Json"

MOVE /Y C_Quest.json				    "../P1/Content/P1/Data/DataTables"

echo [INFO] 완료되었습니다.
PAUSE
popd
endlocal