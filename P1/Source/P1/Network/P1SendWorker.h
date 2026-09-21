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
    bool SendPacket(SendBufferRef SendBuffer);
    void Destroy();

private:
    bool SendDesiredBytes(const uint8* Buffer, int32 Size);

protected:
    FRunnableThread* Thread = nullptr;
    bool Running = true;

    // 생성자 초기화 리스트가 Socket, SessionRef 순서로 적혀 있다. 둘의 선언 순서를 지킨다.
    FSocket* Socket;
    TWeakPtr<class PacketSession> SessionRef;
};
