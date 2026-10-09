#include "Core/pch.h"
#include "Game/Characters/CharacterCreation.h"
#include "Utils/EncodingConverter.h"

optional<string> CharacterCreation::Validate(const Protocol::CharacterOverview& character)
{
    // 레벨 표가 없는 직업은 데이터가 아직 없는 직업이다. NONE은 직업이 아니므로 누가 표를 넣어도 막는다.
    if (character.class_() == Protocol::CLASS_TYPE_NONE
        || Gamedata::FindClassLevelTable(character.class_()) == nullptr)
    {
        return string("선택할 수 없는 직업입니다.");
    }

    // 생성 쿼리가 쓰는 것과 같은 변환기로 센다. 잘못된 UTF-8 바이트는 U+FFFD로 바뀌어 한 글자로 세고,
    // 이름 안의 NUL 뒤는 잘린다. DB에 들어가는 이름도 같은 결과다.
    const wstring name = EncodingConverter::StringToWString(character.name());
    if (name.empty() || name.size() > MAX_NAME_LENGTH)
        return "캐릭터 이름은 " + to_string(MAX_NAME_LENGTH) + "이하여야 합니다.";

    return nullopt;
}
