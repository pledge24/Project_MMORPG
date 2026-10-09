#include "Core/pch.h"
#include "Game/Data/Gamedata.h"
#include "Game/Data/GamedataParser.h"
#include <fstream>

namespace
{
    /**
     * 게임 데이터 JSON 위치. 작업 디렉터리(프로젝트 폴더) 기준 상대 경로이다.
     * GenJsonFile.bat의 MOVE 목적지와 반드시 같아야 한다.
     */
    constexpr string_view GAMEDATA_DIR = "Game/Data/Json/";

    /** 파일을 열지 못하거나 Json 문법이 틀리면 사유를 돌려준다. */
    optional<string> ReadDocument(string_view fileName, OUT Json& document)
    {
        ifstream file(string(GAMEDATA_DIR) + string(fileName));
        if (file.is_open() == false)
            return format("{}: 파일을 열지 못했다", fileName);

        try
        {
            document = Json::parse(file);
        }
        catch (const Json::parse_error& e)
        {
            return format("{}: Json 문법 오류(byte {}): {}", fileName, e.byte, e.what());
        }

        return nullopt;
    }
}

GamedataTables Gamedata::s_tables;

bool Gamedata::LoadAllGamedata()
{
    // 퀘스트 표(S_Quest.json)는 읽는 코드가 없어 불러오지 않는다. 퀘스트를 만들 때 템플릿과 함께 추가한다.
    GamedataDocuments documents;
    const pair<string_view, Json*> files[] = {
        { GamedataFile::WARRIOR_LEVELS, &documents.warriorLevels },
        { GamedataFile::ITEMS, &documents.items },
        { GamedataFile::MAPS, &documents.maps },
        { GamedataFile::MONSTERS, &documents.monsters },
    };

    for (const auto& [fileName, document] : files)
    {
        if (optional<string> error = ReadDocument(fileName, OUT *document))
        {
            GLogger->Error("기획 데이터를 불러오지 못했다. {}", error.value());
            return false;
        }
    }

    if (optional<string> error = Load(documents))
    {
        GLogger->Error("기획 데이터 검증에 실패했다. {}", error.value());
        return false;
    }

    return true;
}

optional<string> Gamedata::Load(const GamedataDocuments& documents)
{
    GamedataTables tables;
    if (optional<string> error = GamedataParser::Parse(documents, OUT tables))
        return error;

    Install(std::move(tables));
    return nullopt;
}

void Gamedata::Install(GamedataTables tables)
{
    s_tables = std::move(tables);
}

const ItemTemplate* Gamedata::FindItem(int32 templateId)
{
    auto it = s_tables.items.find(templateId);
    return it == s_tables.items.end() ? nullptr : &it->second;
}

const MonsterTemplate* Gamedata::FindMonster(int32 templateId)
{
    auto it = s_tables.monsters.find(templateId);
    return it == s_tables.monsters.end() ? nullptr : &it->second;
}

const MapTemplate* Gamedata::FindMap(int32 templateId)
{
    auto it = s_tables.maps.find(templateId);
    return it == s_tables.maps.end() ? nullptr : &it->second;
}

const ClassLevelTable* Gamedata::FindClassLevelTable(int32 classId)
{
    auto it = s_tables.classLevelTables.find(classId);
    return it == s_tables.classLevelTables.end() ? nullptr : &it->second;
}
