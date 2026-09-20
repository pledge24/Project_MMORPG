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

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;

	bool SendPacket(SendBufferRef SendBuffer);

	void Destroy();

private:
	bool SendDesiredBytes(const uint8* Buffer, int32 Size);

protected:
	FRunnableThread* Thread = nullptr;
	bool Running = true;
	FSocket* Socket;
	TWeakPtr<class PacketSession> SessionRef;
};
