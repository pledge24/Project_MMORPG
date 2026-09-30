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
	// 핸들러(S_LEAVE_GAME)가 연결을 끊으면 Socket이 비고, 남은 패킷은 처리하지 않는다.
	while (Socket)
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
	// 순서가 중요하다.
	// 1) 수신 스레드에 먼저 멈추라고 알린다. 소켓이 닫혀 Recv가 실패해도 끊김으로 오인해 경고하지 않는다.
	// 2) 송신 스레드가 남은 큐(C_LEAVE_GAME 등)를 보낸 뒤 끝난다.
	// 3) 소켓을 닫아야 페이로드를 기다리며 블로킹된 Recv가 풀려 수신 스레드가 끝난다.
	if (RecvWorkerThread)
		RecvWorkerThread->RequestStop();

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

	// 소켓은 게임 인스턴스가 소유하고 곧 해제한다. 핸들러가 연결을 끊어도 펌프가 이 세션을 쥐고 있어서
	// 소멸자가 나중에 Disconnect를 다시 부른다. 그때 해제된 소켓을 건드리지 않도록 놓는다.
	Socket = nullptr;
}

