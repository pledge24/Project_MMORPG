#include "Core/pch.h"
#include "Lobby/Data/LobbyData.h"
#include <fstream>

namespace
{
    /**
     * 작업 디렉터리(프로젝트 폴더) 기준 상대 경로다.
     * GenJsonFile.bat이 Json을 옮기는 곳과 다르면 부팅 때 로비 표를 찾지 못한다.
     */
    constexpr string_view LOBBY_FILE_PATH = "Lobby/Data/Json/S_Lobby.json";
    constexpr string_view LOBBY_FILE_NAME = "S_Lobby.json";

    constexpr string_view DEFAULT_CHARACTER_SLOT_COUNT = "defaultCharacterSlotCount";
    constexpr string_view CREATABLE_CLASSES = "creatableClasses";

    /** 기획 원본은 소문자 직업 이름(warrior)을 적는다. 게임의 착용 조건과 같은 표기다. */
    const unordered_map<string_view, Protocol::CharacterClass> CLASS_BY_NAME = {
        { "warrior", Protocol::CLASS_TYPE_WARRIOR },
        { "mage", Protocol::CLASS_TYPE_MAGE },
    };

    /** 행 하나를 검증해 tables에 쓴다. 틀리면 사유. */
    optional<string> ParseRow(const Json& row, OUT LobbyTables& tables)
    {
        if (row.is_object() == false)
            return "행이 객체가 아니다";

        // DB의 Account.CharacterSlotCount가 TINYINT라 255를 넘으면 계정을 만들 때 저장하지 못한다.
        auto slotIt = row.find(DEFAULT_CHARACTER_SLOT_COUNT);
        if (slotIt == row.end() || slotIt->is_number_integer() == false)
            return format("'{}'가 없거나 정수가 아니다", DEFAULT_CHARACTER_SLOT_COUNT);

        const int64 slotCount = slotIt->get<int64>();
        if (slotCount < 1 || slotCount > UINT8_MAX)
            return format("'{}'가 1 이상 {} 이하여야 한다: {}", DEFAULT_CHARACTER_SLOT_COUNT, UINT8_MAX, slotCount);

        tables.defaultCharacterSlotCount = static_cast<int32>(slotCount);

        auto classesIt = row.find(CREATABLE_CLASSES);
        if (classesIt == row.end() || classesIt->is_array() == false || classesIt->empty())
            return format("'{}'가 없거나 비었거나 배열이 아니다", CREATABLE_CLASSES);

        for (const Json& value : *classesIt)
        {
            if (value.is_string() == false)
                return format("'{}'의 값이 문자열이 아니다: {}", CREATABLE_CLASSES, value.dump());

            const string name = value.get<string>();
            auto classIt = CLASS_BY_NAME.find(name);
            if (classIt == CLASS_BY_NAME.end())
                return format("'{}'의 \"{}\"는 직업이 아니다", CREATABLE_CLASSES, name);

            const int32 classId = classIt->second;
            if (ranges::find(tables.creatableClasses, classId) != tables.creatableClasses.end())
                return format("'{}'에 \"{}\"가 두 번 있다", CREATABLE_CLASSES, name);

            tables.creatableClasses.push_back(classId);
        }

        return nullopt;
    }
}

LobbyTables LobbyData::s_tables;

bool LobbyData::LoadAll()
{
    ifstream file{ string(LOBBY_FILE_PATH) };
    if (file.is_open() == false)
    {
        GLogger->Error("로비 기획 데이터를 불러오지 못했다. {}: 파일을 열지 못했다", LOBBY_FILE_NAME);
        return false;
    }

    Json document;
    try
    {
        document = Json::parse(file);
    }
    catch (const Json::parse_error& e)
    {
        GLogger->Error("로비 기획 데이터를 불러오지 못했다. {}: Json 문법 오류(byte {}): {}", LOBBY_FILE_NAME, e.byte, e.what());
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
    // 로비 설정은 하나로 정해져야 한다.
    if (document.is_array() == false || document.size() != 1)
        return format("{}: 행이 하나인 배열이어야 한다", LOBBY_FILE_NAME);

    LobbyTables tables;
    if (optional<string> error = ParseRow(document[0], OUT tables))
        return format("{} 1번째 행: {}", LOBBY_FILE_NAME, error.value());

    Install(std::move(tables));
    return nullopt;
}

void LobbyData::Install(LobbyTables tables)
{
    s_tables = std::move(tables);
}

bool LobbyData::IsCreatableClass(int32 classId)
{
    return ranges::find(s_tables.creatableClasses, classId) != s_tables.creatableClasses.end();
}
