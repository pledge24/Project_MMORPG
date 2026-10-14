#pragma once

#include "CoreMinimal.h"
#include "Utils/Types.h"

class FSocket;

/**
 * 게임 서버에서 패킷을 받는 수신 스레드.
 *
 * 소켓에서 패킷 단위로 읽어 세션의 수신 큐에 넘기고, 연결이 끊기면 세션에 알린다.
 * 이 스레드는 UObject를 만지지 않는다.
 */
class P1_API FP1RecvWorker : public FRunnable
{
public:
    FP1RecvWorker(FSocket* Socket, PacketSessionRef Session);
    ~FP1RecvWorker();

    //~ Begin FRunnable Interface
public:
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Exit() override;
    //~ End FRunnable Interface

    //~ Receive Loop
public:
    /**
     * 루프에 멈추라고 알리고 기다리지 않는다.
     * 소켓을 닫기 전에 불러야 닫혀서 생긴 수신 실패를 끊김으로 알리지 않는다.
     */
    void RequestStop();

    /**
     * 스레드가 끝날 때까지 기다린다.
     * 페이로드를 기다리며 막혀 있으면 소켓이 닫혀야 끝나므로, 소켓을 먼저 닫고 부른다.
     */
    void Destroy();

private:
    void NotifyConnectionLost();
    bool ReceivePacket(TArray<uint8>& OutPacket);
    bool ReceiveDesiredBytes(uint8* Results, int32 Size);

    /** 읽을 데이터를 한 번에 기다리는 최대 시간(초). 멈춤 요청 뒤 루프가 빠져나오기까지의 상한이다. */
    static constexpr float WAIT_FOR_READ_SECONDS = 0.1f;

protected:
    FRunnableThread* Thread = nullptr;
    /** 게임 스레드가 끄고 수신 스레드가 읽는다. */
    std::atomic<bool> bRunning = true;

    // 생성자 초기화 리스트가 Socket, SessionRef 순서로 적혀 있다. 둘의 선언 순서를 지킨다.
    FSocket* Socket;
    TWeakPtr<class PacketSession> SessionRef;
};
