#include "Core/pch.h"
#include "Lobby/LobbyReturn.h"
#include "Lobby/LobbyRequests.h"
#include "DB/DAOCommon.h"

void LobbyReturn::Accept(const GameSessionRef& session, int64 userId)
{
    // 복귀 중에 끊겼다. 저장은 이미 끝났고 받을 세션이 없다.
    if (session->IsConnected() == false)
        return;

    optional<Protocol::S_CHARACTER_LIST> characterListPkt;
    try
    {
        DBConnectionGuard conn;
        characterListPkt = LobbyRequests::ReturnToLobby(*conn, userId);
    }
    catch (const exception& error)
    {
        GLogger->Error("계정 {} 로비 복귀 캐릭터 목록 불러오기 실패: {}", userId, error.what());
    }

    Finish(session, characterListPkt);
}

void LobbyReturn::Finish(const GameSessionRef& session, const optional<Protocol::S_CHARACTER_LIST>& characterListPkt)
{
    Protocol::S_RETURN_TO_LOBBY returnToLobbyPkt;

    // 플레이어는 이미 룸에서 빠졌으므로 게임으로 되돌릴 수 없다. 목록 없이 로비로 받으면 클라이언트가 목록을 기다리며 멈춘다.
    if (characterListPkt.has_value() == false)
    {
        returnToLobbyPkt.set_result(Protocol::ENTRY_RESULT_INTERNAL_ERROR);
        SendPacketAndDisconnect(session, returnToLobbyPkt, "로비 복귀 캐릭터 목록 불러오기 실패");
        return;
    }

    // 응답을 보낸 뒤에 바꾸면 목록을 받은 클라이언트의 첫 로비 요청이 로비 복귀 중 단계에서 버려진다.
    session->CompleteLobbyReturn();

    returnToLobbyPkt.set_result(Protocol::ENTRY_RESULT_SUCCESS);
    SendPacket(session, returnToLobbyPkt);
    SendPacket(session, characterListPkt.value());
}
