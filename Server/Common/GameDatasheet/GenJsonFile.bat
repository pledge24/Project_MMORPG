@echo off
setlocal
pushd %~dp0
ExcelToJsonConverter.exe --path=./Original_Warrior_Level_Data.xlsx --s_output=S_Warrior_Level_Data.json
ExcelToJsonConverter.exe --path=./Original_Monster.xlsx --s_output=S_Monster.json --c_output=C_Monster.json 
ExcelToJsonConverter.exe --path=./Original_Item.xlsx --s_output=S_Item.json --c_output=C_Item.json 
ExcelToJsonConverter.exe --path=./Original_Map.xlsx --s_output=S_Map.json --c_output=C_Map.json 
ExcelToJsonConverter.exe --path=./Original_Quest.xlsx --s_output=S_Quest.json --c_output=C_Quest.json 

IF ERRORLEVEL 1 PAUSE

MOVE /Y S_Warrior_Level_Data.json		"../../GameServer"
MOVE /Y S_Monster.json				    "../../GameServer"
MOVE /Y S_Item.json				        "../../GameServer"
MOVE /Y S_Map.json				        "../../GameServer"
MOVE /Y S_Quest.json				    "../../GameServer"

MOVE /Y C_Monster.json				    "../../../P1/Content/Gamedata"
MOVE /Y C_Item.json				        "../../../P1/Content/Gamedata"
MOVE /Y C_Map.json				        "../../../P1/Content/Gamedata"
MOVE /Y C_Quest.json				    "../../../P1/Content/Gamedata"

echo [INFO] 완료되었습니다.
PAUSE
popd
endlocal