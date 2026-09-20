#include "Network/P1SendWorker.h"
#include "Network/SendBuffer.h"
#include "Network/PacketSession.h"
#include "Sockets.h"
#include "Serialization/ArrayWriter.h"
#include "Utils/LogCategory.h"

FP1SendWorker::FP1SendWorker(FSocket* Socket, PacketSessionRef Session) : Socket(Socket), SessionRef(Session)
{
	Thread = FRunnableThread::Create(this, TEXT("SendWorkerThread"));
}

FP1SendWorker::~FP1SendWorker()
{

}

bool FP1SendWorker::Init()
{
    UE_LOG(LogSystem, Display, TEXT("Send Thread Init"));
	return true;
}

uint32 FP1SendWorker::Run()
{
	while (Running)
	{
		SendBufferRef SendBuffer;

		if (PacketSessionRef Session = SessionRef.Pin())
		{
			if (Session->SendPacketQueue.Dequeue(OUT SendBuffer))
			{
				SendPacket(SendBuffer);
			}
		}
	}

	return 0;
}

void FP1SendWorker::Exit()
{

}

bool FP1SendWorker::SendPacket(SendBufferRef SendBuffer)
{
	if (SendDesiredBytes(SendBuffer->Buffer(), SendBuffer->WriteSize()) == false)
		return false;

	return true;
}

void FP1SendWorker::Destroy()
{
	Running = false;
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
