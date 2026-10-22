#include "Core/pch.h"
#include "Core/DataCrossCheck.h"
#include "Lobby/Data/LobbyData.h"

optional<string> DataCrossCheck::Check()
{
    // 레벨 표가 없는 직업의 캐릭터는 만들 수는 있어도 첫 게임 입장에서 HP와 MP를 정하지 못해 들어가지 못한다.
    for (int32 classId : LobbyData::GetCreatableClasses())
    {
        if (Gamedata::FindClassLevelTable(classId) == nullptr)
            return format("로비가 생성할 수 있는 직업 {}에 게임의 레벨 표가 없다", classId);
    }

    return nullopt;
}
