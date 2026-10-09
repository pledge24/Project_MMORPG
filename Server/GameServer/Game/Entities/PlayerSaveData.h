#pragma once

/**
 * 접속 종료 시 DB에 저장할 플레이어 상태의 사본.
 * Player::MakeSaveData로 생성되며,생성된 사본은 DBQueue로 전달된다.
 */
struct PlayerSaveData
{
    int64 userId = 0;

    Protocol::PlayerInfo playerInfo;
    Protocol::PosInfo posInfo;
    Protocol::StatInfo statInfo;
    Protocol::Possession possession;

    /** dirty flag는 가방이 없으면 비워 둔다. 저장 함수가 그 경우를 실패로 처리한다. */
    optional<vector<bool>> gearDirtyFlags;
    optional<vector<bool>> consumableDirtyFlags;
    optional<vector<bool>> miscDirtyFlags;
    map<int32, bool> equippedGearDirtyFlags;
};
