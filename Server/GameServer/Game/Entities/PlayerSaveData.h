#pragma once
#include "Game/Entities/PlayerProgress.h"

/**
 * 접속 종료 시 DB에 저장할 진행의 사본. 진행에 계정 번호와 바뀐 슬롯 표시를 더한다.
 * Player::MakeSaveData로 생성되며, 생성된 사본은 DBQueue로 전달된다.
 */
struct PlayerSaveData
{
    int64 userId = 0;

    PlayerProgress progress;

    /** dirty flag는 가방이 없으면 비워 둔다. 저장 함수가 그 경우를 실패로 처리한다. */
    optional<vector<bool>> gearDirtyFlags;
    optional<vector<bool>> consumableDirtyFlags;
    optional<vector<bool>> miscDirtyFlags;
    map<int32, bool> equippedGearDirtyFlags;
};
