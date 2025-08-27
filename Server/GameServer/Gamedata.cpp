#include "pch.h"
#include "Gamedata.h"
#include "Global.h"
#include <fstream>
#include "EncodingConverter.h"

/*-------------------
       Gamedata
---------------------*/

/* 직업별 레벨 테이블 */
DataTable Gamedata::WarriorLevelDataTable;

/* 레벨 테이블 매핑 */
unordered_map<int32, DataTable*> Gamedata::ClassLevelDataTableMappings;

/* 게임 데이터 */
DataTable Gamedata::ItemDataTable;
DataTable Gamedata::EquipmentDataTable;
DataTable Gamedata::MapDataTable;
DataTable Gamedata::MonsterDataTable;
DataTable Gamedata::QuestDataTable;

bool Gamedata::LoadAllGamedata()
{
    // 레벨 테이블 매핑 초기화
    ClassLevelDataTableMappings = {
        make_pair(Protocol::CharacterClass::CLASS_TYPE_WARRIOR, &WarriorLevelDataTable)
    };

    // 1. 캐릭터 정보
    try
    {
        ifstream file("S_Warrior_Level_Data.json");
        if (file.is_open())
        {
            Json json_data = Json::parse(file);
            for (auto& row : json_data)
            {
                int32 level = row["level"];
                WarriorLevelDataTable[level] = row;
            }
        }
    }
    catch (const exception& e)
    {
        wcerr << L"캐릭터 데이터 저장 오류" << e.what() << endl;
        return false;
    }

    // 2. 아이템 정보
    try
    {
        ifstream file("S_Item.json");
        if (file.is_open())
        {
            Json json_data = Json::parse(file);
            for (auto& row : json_data)
            {
                int32 templateId = row["template_id"];
                ItemDataTable[templateId] = row;
            }
        }
    }
    catch (const exception& e)
    {
        wcerr << L"아이템 데이터 저장 오류" << e.what() << endl;
        return false;
    }

    // 3. 장비 정보
    try
    {
        ifstream file("S_Equipment.json");
        if (file.is_open())
        {
            Json json_data = Json::parse(file);
            for (auto& row : json_data)
            {
                int32 templateId = row["template_id"];
                EquipmentDataTable[templateId] = row;
            }
        }
    }
    catch (const exception& e)
    {
        wcerr << L"장비 데이터 저장 오류" << e.what() << endl;
        return false;
    }

    // 4. 맵 정보
    try
    {
        ifstream file("S_Map.json");
        if (file.is_open())
        {
            Json json_data = Json::parse(file);
            for (auto& row : json_data)
            {
                int32 templateId = row["template_id"];
                MapDataTable[templateId] = row;
            }
        }
    }
    catch (const exception& e)
    {
        wcerr << L"맵 데이터 저장 오류" << e.what() << endl;
        return false;
    }

    // 5. 몬스터 정보
    try
    {
        ifstream file("S_Monster.json");
        if (file.is_open())
        {
            Json json_data = Json::parse(file);
            for (auto& row : json_data)
            {
                int32 templateId = row["template_id"];
                MonsterDataTable[templateId] = row;
            }
        }
    }
    catch (const exception& e)
    {
        wcerr << L"몬스터 데이터 저장 오류" << e.what() << endl;
        return false;
    }

    // 6. 퀘스트 정보
    try
    {
        ifstream file("S_Quest.json");
        if (file.is_open())
        {
            Json json_data = Json::parse(file);
            for (auto& row : json_data)
            {
                int32 templateId = row["template_id"];
                QuestDataTable[templateId] = row;
            }
        }
    }
    catch (const exception& e)
    {
        wcerr << L"퀘스트 데이터 저장 오류" << e.what() << endl;
        return false;
    }

    return true;
}

#ifdef _DEBUG
void Gamedata::PrintAllGamedata()
{
    for (auto elem : WarriorLevelDataTable)
    {
        string str = elem.second.dump();
        wcout << EncodingConverter::StringToWString(str) << '\n';
    }

    for (auto elem : ItemDataTable)
    {
        string str = elem.second.dump();
        wcout << EncodingConverter::StringToWString(str) << '\n';
    }

    for (auto elem : EquipmentDataTable)
    {
        string str = elem.second.dump();
        wcout << EncodingConverter::StringToWString(str) << '\n';
    }

    for (auto elem : MapDataTable)
    {
        string str = elem.second.dump();
        wcout << EncodingConverter::StringToWString(str) << '\n';
    }

    for (auto elem : MonsterDataTable)
    {
        string str = elem.second.dump();
        wcout << EncodingConverter::StringToWString(str) << '\n';
    }

    for (auto elem : QuestDataTable)
    {
        string str = elem.second.dump();
        wcout << EncodingConverter::StringToWString(str) << '\n';
    }
}
#endif