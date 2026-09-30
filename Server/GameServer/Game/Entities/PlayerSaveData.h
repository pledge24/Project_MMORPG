#pragma once

/*--------------------------------------------------------------
    PlayerSaveData

    접속 종료 때 DB에 저장할 플레이어 상태의 사본이다.
    룸 큐 위에서 Player::MakeSaveData로 만들고 DB 큐로 넘긴다.
    DB 스레드가 살아 있는 Player를 읽으면 룸 스레드의 변경과 경쟁하므로 사본만 읽게 한다.

    dirty flag는 가방이 없으면 비워 둔다. 저장 함수가 그 경우를 실패로 처리한다.
---------------------------------------------------------------*/

struct PlayerSaveData
{
    int64 userId = 0;

    Protocol::PlayerInfo playerInfo;
    Protocol::PosInfo posInfo;
    Protocol::StatInfo statInfo;
    Protocol::Possession possession;

    optional<vector<bool>> gearDirtyFlags;
    optional<vector<bool>> consumableDirtyFlags;
    optional<vector<bool>> miscDirtyFlags;
    map<int32, bool> equippedGearDirtyFlags;
};
