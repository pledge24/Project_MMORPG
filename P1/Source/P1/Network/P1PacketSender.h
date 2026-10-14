#pragma once

#include "CoreMinimal.h"
#include "Network/ClientPacketHandler.h"

/**
 * 게임 코드가 패킷을 보내는 창구.
 *
 * 패킷 자료형만 받고, 어느 연결로 어떻게 보내는지는 감춘다.
 * 게임 코드가 네트워크 계층에서 부르는 것은 이 창구 하나뿐이다. 게임 스레드 전용.
 */
class P1_API FP1PacketSender
{
public:
    /** WorldContext가 속한 게임 인스턴스의 연결로 보낸다. 연결이 없으면 버린다. */
    static void Send(const UObject* WorldContext, SendBufferRef SendBuffer);

    template <typename T>
    static void Send(const UObject* WorldContext, T& Pkt)
    {
        Send(WorldContext, ClientPacketHandler::MakeSerializedPacket(Pkt));
    }
};
