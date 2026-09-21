#pragma once

#include "CoreMinimal.h"
#include "Network/SendBuffer.h"
#include "Utils/Types.h"

class P1_API PacketSession : public TSharedFromThis<PacketSession>
{
public:
    PacketSession(class FSocket* Socket);
    ~PacketSession();

    //~ Session Lifecycle
public:
    void Run();
    void Disconnect();

    class FSocket* Socket;

    FP1RecvWorkerRef RecvWorkerThread;
    FP1SendWorkerRef SendWorkerThread;

    //~ Packet Queues
public:
    /** 게임 스레드에서 부른다. 수신 큐를 비우면서 핸들러를 돌린다. */
    UFUNCTION(BlueprintCallable)
    void HandleRecvPackets();

    void SendPacket(SendBufferRef SendBuffer);

    // 게임 스레드와 네트워크 스레드가 주고받을 때 쓰는 패킷 저장 큐
    TQueue<TArray<uint8>> RecvPacketQueue;
    TQueue<SendBufferRef> SendPacketQueue;
};
