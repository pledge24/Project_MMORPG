#pragma once

#include "CoreMinimal.h"
#include "Network/ClientPacketHandler.h"

/**
 * 게임 코드가 패킷을 보내는 창구다. 패킷 자료형만 받고, 어느 세션으로 어떻게 가는지는 감춘다.
 * 게임 도메인이 Network/에서 부르는 헤더는 이것 하나뿐이다(docs/folder-structure.md 3.3).
 */
class P1_API FP1PacketSender
{
public:
    /** WorldContext가 속한 게임 인스턴스의 세션으로 보낸다. 연결이 없으면 경고만 남기고 버린다. */
    static void Send(const UObject* WorldContext, SendBufferRef SendBuffer);

    template <typename T>
    static void Send(const UObject* WorldContext, T& Pkt)
    {
        Send(WorldContext, ClientPacketHandler::MakeSerializedPacket(Pkt));
    }
};
