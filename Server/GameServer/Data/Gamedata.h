#pragma once

#include "nlohmann/json.hpp"

/*-------------------
       Gamedata
---------------------*/
using Json = nlohmann::json;
using DataTable = unordered_map<int32, Json>;

class Gamedata
{
public:
    static bool LoadAllGamedata();

#ifdef _DEBUG
    static void PrintAllGamedata();
#endif

public:
    /* 직업별 레벨 테이블 */
    static DataTable s_invalidLevelDataTable;
    static DataTable s_warriorLevelDataTable;

    /* 레벨 테이블 매핑 */
    static unordered_map<int32, DataTable*> s_classLevelDataTableMappings;

    /* 게임 데이터 */
    static DataTable s_itemDataTable;
    static DataTable s_mapDataTable;
    static DataTable s_monsterDataTable;
    static DataTable s_questDataTable;
};