#pragma once

#include "CoreMinimal.h"
#include "Utils/Types.h"

class FSocket;

class P1_API FP1RecvWorker : public FRunnable
{
public:
	FP1RecvWorker(FSocket* Socket, PacketSessionRef Session);
	~FP1RecvWorker();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;

	void Destroy();

private:
	bool ReceivePacket(TArray<uint8>& OutPacket);
	bool ReceiveDesiredBytes(uint8* Results, int32 Size);

protected:
	FRunnableThread* Thread = nullptr;
	bool Running = true;
	FSocket* Socket;
	TWeakPtr<class PacketSession> SessionRef;
};
