#include "Network/P1ConnectionSubsystem.h"
#include "Network/PacketSession.h"
#include "Network/P1NetworkSettings.h"
#include "Network/ClientPacketHandler.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Utils/LogCategory.h"

void UP1ConnectionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 수신 펌프를 코어 티커에 등록한다.
    // 게임 인스턴스는 레벨 전환에 살아남으므로 펌프도 레벨과 무관하게 계속 돈다.
    // 코어 티커는 게임 스레드에서 돌기 때문에 여기서 UObject를 만져도 된다.
    RecvPumpTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UP1ConnectionSubsystem::TickRecvPump));
}

void UP1ConnectionSubsystem::Deinitialize()
{
    // 펌프를 먼저 멈춘다. 종료 중에 패킷을 처리하면 이미 정리된 UObject를 건드린다.
    if (RecvPumpTickerHandle.IsValid())
    {
        FTSTicker::RemoveTicker(RecvPumpTickerHandle);
        RecvPumpTickerHandle.Reset();
    }

    // 사용자가 게임을 끄는 경로다. OnConnectionLost를 알리지 않고 서버에 접속 종료만 알린다.
    SendLeaveGame();
    Close();

    Super::Deinitialize();
}

void UP1ConnectionSubsystem::Connect(const FString& AccessToken)
{
    // 로그인을 다시 누르면 이 함수가 또 불린다. 이전 연결을 정리하지 않으면 소켓이 샌다.
    Close();

    Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(TEXT("Stream"), TEXT("Client Socket"));

    const UP1NetworkSettings* NetworkSettings = GetDefault<UP1NetworkSettings>();

    FIPv4Address Ip;
    FIPv4Address::Parse(NetworkSettings->GameServerIp, Ip);

    TSharedRef<FInternetAddr> InternetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
    InternetAddr->SetIp(Ip.Value);
    InternetAddr->SetPort(NetworkSettings->GameServerPort);

    const bool bConnected = Socket->Connect(*InternetAddr);
    if (bConnected == false)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, FString::Printf(TEXT("Fail To Connect GameServer")));

        Close();
        return;
    }

    GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("Success To Connect GameServer")));

    Session = MakeShared<PacketSession>(Socket, this);
    Session->Run();

    // 인증 서버가 발급한 토큰으로 게임 서버에 로그인한다. 토큰은 게임 서버가 한 번 쓰고 지운다.
    Protocol::C_LOGIN Pkt;
    Pkt.set_access_token(TCHAR_TO_UTF8(*AccessToken));
    Send(ClientPacketHandler::MakeSerializedPacket(Pkt));
}

void UP1ConnectionSubsystem::Close()
{
    // 세션이 송신 큐를 비운 뒤 소켓을 닫고 수신 스레드를 끝낸다. 순서는 PacketSession::Disconnect에 있다.
    if (Session)
    {
        Session->Disconnect();
        Session = nullptr;
    }

    if (Socket)
    {
        // 연결에 실패해 세션이 없었으면 여기서 처음 닫힌다. 세션이 이미 닫았으면 두 번째 Close는 무시된다.
        Socket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
        Socket = nullptr;
    }
}

bool UP1ConnectionSubsystem::IsConnected() const
{
    return Socket != nullptr && Session != nullptr;
}

void UP1ConnectionSubsystem::Send(SendBufferRef SendBuffer)
{
    // 생성된 MakeSerializedPacket은 헤더에 담기지 않는 패킷이면 nullptr를 돌려주고 로그를 남긴다.
    if (SendBuffer.IsValid() == false || IsConnected() == false)
        return;

    Session->SendPacket(SendBuffer);
}

void UP1ConnectionSubsystem::SendLeaveGame()
{
    if (IsConnected() == false)
        return;

    Protocol::C_LEAVE_GAME LeavePkt;
    Send(ClientPacketHandler::MakeSerializedPacket(LeavePkt));
}

bool UP1ConnectionSubsystem::TickRecvPump(float DeltaTime)
{
    UWorld* World = GetGameInstance()->GetWorld();
    if (World == nullptr)
        return true;

    // 월드가 준비되기 전과 정리 중에는 펌프를 돌리지 않는다.
    // 레벨 블루프린트 틱은 레벨 전환 동안 죽어 있어서 그 사이 패킷이 큐에 쌓였다가
    // 월드가 준비된 뒤 처리됐다. 코어 티커는 전환 중에도 돌기 때문에 그 버퍼링을
    // 여기서 직접 복원한다. 큐는 비우지 않으므로 패킷은 유실되지 않는다.
    if (World->bIsTearingDown || World->HasBegunPlay() == false)
        return true;

    // 코어 티커는 월드 틱 밖에서 돌기 때문에 이 시점의 GWorld는 게임 월드가 아니다.
    // 패킷 핸들러는 GWorld를 보지 않고 세션에서 얻은 게임 인스턴스를 쓴다(PacketSession::GetGameInstance).
    HandleRecvPackets();

    return true;
}

void UP1ConnectionSubsystem::HandleRecvPackets()
{
    if (IsConnected() == false)
        return;

    // 끊김 표시를 큐보다 먼저 읽는다. 수신 워커는 마지막 패킷을 큐에 넣은 뒤 표시를 세우므로,
    // 표시를 본 뒤에 큐를 비우면 끊기기 직전에 온 S_LEAVE_GAME까지 처리한다.
    const bool bConnectionLost = Session->IsConnectionLost();

    Session->HandleRecvPackets();

    // S_LEAVE_GAME을 처리했으면 HandleLeaveGame이 이미 연결을 닫고 알려서 세션이 없다.
    if (bConnectionLost && Session)
    {
        Close();
        OnConnectionLost.Broadcast(Protocol::LEAVE_REASON_NONE);
    }
}

void UP1ConnectionSubsystem::HandleLeaveGame(Protocol::LeaveReason Reason)
{
    Close();
    OnConnectionLost.Broadcast(Reason);
}
