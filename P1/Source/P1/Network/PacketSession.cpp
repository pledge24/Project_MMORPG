#include "Network/PacketSession.h"
#include "Network/P1RecvWorker.h"
#include "Network/P1SendWorker.h"
#include "Sockets.h"
#include "Common/TcpSocketBuilder.h"
#include "Serialization/ArrayWriter.h"
#include "SocketSubsystem.h"
#include "Network/ClientPacketHandler.h"

PacketSession::PacketSession(class FSocket* Socket) : Socket(Socket)
{
	ClientPacketHandler::Init();
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
            UE_LOG(LogTemp, Warning, TEXT("Fail to Handle Packet. Packet Id: %d"), header->id);
        }
	}
}

void PacketSession::SendPacket(SendBufferRef SendBuffer)
{
	SendPacketQueue.Enqueue(SendBuffer);
}

void PacketSession::Disconnect()
{
	if (RecvWorkerThread)
	{
		RecvWorkerThread->Destroy();
		RecvWorkerThread = nullptr;
	}

	if (SendWorkerThread)
	{
		SendWorkerThread->Destroy();
		SendWorkerThread = nullptr;
	}
}

