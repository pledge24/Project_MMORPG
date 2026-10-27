#include "Core/pch.h"
#include "Game/Data/Gamedata.h"
#include "Game/Data/GamedataParser.h"
#include <fstream>

namespace
{
    /**
     * 작업 디렉터리(프로젝트 폴더) 기준 상대 경로다.
     * GenJsonFile.bat이 Json을 옮기는 곳과 다르면 부팅 때 기획표를 찾지 못한다.
     */
    constexpr string_view GAMEDATA_DIR = "Game/Data/Json/";

    /** DataBuilder의 산출물 폴더. 작업 디렉터리 기준이고, databuilder.toml의 game 모듈 폴더와 같아야 한다. */
    constexpr string_view GENERATED_DIR = "Game/Data/Generated/";

    /** 파일을 열지 못하거나 Json 문법이 틀리면 사유를 돌려준다. */
    optional<string> ReadDocument(string_view dir, string_view fileName, OUT Json& document)
    {
        ifstream file(string(dir) + string(fileName));
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
    // TODO: 퀘스트를 만들 때 퀘스트 표(S_Quest.json)도 템플릿과 함께 불러온다.
    GamedataDocuments documents;

    /** 옛 변환기의 표와 DataBuilder의 표는 폴더가 다르다. */
    struct DocumentFile
    {
        string_view dir;
        string_view fileName;
        Json* document;
    };

    const DocumentFile files[] = {
        { GAMEDATA_DIR, GamedataFile::WARRIOR_LEVELS, &documents.warriorLevels },
        { GAMEDATA_DIR, GamedataFile::ITEMS, &documents.items },
        { GAMEDATA_DIR, GamedataFile::MAPS, &documents.maps },
        { GAMEDATA_DIR, GamedataFile::MONSTERS, &documents.monsters },
        { GAMEDATA_DIR, GamedataFile::INITIAL_PROGRESS, &documents.initialProgress },
        { GAMEDATA_DIR, GamedataFile::INVENTORY_SECTIONS, &documents.inventorySections },
        { GAMEDATA_DIR, GamedataFile::QUICK_SLOTS, &documents.quickSlots },
        { GENERATED_DIR, DesignData::CURRENCY_FILE, &documents.currencies },
    };

    for (const DocumentFile& file : files)
    {
        if (optional<string> error = ReadDocument(file.dir, file.fileName, OUT *file.document))
        {
            GLogger->Error("기획 데이터를 불러오지 못했다. {}", error.value());
            return false;
        }
    }

    // 어느 원본으로 만든 데이터를 읽었는지 로그로 남긴다. 원본 해시라서 같은 원본이면 같은 값이다.
    Json versionDocument;
    string dataVersion;
    optional<string> versionError = ReadDocument(GENERATED_DIR, DesignData::VERSION_FILE, OUT versionDocument);
    if (versionError.has_value() == false)
        versionError = DesignData::ParseDataVersion(versionDocument, OUT dataVersion);
    if (versionError.has_value())
    {
        GLogger->Error("기획 데이터 버전을 읽지 못했다. {}", versionError.value());
        return false;
    }

    if (optional<string> error = Load(documents))
    {
        GLogger->Error("기획 데이터 검증에 실패했다. {}", error.value());
        return false;
    }

    GLogger->Info("기획 데이터 버전 {}을 불러왔다", dataVersion);
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

int32 Gamedata::GetInventorySlotCount(Protocol::ItemType itemType)
{
    auto it = s_tables.inventorySlotCounts.find(itemType);
    return it == s_tables.inventorySlotCounts.end() ? 0 : it->second;
}
