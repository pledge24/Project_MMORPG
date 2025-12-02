#include "pch.h"
#include "Gamedata.h"
#include "Global.h"
#include <fstream>
#include "EncodingConverter.h"

/*-------------------
       Gamedata
---------------------*/

/* 직업별 레벨 테이블 */
DataTable Gamedata::InvalidLevelDataTable;
DataTable Gamedata::WarriorLevelDataTable;

/* 레벨 테이블 매핑 */
unordered_map<int32, DataTable*> Gamedata::ClassLevelDataTableMappings;

/* 게임 데이터 */
DataTable Gamedata::ItemDataTable;
DataTable Gamedata::MapDataTable;
DataTable Gamedata::MonsterDataTable;
DataTable Gamedata::QuestDataTable;

Protocol::PosInfo Gamedata::RESPAWN_POINT;

bool Gamedata::LoadAllGamedata()
{
    // 레벨 테이블 매핑 초기화
    ClassLevelDataTableMappings = {
        make_pair(Protocol::CharacterClass::CLASS_TYPE_NONE, &InvalidLevelDataTable),
        make_pair(Protocol::CharacterClass::CLASS_TYPE_WARRIOR, &WarriorLevelDataTable)
    };

    try
    {
        // 1. 캐릭터 정보
        {
            ifstream file("S_Warrior_Level_Data.json");
            if (file.is_open() == false)
                throw wstring(L"LevelTable JSON 파일 열기 실패");
   
            Json json_data = Json::parse(file);
            for (auto& row : json_data)
            {
                if (row.contains(JsonProperty::LevelTable::Level) == false)
                    throw wstring(L"LevelTable JSON 파일에 level 정보가 존재하지 않음");

                int32 level = row[JsonProperty::LevelTable::Level];
                WarriorLevelDataTable[level] = row;
            }
        }

        // 2. 아이템 정보
        {
            ifstream file("S_Item.json");
            if (file.is_open() == false)
                throw wstring(L"Item JSON 파일 열기 실패");

            Json json_data = Json::parse(file);
            for (auto& row : json_data)
            {
                if(row.contains(JsonProperty::Item::TemplateId) == false)
                    throw wstring(L"Item JSON 파일에 templateId 정보가 존재하지 않음");

                int32 templateId = row[JsonProperty::Item::TemplateId];
                ItemDataTable[templateId] = row;
            }
        }
    
        // 3. 맵 정보
        {
            ifstream file("S_Map.json");
            if (file.is_open() == false)
                throw wstring(L"Map JSON 파일 열기 실패");

            Json json_data = Json::parse(file);
            for (auto& row : json_data)
            {
                if(row.contains(JsonProperty::Map::TemplateId) == false)
                    throw wstring(L"Map JSON 파일에 templateId 정보가 존재하지 않음");

                int32 templateId = row[JsonProperty::Map::TemplateId];
                MapDataTable[templateId] = row;

                // 리스폰 포인트 저장
                if (templateId == RESPAWN_ROOM_ID)
                {
                    if (row.contains(JsonProperty::Map::SpawnPoint) == false)
                        throw wstring(L"Map JSON 파일에 SpawnPoint 정보가 존재하지 않음");

                    const Json& respawnPoint = row[JsonProperty::Map::SpawnPoint];
                    float posX = respawnPoint[JsonProperty::Map::PosX];
                    float posY = respawnPoint[JsonProperty::Map::PosY];
                    float posZ = respawnPoint[JsonProperty::Map::PosZ];

                    RESPAWN_POINT.set_x(posX);
                    RESPAWN_POINT.set_y(posY);
                    RESPAWN_POINT.set_z(posZ);
                    RESPAWN_POINT.set_yaw(0.f);
                    RESPAWN_POINT.set_state(Protocol::MoveState::MOVE_STATE_IDLE);
                }
            }
        }

        // 4. 몬스터 정보
        {
            ifstream file("S_Monster.json");
            if (file.is_open() == false)
                throw wstring(L"Monster JSON 파일 열기 실패");

            Json json_data = Json::parse(file);
            for (auto& row : json_data)
            {
                if(row.contains(JsonProperty::Monster::TemplateId) == false)
                    throw wstring(L"Monster JSON 파일에 templateId 정보가 존재하지 않음");

                int32 templateId = row[JsonProperty::Monster::TemplateId];
                MonsterDataTable[templateId] = row;
            }
        }

        // 5. 퀘스트 정보(현재 사용하지 않음)
        {
            ifstream file("S_Quest.json");
            if (file.is_open() == false)
                throw wstring(L"Quest JSON 파일 열기 실패");

            Json json_data = Json::parse(file);
            for (auto& row : json_data)
            {
                int32 templateId = row["templateId"];
                QuestDataTable[templateId] = row;
            }
        }
    
    }
    catch (const wstring cause)
    {
        wcout << L"Gamedata JSON 파일 로드 중 오류 발생: " << cause << endl;
        return false;
    }
    catch (const Json::parse_error& e)
    {
        // JSON 파싱 실패 시 예외 처리
        wcout << L"JSON 파싱 오류 발생: " << e.what() << endl;
        wcout << L"오류 코드: " << e.id << endl;
        wcout << L"오류 발생 위치 (byte offset): " << e.byte << endl;
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