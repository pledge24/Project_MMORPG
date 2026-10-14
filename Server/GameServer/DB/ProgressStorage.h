#pragma once

struct PlayerProgress;
struct PlayerSaveData;

/**
 * 플레이어 진행의 저장소.
 *
 * 입장할 때 캐릭터의 진행을 불러오고, 접속이 끊길 때 진행을 저장한다.
 * 살아 있는 Player가 아니라 진행 사본만 다룬다.
 * 같은 계정의 저장과 불러오기가 순서대로 돌도록 userId로 고른 DB 큐의 잡 안에서 부른다.
 */
class ProgressStorage
{
public:
    /**
     * userId 계정의 캐릭터 characterId의 진행을 progress에 채운다. 예외를 던지지 않는다.
     * 그 계정의 캐릭터가 아니거나 실패하면 false이고, 그때 progress는 일부만 채워져 있으므로 쓰지 않는다.
     */
    static bool Load(DBConnection& conn, int64 userId, int64 characterId, OUT PlayerProgress& progress);

    /** 진행을 트랜잭션 하나로 저장한다. 실패하면 모두 되돌리고 false. 다시 시도하지 않는다. */
    static bool Save(DBConnection& conn, const PlayerSaveData& data);
};
