#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "Utils/Types.h"
#include "P1ConnectionSubsystem.generated.h"

class FSocket;

DECLARE_MULTICAST_DELEGATE(FOnConnectionLost);

/** 게임 서버와의 연결을 소유한다. 소켓, 세션, 수신 펌프가 여기 있다. */
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
     * 이전 연결을 정리하고 게임 서버에 접속한 뒤 AccessToken을 실은 C_LOGIN을 보낸다.
     * 접속에 실패하면 연결을 남기지 않는다. 게임 스레드 전용.
     */
    void Connect(const FString& AccessToken);

    /**
     * 소켓을 닫고 세션 스레드가 끝나기를 기다린 뒤 소켓을 파괴한다. 연결이 없으면 아무것도 하지 않는다.
     * OnConnectionLost를 알리지 않는다. 게임 스레드 전용.
     */
    void Close();

    bool IsConnected() const;

    /** 연결이 없으면 버린다. */
    void Send(SendBufferRef SendBuffer);

private:
    /** 접속 종료를 서버에 알린다. 연결이 없으면 아무것도 하지 않는다. */
    void SendLeaveGame();

    FSocket* Socket = nullptr;
    PacketSessionRef Session;

    //~ Connection Loss
public:
    /**
     * 수신 펌프가 끊김을 알아채 연결을 정리한 뒤 한 번 알린다. 게임 스레드에서 알린다.
     * 핸들러가 S_LEAVE_GAME을 처리하며 Close한 경우에는 알리지 않는다.
     */
    FOnConnectionLost OnConnectionLost;

    //~ Packet Pump
private:
    /** 코어 티커 콜백이다. true를 돌려주면 다음 프레임에도 호출된다. */
    bool TickRecvPump(float DeltaTime);

    void HandleRecvPackets();

    /** Initialize에서 등록하고 Deinitialize에서 해제한다. 게임 인스턴스와 수명이 같다. */
    FTSTicker::FDelegateHandle RecvPumpTickerHandle;
};
