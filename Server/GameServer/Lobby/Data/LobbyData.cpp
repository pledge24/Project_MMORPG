#include "Core/pch.h"
#include "Lobby/Data/LobbyData.h"
#include <fstream>

namespace
{
    /** DataBuilder의 산출물 폴더. 작업 디렉터리 기준이고, databuilder.toml의 lobby 모듈 폴더와 같아야 한다. */
    constexpr string_view GENERATED_DIR = "Lobby/Data/Generated/";
}

LobbyDesignData::LobbyConfig LobbyData::s_config;

bool LobbyData::LoadAll()
{
    ifstream file(string(GENERATED_DIR) + string(LobbyDesignData::LOBBY_FILE));
    if (file.is_open() == false)
    {
        GLogger->Error("로비 기획 데이터를 불러오지 못했다. {}: 파일을 열지 못했다", LobbyDesignData::LOBBY_FILE);
        return false;
    }

    Json document;
    try
    {
        document = Json::parse(file);
    }
    catch (const Json::parse_error& e)
    {
        GLogger->Error("로비 기획 데이터를 불러오지 못했다. {}: Json 문법 오류(byte {}): {}", LobbyDesignData::LOBBY_FILE, e.byte, e.what());
        return false;
    }

    if (optional<string> error = Load(document))
    {
        GLogger->Error("로비 기획 데이터 검증에 실패했다. {}", error.value());
        return false;
    }

    return true;
}

optional<string> LobbyData::Load(const Json& document)
{
    LobbyDesignData::LobbyConfig config;
    if (optional<string> error = LobbyDesignData::ParseLobby(document, OUT config))
        return error;

    // DB의 Account.CharacterSlotCount가 TINYINT라 255를 넘으면 계정을 만들 때 저장하지 못한다. 1 이상인지는 DataBuilder가 검사한다.
    if (config.DefaultCharacterSlotCount > UINT8_MAX)
        return format("{}: 'DefaultCharacterSlotCount'가 {} 이하여야 한다: {}", LobbyDesignData::LOBBY_FILE, UINT8_MAX, config.DefaultCharacterSlotCount);

    Install(std::move(config));
    return nullopt;
}

void LobbyData::Install(LobbyDesignData::LobbyConfig config)
{
    s_config = std::move(config);
}

bool LobbyData::IsCreatableClass(int32 classId)
{
    return ranges::find(s_config.CreatableClasses, classId) != s_config.CreatableClasses.end();
}
