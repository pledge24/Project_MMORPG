#include "Network/P1RecvWorker.h"
#include "Network/P1PacketHeader.h"
#include "Network/PacketSession.h"
#include "Sockets.h"
#include "Serialization/ArrayWriter.h"
#include "Utils/LogCategory.h"

FP1RecvWorker::FP1RecvWorker(FSocket* Socket, PacketSessionRef Session) : Socket(Socket), SessionRef(Session)
{
	Thread = FRunnableThread::Create(this, TEXT("RecvWorkerThread"));
}

FP1RecvWorker::~FP1RecvWorker()
{

}

bool FP1RecvWorker::Init()
{
    UE_LOG(LogP1System, Display, TEXT("Recv Thread Init"));
	return true;
}

uint32 FP1RecvWorker::Run()
{
	while (Running)
	{
		TArray<uint8> Packet;

		if (ReceivePacket(OUT Packet))
		{
			if (PacketSessionRef Session = SessionRef.Pin())
			{
				Session->RecvPacketQueue.Enqueue(Packet);
			}
		}
	}

	return 0;
}

void FP1RecvWorker::Exit()
{

}

void FP1RecvWorker::Destroy()
{
	Running = false;
}

bool FP1RecvWorker::ReceivePacket(TArray<uint8>& OutPacket)
{
	// 헤더 준비
	const int32 HeaderSize = sizeof(FP1PacketHeader);
	TArray<uint8> HeaderBuffer;
	HeaderBuffer.AddZeroed(HeaderSize);

	if (ReceiveDesiredBytes(HeaderBuffer.GetData(), HeaderSize) == false)
		return false;

	// ID, Size 파싱
	FP1PacketHeader Header;
	{
		FMemoryReader Reader(HeaderBuffer);
		Reader << Header;
        
        if (Header.PacketID != 1018 /* MovePacketId */)
        {
		    UE_LOG(LogP1Network, Log, TEXT("Recv PacketID : %d, PacketSize : %d"), Header.PacketID, Header.PacketSize);
        }
	}

	// 헤더 추가
	OutPacket = HeaderBuffer;

	// 페이로드
	TArray<uint8> PayloadBuffer;
	const int32 PayloadSize = Header.PacketSize - HeaderSize;
	if (PayloadSize == 0)
		return true;

	OutPacket.AddZeroed(PayloadSize);

	if (ReceiveDesiredBytes(&OutPacket[HeaderSize], PayloadSize))
		return true;

	return false;
}

bool FP1RecvWorker::ReceiveDesiredBytes(uint8* Results, int32 Size)
{
	uint32 PendingDataSize;
	if (Socket->HasPendingData(OUT PendingDataSize) == false || PendingDataSize <= 0)
		return false;

	int32 Offset = 0;

	while (Size > 0)
	{
		int32 NumRead = 0;
		Socket->Recv(Results + Offset, Size, OUT NumRead);
		check(NumRead <= Size);

		if (NumRead <= 0)
			return false;

		Offset += NumRead;
		Size -= NumRead;
	}

	return true;
}
