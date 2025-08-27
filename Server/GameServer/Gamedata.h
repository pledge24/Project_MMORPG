#pragma once

#include "nlohmann/json.hpp"
using Json = nlohmann::json;
using DataTable = unordered_map<int32, Json>;

/*-------------------
       Gamedata
---------------------*/

class Gamedata
{
public:
    static bool LoadAllGamedata();

#ifdef _DEBUG
    static void PrintAllGamedata();
#endif

public:
    /* 직업별 레벨 테이블 */
    static DataTable WarriorLevelDataTable;

    /* 레벨 테이블 매핑 */
    static unordered_map<int32, DataTable*> ClassLevelDataTableMappings;

    /* 게임 데이터 */
    static DataTable ItemDataTable;
    static DataTable EquipmentDataTable;
    static DataTable MapDataTable;
    static DataTable MonsterDataTable;
    static DataTable QuestDataTable;


};