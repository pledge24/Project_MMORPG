#pragma once
#include "Game/Entities/PlayerProgress.h"

/** 접속 종료 때 DB에 저장할 진행의 사본. 아이템은 지금 들고 있는 것 전체를 저장한다. */
struct PlayerSaveData
{
    int64 userId = 0;

    PlayerProgress progress;
};
