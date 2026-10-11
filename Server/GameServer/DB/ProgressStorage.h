#pragma once

struct PlayerProgress;
struct PlayerSaveData;

/**
 * 플레이어의 진행을 불러오고 저장한다. SQL은 들고 있지 않고, 캐릭터 상태와 아이템의 DAO를 차례로 부른다.
 * 세션과 패킷을 모른다. 살아 있는 Player도 읽거나 쓰지 않고 진행 사본만 다룬다.
 * 두 함수 모두 userId로 고른 DB 큐의 잡 안에서 부른다. 그래야 같은 계정의 저장과 불러오기가 순서대로 돈다.
 */
class ProgressStorage
{
public:
    /**
     * 입장할 때 부른다. userId 계정의 캐릭터 characterId의 진행을 progress에 채운다.
     * 그 계정의 캐릭터가 아니거나 DB 작업이 실패하면(표준 예외 전체) 사유를 로그에 남기고 false를 돌려준다.
     * 그때 progress는 일부만 채워져 있으므로 쓰지 않는다.
     */
    static bool Load(DBConnection& conn, int64 userId, int64 characterId, OUT PlayerProgress& progress);

    /**
     * 접속이 끊길 때 룸 큐에서 뜬 사본으로 저장한다. 세 단계를 트랜잭션 하나로 묶는다.
     * 한 단계라도 실패하면 앞 단계까지 되돌리고 false를 돌려준다. 다시 시도하지 않는다.
     */
    static bool Save(DBConnection& conn, const PlayerSaveData& data);
};
