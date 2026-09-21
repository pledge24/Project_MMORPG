#pragma once

#include "CoreMinimal.h"
#include "Utils/Types.h"

class FSocket;

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
    void Destroy();

private:
    bool ReceivePacket(TArray<uint8>& OutPacket);
    bool ReceiveDesiredBytes(uint8* Results, int32 Size);

protected:
    FRunnableThread* Thread = nullptr;
    bool Running = true;

    // 생성자 초기화 리스트가 Socket, SessionRef 순서로 적혀 있다. 둘의 선언 순서를 지킨다.
    FSocket* Socket;
    TWeakPtr<class PacketSession> SessionRef;
};
