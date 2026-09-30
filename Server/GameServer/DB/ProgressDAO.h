#pragma once

struct PlayerSaveData;

/*-------------------------
    ProgressDAO

    플레이어의 진행을 불러오고 저장한다. 캐릭터 상태와 아이템의 DAO를 차례로 부른다.
--------------------------*/

class ProgressDAO
{
public:
    // 입장할 때 부른다. 결과로 S_ENTER_GAME을 보낸다.
    static void Load(SessionRef session, int64 characterId);

    // 접속이 끊길 때 룸 큐에서 뜬 사본으로 저장한다.
    static void Save(const PlayerSaveData& data);
};
