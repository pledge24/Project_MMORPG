#include "pch.h"
#include "Gamedata.h"
#include "Global.h"
#include <fstream>
#include "EncodingConverter.h"

/*-------------------
       Gamedata
---------------------*/

map<pair<int32, int32>, Json> Gamedata::CharacterDataTable;
unordered_map<int32, Json> Gamedata::ItemDataTable;
unordered_map<int32, Json> Gamedata::MapDataTable;
unordered_map<int32, Json> Gamedata::MonsterDataTable;
unordered_map<int32, Json> Gamedata::QuestDataTable;

bool Gamedata::LoadAllGamedata()
{
    // 1. 캐릭터 정보
    try
    {
        ifstream file("S_Character.json");
        if (file.is_open())
        {
            Json json_data = Json::parse(file);
            for (auto& row : json_data)
            {
                int32 classId = ClassMappings[row["class"]];
                int32 level = row["level"];
                CharacterDataTable[make_pair(classId, level)] = row;
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

    // 3. 맵 정보
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

    // 4. 몬스터 정보
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

    // 5. 퀘스트 정보
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
    for (auto elem : CharacterDataTable)
    {
        string str = elem.second.dump();
        wcout << EncodingConverter::StringToWString(str) << '\n';
    }

    for (auto elem : ItemDataTable)
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
