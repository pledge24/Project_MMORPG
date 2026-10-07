#include "Core/pch.h"
#include "Game/Entities/CharacterCreation.h"
#include "Utils/EncodingConverter.h"

optional<string> CharacterCreation::Validate(const Protocol::CharacterOverview& character)
{
    // NONE은 매핑에 있고 지금은 빈 표를 가리킨다. 표가 비어 있는지만 보면 누가 그 표를 채우는 순간
    // NONE이 통과하므로 따로 막는다. 빈 표는 데이터가 아직 없는 직업이다.
    const DataTable* classLevelTable = Gamedata::FindClassLevelTable(character.class_());
    if (character.class_() == Protocol::CLASS_TYPE_NONE
        || classLevelTable == nullptr
        || classLevelTable->empty())
    {
        return string("선택할 수 없는 직업입니다.");
    }

    // 생성 쿼리가 쓰는 것과 같은 변환기로 센다. 잘못된 UTF-8 바이트는 U+FFFD로 바뀌어 한 글자로 세고,
    // 이름 안의 NUL 뒤는 잘린다. DB에 들어가는 이름도 같은 결과다.
    const wstring name = EncodingConverter::StringToWString(character.name());
    if (name.empty() || name.size() > MAX_NAME_LENGTH)
        return "캐릭터 이름은 1~" + to_string(MAX_NAME_LENGTH) + "자여야 합니다.";

    return nullopt;
}
