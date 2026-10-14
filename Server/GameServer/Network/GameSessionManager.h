#pragma once

class GameSession;

using GameSessionRef = shared_ptr<GameSession>;

/**
 * 로그인한 계정과 그 계정의 현재 세션을 잇는 등록부.
 *
 * 한 계정이 세션 하나만 갖게 한다. 같은 계정의 로그인이 다시 오면 나중에 온 쪽이 이긴다.
 */
class GameSessionManager
{
public:
    /**
     * [LOCK] session을 userId 계정에 묶는다. 같은 계정의 기존 세션이 있으면 교체하고 그 세션을, 없으면 nullptr.
     * 돌려받은 세션을 끊는 것은 호출자의 몫이다.
     */
    GameSessionRef RegisterUser(int64 userId, GameSessionRef session);
    /** [LOCK] 이 세션의 계정 등록을 지운다. 새 로그인에 밀려난 세션이면 아무것도 하지 않는다. */
    void UnregisterUser(GameSessionRef session);

private:
    /** 락을 잡은 상태에서만 부른다. */
    void EraseRegistration(const GameSessionRef& session);

    MAKE_LOCK;
    map<int64, GameSessionRef> _userSessions;   // userId → 그 계정의 현재 세션
};
