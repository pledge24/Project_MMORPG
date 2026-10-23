#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "Utils/Types.h"
#include "Common.pb.h"
#include "P1ConnectionSubsystem.generated.h"

class FSocket;

DECLARE_MULTICAST_DELEGATE(FOnGameServerConnected);
DECLARE_MULTICAST_DELEGATE(FOnGameServerConnectFailed);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnConnectionLost, Protocol::DisconnectReason);

/**
 * 게임 서버와의 연결.
 *
 * 클라이언트가 게임 서버와 통신할 때 거치는 연결을 소유하고, 주소로 연결하고 끊는 일만 한다.
 * 연결 위에서 무엇을 보낼지는 모른다.
 */
UCLASS()
class P1_API UP1ConnectionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

    //~ Begin USubsystem Interface
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

    //~ Connection
public:
    /**
     * Ip와 Port의 게임 서버에 연결하고, 결과를 반환하기 전에 OnConnected나 OnConnectFailed로 알린다.
     * 이미 연결이 있으면 먼저 닫는다. 게임 스레드 전용.
     */
    void Connect(const FString& Ip, int32 Port);

    /**
     * 연결을 닫는다. 세션 스레드가 끝날 때까지 기다리고, OnConnectionLost는 알리지 않는다.
     * 게임 스레드 전용.
     */
    void Close();

    bool IsConnected() const;

    /** 연결이 없으면 버린다. */
    void Send(SendBufferRef SendBuffer);

    FOnGameServerConnected OnConnected;
    FOnGameServerConnectFailed OnConnectFailed;

private:
    FSocket* Socket = nullptr;
    PacketSessionRef Session;

    //~ Connection Loss
public:
    /** 서버가 S_DISCONNECT로 끊었을 때 연결을 닫고 OnConnectionLost를 알린다. 게임 스레드 전용. */
    void HandleDisconnect(Protocol::DisconnectReason Reason);

    /**
     * 연결이 끊기면 게임 스레드에서 한 번 알린다. 서버가 사유를 보내지 않았으면 DISCONNECT_REASON_NONE.
     * 게임 종료(Deinitialize)와 Close로 닫을 때는 알리지 않는다.
     */
    FOnConnectionLost OnConnectionLost;

    //~ Packet Pump
private:
    bool TickRecvPump(float DeltaTime);

    void HandleRecvPackets();

    FTSTicker::FDelegateHandle RecvPumpTickerHandle;
};
