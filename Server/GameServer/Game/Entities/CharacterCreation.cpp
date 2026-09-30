#include "pch.h"
#include "CharacterCreation.h"
#include "EncodingConverter.h"

optional<string> CharacterCreation::Validate(const Protocol::CharacterOverview& character)
{
    // 전역 표는 여러 스레드가 함께 읽는다. operator[]는 없는 키를 끼워 넣으므로 find로만 조회한다.
    // NONE은 매핑에 있지만 빈 표를 가리키므로 비어 있는지까지 본다.
    auto classIt = Gamedata::s_classLevelDataTableMappings.find(character.class_());
    if (character.class_() == Protocol::CLASS_TYPE_NONE
        || classIt == Gamedata::s_classLevelDataTableMappings.end()
        || classIt->second == nullptr
        || classIt->second->empty())
    {
        return string("선택할 수 없는 직업입니다.");
    }

    // UTF-8로 변환할 수 없는 이름은 빈 문자열이 되어 여기서 걸린다.
    const wstring name = EncodingConverter::StringToWString(character.name());
    if (name.empty() || name.size() > MAX_NAME_LENGTH)
        return string("캐릭터 이름은 1~50자여야 합니다.");

    return nullopt;
}
