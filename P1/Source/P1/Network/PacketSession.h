#pragma once

#include "CoreMinimal.h"
#include "SendBuffer.h"
#include "Types.h"

class P1_API PacketSession : public TSharedFromThis<PacketSession>
{
public:
	PacketSession(class FSocket* Socket);
	~PacketSession();

	void Run();

	UFUNCTION(BlueprintCallable)
	void HandleRecvPackets();

	void SendPacket(SendBufferRef SendBuffer);

	void Disconnect();

public:
	class FSocket* Socket;

	RecvWorkerRef RecvWorkerThread;
	SendWorkerRef SendWorkerThread;

	// GameThread랑 NetworkThread가 통신할때 사용하는 패킷 저장 큐
	TQueue<TArray<uint8>> RecvPacketQueue;
	TQueue<SendBufferRef> SendPacketQueue;
};
