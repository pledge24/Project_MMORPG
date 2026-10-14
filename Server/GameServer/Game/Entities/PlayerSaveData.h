#pragma once
#include "Game/Entities/PlayerProgress.h"

/** 접속 종료 때 DB에 저장할 진행의 사본. */
struct PlayerSaveData
{
    int64 userId = 0;

    PlayerProgress progress;

    /** 가방이 없으면 비워 둔다. 저장 함수는 비어 있으면 실패로 처리한다. */
    optional<vector<bool>> gearDirtyFlags;
    optional<vector<bool>> consumableDirtyFlags;
    optional<vector<bool>> miscDirtyFlags;
    map<int32, bool> equippedGearDirtyFlags;
};
