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
