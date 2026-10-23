#pragma once

/**
 * 클라이언트의 채팅 요청 처리.
 *
 * 메시지를 검사해 말한 플레이어에게 결과를 보내고, 받아들인 메시지를 룸에 중계한다.
 */
namespace ChatRequests
{
    /** 메시지 최대 길이(UTF-16 기준). */
    constexpr int32 MAX_MESSAGE_LENGTH = 100;

    /** 클라이언트가 보낸 메시지를 검사한다. 통과하면 nullopt, 거절하면 그 결과 값. */
    optional<Protocol::ChatResult> Validate(const string& message);

    /** 받아들이면 말한 플레이어를 포함한 룸의 모든 플레이어에게 메시지를 알린다. room의 큐 위에서 부른다. */
    void HandleChat(Room& room, const PlayerRef& player, const Protocol::C_CHAT& pkt);
}
