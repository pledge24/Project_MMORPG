@echo off
setlocal
pushd %~dp0
ExcelToJsonConverter.exe --path=./Item.xlsx --output=Item.json
ExcelToJsonConverter.exe --path=./Monster.xlsx --output=Monster.json
ExcelToJsonConverter.exe --path=./Quest.xlsx --output=Quest.json

IF ERRORLEVEL 1 PAUSE

MOVE /Y Item.json					"../../GameServer"
MOVE /Y Monster.json				"../../GameServer"
MOVE /Y Quest.json				    "../../GameServer"

echo [INFO] 완료되었습니다.
PAUSE
popd
endlocal