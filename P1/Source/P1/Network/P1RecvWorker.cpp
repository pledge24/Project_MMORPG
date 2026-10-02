#include "Network/P1RecvWorker.h"
#include "Network/P1PacketHeader.h"
#include "Network/PacketSession.h"
#include "Network/ClientPacketHandler.h"
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
	while (bRunning)
	{
		// 읽을 데이터가 올 때까지 잠든다. 타임아웃으로 깨어나면 bRunning을 다시 본다.
		if (Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromSeconds(WAIT_FOR_READ_SECONDS)) == false)
		{
			// 타임아웃이 아니라 소켓 오류로 돌아왔으면 곧바로 다시 돌아와 헛돈다. 연결이 끊긴 것으로 본다.
			if (bRunning && Socket->GetConnectionState() == SCS_ConnectionError)
			{
				UE_LOG(LogP1Network, Warning, TEXT("게임 서버와 연결이 끊겨 수신 스레드를 멈춘다"));
				NotifyConnectionLost();
				break;
			}
			continue;
		}

		TArray<uint8> Packet;
		if (ReceivePacket(OUT Packet) == false)
		{
			// 읽을 수 있다고 깨어났는데 읽지 못했으면 연결이 끊긴 것이다. 다시 돌면 헛돌기만 한다.
			// 게임 스레드가 멈추라고 한 뒤라면 소켓을 닫아서 생긴 실패이므로 끊김으로 알리지 않는다.
			if (bRunning)
			{
				UE_LOG(LogP1Network, Warning, TEXT("게임 서버에서 수신하지 못해 수신 스레드를 멈춘다"));
				NotifyConnectionLost();
			}
			break;
		}

		if (PacketSessionRef Session = SessionRef.Pin())
		{
			Session->RecvPacketQueue.Enqueue(Packet);
		}
	}

	return 0;
}

void FP1RecvWorker::Exit()
{

}

void FP1RecvWorker::NotifyConnectionLost()
{
	// 받은 패킷은 모두 큐에 넣은 뒤에 부른다. 게임 스레드는 이 표시를 본 뒤 큐를 비우므로
	// 끊기기 직전에 온 S_LEAVE_GAME을 놓치지 않는다.
	if (PacketSessionRef Session = SessionRef.Pin())
		Session->MarkConnectionLost();
}

void FP1RecvWorker::RequestStop()
{
	bRunning = false;
}

void FP1RecvWorker::Destroy()
{
	RequestStop();

	// 스레드가 끝난 뒤에 소켓이 파괴되도록 여기서 기다린다. 소켓이 먼저 닫혀 있어야 진행 중인 Recv가 끝난다.
	if (Thread)
	{
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;
	}
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
        
        if (Header.PacketID != PKT_S_MOVE)
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
	// 소켓은 블로킹이다. 헤더 뒤의 페이로드가 아직 도착하지 않았어도 여기서 기다린다.
	// 예전처럼 대기 중인 데이터가 없다고 바로 돌아가면 읽은 헤더를 버리게 되어 스트림이 어긋난다.
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
