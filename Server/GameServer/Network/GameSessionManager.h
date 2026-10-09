#pragma once

class GameSession;

using GameSessionRef = shared_ptr<GameSession>;

/**
 * 계정마다 현재 세션 하나를 기록한다. 같은 계정의 로그인이 다시 오면 나중에 온 쪽이 이긴다.
 * 로그인 잡이 랜덤 DB 큐에서 돌아 같은 계정의 로그인이 동시에 올 수 있으므로, 확인과 교체를 락 하나 안에서 한다.
 * 접속한 세션 전체는 ServerCore의 Service가 들고 있다. 이 클래스는 로그인한 세션만 안다.
 */
class GameSessionManager
{
public:
    /**
     * [LOCK] 세션을 계정에 묶고 session의 계정 번호를 userId로 바꾼다.
     * 같은 계정의 기존 세션이 있으면 교체하고 그 세션을 돌려준다. 없으면 nullptr를 돌려준다.
     * 돌려받은 세션을 끊는 것은 호출자의 몫이다.
     * 이 세션이 다른 계정에 묶여 있었으면 그 등록을 거둔다.
     */
    GameSessionRef RegisterUser(int64 userId, GameSessionRef session);
    /** [LOCK] 계정 등록이 이 세션을 가리킬 때만 그 등록을 지운다. 새 로그인에 밀려난 세션이면 아무것도 하지 않는다. */
    void UnregisterUser(GameSessionRef session);

private:
    /** UnregisterUser의 본문. 락을 잡은 상태에서만 부른다. */
    void EraseRegistration(const GameSessionRef& session);

    MAKE_LOCK;
    map<int64, GameSessionRef> _userSessions;   // userId → 그 계정의 현재 세션
};

extern GameSessionManager GSessionManager;
