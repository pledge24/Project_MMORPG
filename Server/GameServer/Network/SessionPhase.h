#pragma once

/** 게임 서버와의 연결이 지금 어느 단계에 있는지. 단계마다 받는 패킷이 다르다. */
enum class SessionPhase : uint8
{
    BEFORE_LOBBY,       // 연결했지만 로비 접속을 마치지 않았다
    LOBBY,              // 로비에서 캐릭터를 다룬다
    ENTERING_GAME,      // 고른 캐릭터로 게임에 입장하는 중이다. 입장이 끝나거나 거절될 때까지 로비 요청을 받지 않는다
    IN_GAME,            // 게임에 입장했다
};

/** 로그에 쓸 단계 이름. */
inline string_view ToString(SessionPhase phase)
{
    switch (phase)
    {
    case SessionPhase::BEFORE_LOBBY: return "로비 접속 전";
    case SessionPhase::LOBBY: return "로비";
    case SessionPhase::ENTERING_GAME: return "게임 입장 중";
    case SessionPhase::IN_GAME: return "게임";
    }

    return "알 수 없음";
}

/**
 * 받는 패킷마다 그 패킷을 받는 세션 단계를 정한 표.
 *
 * 세션은 패킷을 핸들러에 넘기기 전에 이 표로 단계를 확인하고, 맞지 않는 패킷은 버린다.
 * 받는 패킷을 더하면 이 표에도 더한다.
 */
namespace SessionPhaseTable
{
    /** phase 단계의 세션이 packetId 패킷을 받으면 true. 표에 없는 패킷은 어느 단계에서도 false. 어느 스레드에서 불러도 된다. */
    bool Accepts(uint16 packetId, SessionPhase phase);
}
