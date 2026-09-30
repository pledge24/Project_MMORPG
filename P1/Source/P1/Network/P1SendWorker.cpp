#include "Network/P1SendWorker.h"
#include "Network/SendBuffer.h"
#include "Network/PacketSession.h"
#include "Sockets.h"
#include "Serialization/ArrayWriter.h"
#include "Utils/LogCategory.h"

FP1SendWorker::FP1SendWorker(FSocket* Socket, PacketSessionRef Session) : Socket(Socket), SessionRef(Session)
{
	// 스레드가 곧바로 Run에서 기다리므로 이벤트를 먼저 만든다.
	WakeEvent = FPlatformProcess::GetSynchEventFromPool(false);
	Thread = FRunnableThread::Create(this, TEXT("SendWorkerThread"));
}

FP1SendWorker::~FP1SendWorker()
{
	if (WakeEvent)
	{
		FPlatformProcess::ReturnSynchEventToPool(WakeEvent);
		WakeEvent = nullptr;
	}
}

bool FP1SendWorker::Init()
{
    UE_LOG(LogP1System, Display, TEXT("Send Thread Init"));
	return true;
}

uint32 FP1SendWorker::Run()
{
	while (bRunning)
	{
		// 보낼 것이 없으면 잠든다. Wake나 타임아웃으로 깨어나 큐를 비운다.
		WakeEvent->Wait(WAKE_TIMEOUT_MS);

		if (FlushQueue() == false)
		{
			UE_LOG(LogP1Network, Warning, TEXT("게임 서버로 송신하지 못해 송신 스레드를 멈춘다"));
			if (PacketSessionRef Session = SessionRef.Pin())
				Session->MarkConnectionLost();
			return 0;
		}
	}

	// 종료 요청 뒤에도 큐에 남은 패킷을 보낸다. 게임 인스턴스가 연결을 닫을 때 C_LEAVE_GAME이 여기서 나간다.
	// 세션 소멸자에서 온 종료라면 세션을 붙잡을 수 없어 보내지 못한다.
	FlushQueue();

	return 0;
}

void FP1SendWorker::Exit()
{

}

void FP1SendWorker::Wake()
{
	if (WakeEvent)
		WakeEvent->Trigger();
}

bool FP1SendWorker::FlushQueue()
{
	PacketSessionRef Session = SessionRef.Pin();
	if (Session == nullptr)
		return true;

	SendBufferRef SendBuffer;
	while (Session->SendPacketQueue.Dequeue(OUT SendBuffer))
	{
		if (SendPacket(SendBuffer) == false)
			return false;
	}

	return true;
}

bool FP1SendWorker::SendPacket(SendBufferRef SendBuffer)
{
	if (SendDesiredBytes(SendBuffer->Buffer(), SendBuffer->WriteSize()) == false)
		return false;

	return true;
}

void FP1SendWorker::Destroy()
{
	bRunning = false;
	Wake();

	// 스레드가 남은 큐를 보내고 끝난 뒤에 소켓이 닫히도록 여기서 기다린다.
	if (Thread)
	{
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;
	}
}

bool FP1SendWorker::SendDesiredBytes(const uint8* Buffer, int32 Size)
{
	while (Size > 0)
	{
		int32 BytesSent = 0;
		if (Socket->Send(Buffer, Size, BytesSent) == false)
			return false;

		Size -= BytesSent;
		Buffer += BytesSent;
	}

	return true;
}
