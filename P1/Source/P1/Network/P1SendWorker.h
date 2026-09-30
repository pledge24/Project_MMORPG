#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "Utils/Types.h"

class FSocket;

class P1_API FP1SendWorker : public FRunnable
{
public:
    FP1SendWorker(FSocket* Socket, PacketSessionRef Session);
    ~FP1SendWorker();

    //~ Begin FRunnable Interface
public:
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Exit() override;
    //~ End FRunnable Interface

    //~ Send Loop
public:
    /** 큐에 패킷을 넣은 뒤 부른다. 잠든 송신 스레드를 깨운다. 어느 스레드에서 불러도 된다. */
    void Wake();

    /** 큐에 남은 패킷을 모두 보낸 뒤 스레드를 끝낸다. 소켓을 닫기 전에 불러야 남은 패킷이 나간다. */
    void Destroy();

private:
    bool SendPacket(SendBufferRef SendBuffer);
    bool SendDesiredBytes(const uint8* Buffer, int32 Size);
    // 큐가 빌 때까지 보낸다. 송신에 실패하면 false.
    bool FlushQueue();

    /** 깨울 사람이 없어도 이 간격마다 bRunning을 다시 본다. 밀리초 단위다. */
    static constexpr uint32 WAKE_TIMEOUT_MS = 100;

protected:
    FRunnableThread* Thread = nullptr;
    // 게임 스레드가 끄고 송신 스레드가 읽는다.
    std::atomic<bool> bRunning = true;
    FEvent* WakeEvent = nullptr;

    // 생성자 초기화 리스트가 Socket, SessionRef 순서로 적혀 있다. 둘의 선언 순서를 지킨다.
    FSocket* Socket;
    TWeakPtr<class PacketSession> SessionRef;
};
