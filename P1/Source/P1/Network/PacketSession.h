#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "Network/SendBuffer.h"
#include "Utils/Types.h"

class UP1ConnectionSubsystem;

/**
 * 게임 서버와 맺은 연결 하나의 패킷 세션.
 *
 * 송수신 스레드를 두고, 게임 스레드와 네트워크 스레드가 패킷을 주고받는 큐를 가진다.
 */
class P1_API PacketSession : public TSharedFromThis<PacketSession>
{
public:
    PacketSession(class FSocket* Socket, UP1ConnectionSubsystem* InConnection);
    ~PacketSession();

    /** 이 세션을 연 연결 서브시스템. 사라졌으면 nullptr. 게임 스레드 전용. */
    UP1ConnectionSubsystem* GetConnection() const;

    /** 이 세션을 연 게임 인스턴스의 월드. 연결 서브시스템이 사라졌거나 월드가 없으면 nullptr. 게임 스레드 전용. */
    UWorld* GetWorld() const;

private:
    TWeakObjectPtr<UP1ConnectionSubsystem> Connection;

    //~ Session Lifecycle
public:
    /** 송수신 스레드를 시작한다. */
    void Run();
    /** 송수신 스레드를 멈추고 소켓을 닫는다. 두 스레드가 끝날 때까지 기다린다. */
    void Disconnect();

    /** 연결 서브시스템이 소유한다. Disconnect 뒤에는 nullptr. */
    class FSocket* Socket;

    FP1RecvWorkerRef RecvWorkerThread;
    FP1SendWorkerRef SendWorkerThread;

    //~ Connection Loss
public:
    /** 수신 워커가 끊김을 알아채면 부른다. 게임 스레드에 끊김을 알리는 유일한 경로다. */
    void MarkConnectionLost();

    /** 게임 스레드가 읽는다. true여도 끊기기 직전에 받은 패킷이 수신 큐에 남아 있을 수 있다. */
    bool IsConnectionLost() const;

private:
    std::atomic<bool> bConnectionLost = false;

    //~ Packet Queues
public:
    /** 수신 큐의 패킷을 모두 핸들러로 처리한다. 게임 스레드 전용. */
    void HandleRecvPackets();

    /** 송신 큐에 넣고 송신 스레드를 깨운다. 송신 큐가 단일 생산자용이라 게임 스레드에서만 부른다. */
    void SendPacket(SendBufferRef SendBuffer);

    /** 수신 스레드가 넣고 게임 스레드가 꺼낸다. */
    TQueue<TArray<uint8>> RecvPacketQueue;
    /** 게임 스레드가 넣고 송신 스레드가 꺼낸다. */
    TQueue<SendBufferRef> SendPacketQueue;
};
