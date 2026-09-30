#include "Network/PacketSession.h"
#include "Network/P1RecvWorker.h"
#include "Network/P1SendWorker.h"
#include "Sockets.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"
#include "SocketSubsystem.h"
#include "Network/ClientPacketHandler.h"
#include "Core/P1GameInstance.h"
#include "Utils/LogCategory.h"

PacketSession::PacketSession(class FSocket* Socket, UP1GameInstance* InGameInstance) : GameInstance(InGameInstance), Socket(Socket)
{
	ClientPacketHandler::Init();
}

UP1GameInstance* PacketSession::GetGameInstance() const
{
	return GameInstance.Get();
}

PacketSession::~PacketSession()
{
	Disconnect();
}

void PacketSession::Run()
{
	RecvWorkerThread = MakeShared<FP1RecvWorker>(Socket, AsShared());
	SendWorkerThread = MakeShared<FP1SendWorker>(Socket, AsShared());
}

void PacketSession::HandleRecvPackets()
{
	while (true)
	{
		TArray<uint8> Packet;
		if (RecvPacketQueue.Dequeue(OUT Packet) == false)
			break;

		PacketSessionRef ThisPtr = AsShared();
		bool bHandlePacket = ClientPacketHandler::HandlePacket(ThisPtr, Packet.GetData(), Packet.Num());
        if (!bHandlePacket)
        {
            PacketHeader* header = reinterpret_cast<PacketHeader*>(Packet.GetData());
            UE_LOG(LogP1Network, Warning, TEXT("Fail to Handle Packet. Packet Id: %d"), header->id);
        }
	}
}

void PacketSession::SendPacket(SendBufferRef SendBuffer)
{
	SendPacketQueue.Enqueue(SendBuffer);

	if (SendWorkerThread)
		SendWorkerThread->Wake();
}

void PacketSession::Disconnect()
{
	// 순서가 중요하다. 송신 스레드가 남은 큐(C_LEAVE_GAME 등)를 보낸 뒤에 소켓을 닫고,
	// 소켓이 닫혀야 페이로드를 기다리며 블로킹된 Recv가 풀려 수신 스레드가 끝난다.
	if (SendWorkerThread)
	{
		SendWorkerThread->Destroy();
		SendWorkerThread = nullptr;
	}

	if (Socket)
		Socket->Close();

	if (RecvWorkerThread)
	{
		RecvWorkerThread->Destroy();
		RecvWorkerThread = nullptr;
	}
}

