#include "Core/pch.h"
#include "Game/Progress/InitialProgress.h"
#include "Game/Entities/PlayerProgress.h"

bool InitialProgress::Fill(OUT PlayerProgress& progress)
{
    const ClassLevelTable* classLevelTable = Gamedata::FindClassLevelTable(progress.classId);
    const LevelTemplate* levelTemplate = classLevelTable != nullptr ? classLevelTable->Find(1) : nullptr;
    if (levelTemplate == nullptr)
        return false;

    // 새 캐릭터는 시작 맵의 시작 위치에서, HP와 MP가 가득 찬 채로, 시작 골드를 들고 시작한다.
    const InitialProgressTemplate& initial = Gamedata::GetInitialProgress();
    progress.mapId = initial.startMapId;
    progress.pos = vector3D(initial.startPos.x, initial.startPos.y, initial.startPos.z);
    progress.yaw = initial.startYaw;
    progress.totalExp = 0;
    progress.hp = levelTemplate->maxHp;
    progress.mp = levelTemplate->maxMp;
    progress.gold = initial.startGold;

    return true;
}
