#pragma once

#include "nlohmann/json.hpp"
using Json = nlohmann::json;

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

    /* 게임 데이터 */
    static map<pair<int32, int32>, Json> CharacterDataTable;    // <<classId, level>, LevelTable>
    static unordered_map<int32, Json> ItemDataTable;
    static unordered_map<int32, Json> MapDataTable;
    static unordered_map<int32, Json> MonsterDataTable;
    static unordered_map<int32, Json> QuestDataTable;
};