#pragma once

/**
 * 게임이 로비를 부르는 로비 복귀 창구.
 *
 * 진행 저장을 마친 세션을 로비 단계로 받고, 저장 뒤의 캐릭터 목록을 보낸다.
 * 게임은 이 창구로만 로비를 부른다.
 */
namespace LobbyReturn
{
    /**
     * 로비 복귀 창구. 게임이 진행 저장을 마친 뒤 이 계정의 DB 큐 잡 안에서 부른다.
     * 그사이 연결이 끊겼으면 아무것도 하지 않는다. 결과는 Finish로 알린다.
     */
    void Accept(const GameSessionRef& session, int64 userId);

    /**
     * 목록이 있으면 세션을 로비로 바꾸고 S_RETURN_TO_LOBBY와 목록을 차례로 보낸다.
     * 목록을 불러오지 못했으면(nullopt) 실패 결과를 알린 뒤 연결을 끊는다.
     */
    void Finish(const GameSessionRef& session, const optional<Protocol::S_CHARACTER_LIST>& characterListPkt);
}
