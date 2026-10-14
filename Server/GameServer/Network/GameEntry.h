#pragma once

struct PlayerProgress;

/**
 * 캐릭터를 골라 게임에 들어가는 입장 처리.
 *
 * 저장된 진행으로 플레이어를 만들어 세션에 입장시킨다. 입장에 실패하면 세션에 플레이어를 남기지 않는다.
 * 룸 입장은 맡지 않는다. 룸 입장은 클라이언트의 다음 요청이 시작한다.
 */
namespace GameEntry
{
    /**
     * DB 큐의 잡 안에서 부른다. characterId의 진행으로 session을 입장시키고 클라이언트에 보낼 응답을 돌려준다.
     * userId는 세션에서 다시 읽지 말고 핸들러가 읽은 값을 넘긴다.
     */
    Protocol::S_ENTER_GAME Enter(DBConnection& conn, const GameSessionRef& session, int64 userId, int64 characterId);

    /** progress로 플레이어를 만들어 session에 등록한다. 검증에 실패하거나 이미 플레이어가 있으면 세션을 건드리지 않고 nullptr. */
    PlayerRef SpawnPlayer(const GameSessionRef& session, int64 userId, const PlayerProgress& progress);
}
