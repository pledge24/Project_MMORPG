#pragma once

/**
 * pkt를 직렬화해 session에 보낸다. session이 nullptr이면(끊겨 사라진 세션) 보내지 않고 false.
 * 어느 스레드에서 불러도 된다.
 */
template<typename SessionType, typename PacketType>
bool SendPacket(const shared_ptr<SessionType>& session, const PacketType& pkt)
{
    if (session == nullptr)
        return false;

    // 생성된 MakeSerializedPacket은 비 const 참조를 받지만 패킷을 바꾸지 않는다.
    session->Send(ServerPacketHandler::MakeSerializedPacket(const_cast<PacketType&>(pkt)));
    return true;
}

/** 사유 패킷을 보낼 때까지 끊기를 미루는 상한(ms). 넘기면 상대가 받지 않는 것으로 보고 끊는다. */
constexpr uint64 DISCONNECT_SEND_TIMEOUT_MS = 1000;

/**
 * pkt를 보낸 뒤 session을 끊는다. 클라이언트가 끊긴 이유를 알도록 사유를 먼저 보낼 때 쓴다.
 * 어느 스레드에서 불러도 된다. 이미 끊긴 세션이면 아무것도 하지 않는다.
 */
template<typename SessionType, typename PacketType>
void SendPacketAndDisconnect(const shared_ptr<SessionType>& session, const PacketType& pkt, const char* cause)
{
    SendPacket(session, pkt);
    session->DisconnectAfterSend(cause);

    // 받기만 하고 소켓을 비우지 않는 상대면 송신이 끝나지 않는다. 이미 끊겼으면 Disconnect는 아무것도 하지 않는다.
    GSessionJobQueue->DoTimer(DISCONNECT_SEND_TIMEOUT_MS, [session, cause]()
        {
            session->Disconnect(cause);
        });
}
