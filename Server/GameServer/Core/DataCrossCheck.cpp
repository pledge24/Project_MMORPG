#include "Core/pch.h"
#include "Core/DataCrossCheck.h"
#include "Lobby/Data/LobbyData.h"

optional<string> DataCrossCheck::Check()
{
    // 능력치 표에 없는 직업의 캐릭터는 만들 수는 있어도 첫 게임 입장에서 HP와 MP를 정하지 못해 들어가지 못한다.
    for (Protocol::CharacterClass characterClass : LobbyData::GetCreatableClasses())
    {
        if (Gamedata::HasClassLevelStats(characterClass) == false)
            return format("로비가 생성할 수 있는 직업 {}이 게임의 직업별 능력치 표에 없다", Protocol::CharacterClass_Name(characterClass));
    }

    return nullopt;
}
