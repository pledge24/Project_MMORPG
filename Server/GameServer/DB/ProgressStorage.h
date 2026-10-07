#pragma once

struct PlayerSaveData;

/**
 * 플레이어의 진행을 불러오고 저장한다. SQL은 들고 있지 않고, 캐릭터 상태와 아이템의 DAO를 차례로 부른다.
 * 두 함수 모두 userId로 고른 DB 큐의 잡 안에서 부른다. 그래야 같은 계정의 저장과 불러오기가 순서대로 돈다.
 */
class ProgressStorage
{
public:
    /**
     * 입장할 때 부른다. 세션에 Player를 만들어 둔 뒤에 불러야 한다.
     * 결과로 S_ENTER_GAME을 보낸다. 중간에 실패하면 success=false를 보내고 그 뒤 단계는 건너뛴다.
     */
    static void Load(SessionRef session, int64 characterId);

    /** 접속이 끊길 때 룸 큐에서 뜬 사본으로 저장한다. 한 단계가 실패하면 뒤 단계는 저장하지 않고, 다시 시도하지 않는다. */
    static void Save(const PlayerSaveData& data);
};
