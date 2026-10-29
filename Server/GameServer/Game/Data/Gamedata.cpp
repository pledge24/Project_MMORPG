#include "Core/pch.h"
#include "Game/Data/Gamedata.h"
#include "Game/Data/GamedataParser.h"
#include <filesystem>
#include <fstream>

namespace
{
    /** DataBuilder의 산출물 폴더. 작업 디렉터리 기준이고, databuilder.toml의 game 모듈 폴더와 같아야 한다. */
    constexpr string_view GENERATED_DIR = "Game/Data/Generated/";
    /** 배치 추출과 크리처 추출의 산출물 폴더. 작업 디렉터리 기준이고, 추출 커맨드렛이 쓰는 폴더와 같아야 한다. */
    constexpr string_view EXTRACTED_DIR = "Game/Data/Extracted/";

    /** 파일을 열지 못하거나 Json 문법이 틀리면 사유를 돌려준다. */
    optional<string> ReadDocument(string_view fileName, OUT Json& document, string_view dir = GENERATED_DIR)
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
    GamedataDocuments documents;

    const pair<string_view, Json*> files[] = {
        { DesignData::CURRENCY_FILE, &documents.currencies },
        { DesignData::ITEM_FILE, &documents.items },
        { DesignData::ITEM_EQUIP_STAT_FILE, &documents.itemEquipStats },
        { DesignData::ITEM_CONSUME_EFFECT_FILE, &documents.itemConsumeEffects },
        { DesignData::LEVEL_FILE, &documents.levels },
        { DesignData::CLASS_LEVEL_STAT_FILE, &documents.classLevelStats },
        { DesignData::MONSTER_FILE, &documents.monsters },
        { DesignData::DROP_GROUP_FILE, &documents.dropGroups },
        { DesignData::MAP_FILE, &documents.maps },
        { DesignData::AREA_FILE, &documents.areas },
        { DesignData::PROGRESS_FILE, &documents.initialProgress },
        { DesignData::SHOP_FILE, &documents.shops },
        { DesignData::NPC_FILE, &documents.npcs },
        { DesignData::INTERACTION_FILE, &documents.interaction },
    };

    for (const auto& [fileName, document] : files)
    {
        if (optional<string> error = ReadDocument(fileName, OUT *document))
        {
            GLogger->Error("기획 데이터를 불러오지 못했다. {}", error.value());
            return false;
        }
    }

    // 맵마다 하나인 배치 데이터. 어느 맵의 파일이 빠졌는지는 검증이 맵 표와 대조해 알린다.
    error_code directoryError;
    for (const filesystem::directory_entry& entry : filesystem::directory_iterator(EXTRACTED_DIR, directoryError))
    {
        const string fileName = entry.path().filename().string();
        if (fileName.starts_with(PLACEMENT_FILE_PREFIX) == false)
            continue;

        if (optional<string> error = ReadDocument(fileName, OUT documents.placements[fileName], EXTRACTED_DIR))
        {
            GLogger->Error("배치 데이터를 불러오지 못했다. {}", error.value());
            return false;
        }
    }
    if (directoryError)
    {
        GLogger->Error("배치 데이터 폴더 {}를 읽지 못했다: {}", EXTRACTED_DIR, directoryError.message());
        return false;
    }

    if (optional<string> error = ReadDocument(CREATURE_COMBAT_FILE, OUT documents.creatureCombat, EXTRACTED_DIR))
    {
        GLogger->Error("크리처 판정 데이터를 불러오지 못했다. {}. 에셋에서 추출을 돌린다", error.value());
        return false;
    }

    // 어느 원본으로 만든 데이터를 읽었는지 로그로 남긴다. 원본 해시라서 같은 원본이면 같은 값이다.
    Json versionDocument;
    string dataVersion;
    optional<string> versionError = ReadDocument(DesignData::VERSION_FILE, OUT versionDocument);
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

const DesignData::ItemRow* Gamedata::FindItem(int32 templateId)
{
    auto it = s_tables.items.find(templateId);
    return it == s_tables.items.end() ? nullptr : &it->second;
}

const DesignData::ItemEquipStatRow* Gamedata::FindItemEquipStat(int32 templateId)
{
    auto it = s_tables.itemEquipStats.find(templateId);
    return it == s_tables.itemEquipStats.end() ? nullptr : &it->second;
}

const DesignData::ItemConsumeEffectRow* Gamedata::FindItemConsumeEffect(int32 templateId)
{
    auto it = s_tables.itemConsumeEffects.find(templateId);
    return it == s_tables.itemConsumeEffects.end() ? nullptr : &it->second;
}

const DesignData::MonsterRow* Gamedata::FindMonster(int32 templateId)
{
    auto it = s_tables.monsters.find(templateId);
    return it == s_tables.monsters.end() ? nullptr : &it->second;
}

const DesignData::DropGroupRow* Gamedata::FindDropGroup(int32 dropGroupId)
{
    auto it = s_tables.dropGroups.find(dropGroupId);
    return it == s_tables.dropGroups.end() ? nullptr : &it->second;
}

const DesignData::AreaRow* Gamedata::FindArea(int32 areaId)
{
    auto it = s_tables.areas.find(areaId);
    return it == s_tables.areas.end() ? nullptr : &it->second;
}

const DesignData::ShopRow* Gamedata::FindShop(int32 shopId)
{
    auto it = s_tables.shops.find(shopId);
    return it == s_tables.shops.end() ? nullptr : &it->second;
}

const DesignData::NpcRow* Gamedata::FindNpc(int32 npcId)
{
    auto it = s_tables.npcs.find(npcId);
    return it == s_tables.npcs.end() ? nullptr : &it->second;
}

const DesignData::ClassLevelStatRow* Gamedata::FindClassLevelStat(int32 classId, int32 level)
{
    auto it = s_tables.classLevelStats.find({ static_cast<Protocol::CharacterClass>(classId), level });
    return it == s_tables.classLevelStats.end() ? nullptr : &it->second;
}

const MapPlacement* Gamedata::FindMapPlacement(int32 mapId)
{
    auto it = s_tables.mapPlacements.find(mapId);
    return it == s_tables.mapPlacements.end() ? nullptr : &it->second;
}

const AreaPlacement* Gamedata::FindAreaPlacement(int32 areaId)
{
    auto it = s_tables.areaPlacements.find(areaId);
    return it == s_tables.areaPlacements.end() ? nullptr : &it->second;
}

const PortalPlacement* Gamedata::FindPortalPlacement(int32 portalId)
{
    auto it = s_tables.portalPlacements.find(portalId);
    return it == s_tables.portalPlacements.end() ? nullptr : &it->second;
}

const RespawnPointPlacement* Gamedata::FindRespawnPoint(int32 areaId)
{
    auto it = s_tables.respawnPoints.find(areaId);
    return it == s_tables.respawnPoints.end() ? nullptr : &it->second;
}

const CreatureCombat* Gamedata::FindMonsterCombat(int32 templateId)
{
    auto it = s_tables.monsterCombats.find(templateId);
    return it == s_tables.monsterCombats.end() ? nullptr : &it->second;
}

const CreatureCombat* Gamedata::FindClassCombat(Protocol::CharacterClass classId)
{
    auto it = s_tables.classCombats.find(classId);
    return it == s_tables.classCombats.end() ? nullptr : &it->second;
}
