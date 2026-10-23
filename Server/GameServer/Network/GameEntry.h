#pragma once

struct PlayerProgress;

/**
 * 캐릭터를 골라 게임에 들어가는 입장 처리이자, 로비가 게임을 부르는 게임 입장 창구.
 *
 * 저장된 진행으로 플레이어를 만들어 세션에 입장시킨다. 입장에 실패하면 세션에 플레이어를 남기지 않는다.
 * 룸 입장은 맡지 않는다. 클라이언트가 맵을 다 열었다고 알리면 들어갈 룸이 입장시킨다.
 */
namespace GameEntry
{
    /**
     * 게임 입장 창구. 로비가 캐릭터의 소유를 확인한 뒤 게임 입장 중 단계의 session으로 부른다. 어느 스레드에서 불러도 된다.
     * 저장 대기가 끝나면 진행을 불러와 입장시키고, 결과를 FinishEnter로 알린다.
     */
    void RequestEnter(const GameSessionRef& session, int64 userId, int64 characterId);

    /** 입장 결과로 세션 단계를 게임이나 로비로 바꾸고 응답을 보낸다. 단계를 먼저 바꾸므로 응답 뒤에 온 요청은 새 단계로 받는다. */
    void FinishEnter(const GameSessionRef& session, const Protocol::S_ENTER_GAME& enterGamePkt);

    /**
     * DB 큐의 잡 안에서 부른다. characterId의 진행과 그 맵과 좌표로 고른 룸으로 session을 입장시키고 응답을 돌려준다.
     * userId는 세션에서 다시 읽지 말고 핸들러가 읽은 값을 넘긴다.
     */
    Protocol::S_ENTER_GAME Enter(DBConnection& conn, const GameSessionRef& session, int64 userId, int64 characterId);

    /**
     * progress로 플레이어를 만들어 enterRoom의 enterPos에서 맵 로드를 기다리게 하고 session에 등록한다.
     * 검증에 실패하거나 이미 플레이어가 있으면 세션을 건드리지 않고 nullptr.
     */
    PlayerRef SpawnPlayer(const GameSessionRef& session, int64 userId, const PlayerProgress& progress, const RoomRef& enterRoom, const Protocol::PosInfo& enterPos);
}
