#pragma once

struct PlayerProgress;

/**
 * 게임 입장. 진행을 불러오고, 그 사본으로 플레이어를 만들어 검증한 뒤 세션에 등록한다.
 * 불러오기나 검증이 실패하면 세션에 플레이어를 남기지 않는다.
 * DB 큐의 잡 안에서 부른다. 룸에는 넣지 않는다. 룸 입장은 클라이언트의 다음 요청이 시작한다.
 */
namespace GameEntry
{
    /**
     * conn으로 userId 계정의 characterId 진행을 불러와 session에 입장시키고, 클라이언트에 보낼 응답을 돌려준다.
     * userId는 핸들러가 읽은 값이다. 세션에서 다시 읽지 않는다.
     */
    Protocol::S_ENTER_GAME Enter(DBConnection& conn, const GameSessionRef& session, int64 userId, int64 characterId);

    /**
     * progress로 플레이어를 만들어 검증하고 session에 등록한다.
     * 검증에 실패하거나 세션에 이미 플레이어가 있으면 세션을 건드리지 않고 nullptr를 돌려준다.
     */
    PlayerRef SpawnPlayer(const GameSessionRef& session, int64 userId, const PlayerProgress& progress);
}
