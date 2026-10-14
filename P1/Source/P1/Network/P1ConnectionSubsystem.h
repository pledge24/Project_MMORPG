#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "Utils/Types.h"
#include "Enum.pb.h"
#include "P1ConnectionSubsystem.generated.h"

class FSocket;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnConnectionLost, Protocol::LeaveReason);

/**
 * 게임 서버와의 연결.
 *
 * 클라이언트가 게임 서버와 통신할 때 거치는 연결을 소유하고, 접속과 끊김을 관리한다.
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
     * 게임 서버에 접속하고 AccessToken으로 로그인을 요청한다. 이미 연결이 있으면 먼저 닫는다.
     * 게임 스레드 전용.
     */
    void Connect(const FString& AccessToken);

    /**
     * 연결을 닫는다. 세션 스레드가 끝날 때까지 기다리고, OnConnectionLost는 알리지 않는다.
     * 게임 스레드 전용.
     */
    void Close();

    bool IsConnected() const;

    /** 연결이 없으면 버린다. */
    void Send(SendBufferRef SendBuffer);

private:
    /** 접속 종료를 서버에 알린다. */
    void SendLeaveGame();

    FSocket* Socket = nullptr;
    PacketSessionRef Session;

    //~ Connection Loss
public:
    /** 서버가 S_LEAVE_GAME으로 끊었을 때 연결을 닫고 OnConnectionLost를 알린다. 게임 스레드 전용. */
    void HandleLeaveGame(Protocol::LeaveReason Reason);

    /**
     * 연결이 끊기면 게임 스레드에서 한 번 알린다. 서버가 사유를 보내지 않았으면 LEAVE_REASON_NONE.
     * 게임 종료(Deinitialize)와 Close로 닫을 때는 알리지 않는다.
     */
    FOnConnectionLost OnConnectionLost;

    //~ Packet Pump
private:
    bool TickRecvPump(float DeltaTime);

    void HandleRecvPackets();

    FTSTicker::FDelegateHandle RecvPumpTickerHandle;
};
