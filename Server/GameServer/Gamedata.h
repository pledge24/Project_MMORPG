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
    static const Protocol::PosInfo& GetRespawnPoint() { return RESPAWN_POINT; }

#ifdef _DEBUG
    static void PrintAllGamedata();
#endif

public:
    /* 직업별 레벨 테이블 */
    static DataTable InvalidLevelDataTable;
    static DataTable WarriorLevelDataTable;

    /* 레벨 테이블 매핑 */
    static unordered_map<int32, DataTable*> ClassLevelDataTableMappings;

    /* 게임 데이터 */
    static DataTable ItemDataTable;
    static DataTable MapDataTable;
    static DataTable MonsterDataTable;
    static DataTable QuestDataTable;

    static const int32 RESPAWN_ROOM_ID = 10;
    static Protocol::PosInfo RESPAWN_POINT;
};