#pragma once

#include "CoreMinimal.h"
#include "Network/SendBuffer.h"
#include "Utils/Types.h"

class UP1GameInstance;

class P1_API PacketSession : public TSharedFromThis<PacketSession>
{
public:
    PacketSession(class FSocket* Socket, UP1GameInstance* InGameInstance);
    ~PacketSession();

    /** 이 세션을 연 게임 인스턴스다. 패킷 핸들러는 전역 월드 대신 여기서 게임 인스턴스를 얻는다. */
    UP1GameInstance* GetGameInstance() const;

private:
    TWeakObjectPtr<UP1GameInstance> GameInstance;

    //~ Session Lifecycle
public:
    void Run();
    void Disconnect();

    class FSocket* Socket;

    FP1RecvWorkerRef RecvWorkerThread;
    FP1SendWorkerRef SendWorkerThread;

    //~ Connection Loss
public:
    /** 수신 워커가 연결이 끊긴 것을 알게 되면 부른다. 게임 스레드에 알리는 유일한 경로다. */
    void MarkConnectionLost();

    /** 게임 스레드가 읽는다. 수신 워커는 마지막으로 받은 패킷을 큐에 넣은 뒤에 표시를 세운다. */
    bool IsConnectionLost() const;

private:
    std::atomic<bool> bConnectionLost = false;

    //~ Packet Queues
public:
    /** 게임 스레드에서 부른다. 수신 큐를 비우면서 핸들러를 돌린다. */
    void HandleRecvPackets();

    void SendPacket(SendBufferRef SendBuffer);

    // 게임 스레드와 네트워크 스레드가 주고받을 때 쓰는 패킷 저장 큐
    TQueue<TArray<uint8>> RecvPacketQueue;
    TQueue<SendBufferRef> SendPacketQueue;
};
