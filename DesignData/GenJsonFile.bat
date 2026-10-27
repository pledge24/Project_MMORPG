@echo off
setlocal
pushd %~dp0
..\Tools\ExcelToJsonConverter\ExcelToJsonConverter.exe --path=./Original_Warrior_Level_Data.xlsx --s_output=S_Warrior_Level_Data.json --c_output=C_Warrior_Level_Data.json
DEL /Q C_Warrior_Level_Data.json
..\Tools\ExcelToJsonConverter\ExcelToJsonConverter.exe --path=./Original_Initial_Progress.xlsx --s_output=S_Initial_Progress.json --c_output=C_Initial_Progress.json
DEL /Q C_Initial_Progress.json
..\Tools\ExcelToJsonConverter\ExcelToJsonConverter.exe --path=./Original_Inventory_Section.xlsx --s_output=S_Inventory_Section.json --c_output=C_Inventory_Section.json
DEL /Q C_Inventory_Section.json
..\Tools\ExcelToJsonConverter\ExcelToJsonConverter.exe --path=./Original_Quick_Slot.xlsx --s_output=S_Quick_Slot.json --c_output=C_Quick_Slot.json
DEL /Q C_Quick_Slot.json
..\Tools\ExcelToJsonConverter\ExcelToJsonConverter.exe --path=./Original_Lobby.xlsx --s_output=S_Lobby.json --c_output=C_Lobby.json
DEL /Q C_Lobby.json
..\Tools\ExcelToJsonConverter\ExcelToJsonConverter.exe --path=./Original_Monster.xlsx --s_output=S_Monster.json --c_output=C_Monster.json 
..\Tools\ExcelToJsonConverter\ExcelToJsonConverter.exe --path=./Original_Map.xlsx --s_output=S_Map.json --c_output=C_Map.json 
..\Tools\ExcelToJsonConverter\ExcelToJsonConverter.exe --path=./Original_Quest.xlsx --s_output=S_Quest.json --c_output=C_Quest.json 

IF ERRORLEVEL 1 PAUSE

MOVE /Y S_Warrior_Level_Data.json		"../Server/GameServer/Game/Data/Json"
MOVE /Y S_Monster.json				    "../Server/GameServer/Game/Data/Json"
MOVE /Y S_Map.json				        "../Server/GameServer/Game/Data/Json"
MOVE /Y S_Quest.json				    "../Server/GameServer/Game/Data/Json"
MOVE /Y S_Initial_Progress.json		"../Server/GameServer/Game/Data/Json"
MOVE /Y S_Inventory_Section.json		"../Server/GameServer/Game/Data/Json"
MOVE /Y S_Quick_Slot.json			"../Server/GameServer/Game/Data/Json"

MOVE /Y S_Lobby.json				"../Server/GameServer/Lobby/Data/Json"

MOVE /Y C_Monster.json				    "../P1/Content/P1/Data/DataTables"
MOVE /Y C_Map.json				        "../P1/Content/P1/Data/DataTables"
MOVE /Y C_Quest.json				    "../P1/Content/P1/Data/DataTables"

echo [INFO] 완료되었습니다.
PAUSE
popd
endlocal