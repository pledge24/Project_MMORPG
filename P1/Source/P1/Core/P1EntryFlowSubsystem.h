#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "P1EntryFlowSubsystem.generated.h"

class AP1MyPlayer;
class UP1AuthClient;
class UP1ConnectionSubsystem;
class UP1LobbySubsystem;
class UP1MyPlayerData;

// 로그인 화면이 이 헤더를 부르므로 protobuf 헤더를 끌고 오지 않는다. 생성된 정의와 바탕 타입이 같아야 한다.
namespace Protocol
{
    enum DisconnectReason : int;
}

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEntryNotice, const FString& /*Notice*/);

/** 클라이언트가 로그인부터 게임까지 지나는 단계. */
enum class EP1EntryPhase : uint8
{
    Login,          // 인증 서버 로그인. 로그인 세션이 없다
    WorldSelect,    // 로그인 세션으로 월드를 고른다. 고른 월드의 게임 서버에 연결해 로비 접속 응답을 기다리는 동안도 이 단계다
    Lobby,          // 캐릭터 목록을 받았다
    MapLoad,        // 서버가 알린 맵을 여는 중이다
    Game,           // 맵을 열고 맵 로드 완료를 알렸다
    ReturningToLobby,   // 게임에서 로비 복귀를 요청했고, 서버가 진행을 저장한 뒤 보낼 캐릭터 목록을 기다린다
};

/** 진행 표시 화면이 보여 줄 접속 진행. */
enum class EP1EntryProgress : uint8
{
    None,
    ConnectingWorld,    // 월드를 고른 뒤 인증 티켓을 받아 그 월드의 게임 서버에 연결하고 로비 접속 응답을 기다린다
    LoadingMap,         // 서버가 알린 맵을 열고, 그 맵에 내 플레이어가 스폰되기를 기다린다
    ReturningToLobby,   // 게임에서 로비 복귀를 요청해 저장을 기다리고, 캐릭터 목록을 받은 뒤 로그인 맵을 연다
    ReturningToWorldSelect, // 게임에서 연결을 닫고 월드 선택 화면이 있는 로그인 맵을 연다
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEntryProgressChanged, EP1EntryProgress /*Progress*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEntryPhaseChanged, EP1EntryPhase /*Phase*/);

/**
 * 접속 흐름.
 *
 * 로그인부터 게임까지의 단계 상태와 단계 사이의 레벨 전환을 갖는다. 단계 사이의 순서는 이 서브시스템만 안다.
 * 모든 멤버 함수는 게임 스레드 전용이다.
 */
UCLASS()
class P1_API UP1EntryFlowSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

    //~ Begin USubsystem Interface
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

    //~ Phase
public:
    EP1EntryPhase GetPhase() const { return Phase; }

    /** From에서 To로 넘어갈 수 있으면 true. 같은 단계로 넘어가는 것은 전이가 아니다. */
    static bool CanTransition(EP1EntryPhase From, EP1EntryPhase To);

    /** 허용된 전이면 단계를 바꾸고 OnPhaseChanged로 알린 뒤 true. 아니면 경고를 남기고 단계를 그대로 둔 채 false. */
    bool TryEnterPhase(EP1EntryPhase NewPhase);

    /** 단계가 바뀌었다. 로그인 메뉴가 단계에 맞는 화면을 보인다. */
    FOnEntryPhaseChanged OnPhaseChanged;

private:
    EP1EntryPhase Phase = EP1EntryPhase::Login;

    //~ Progress
public:
    EP1EntryProgress GetProgress() const;

    /** 진행 표시가 바뀌면 알린다. 같은 진행으로는 알리지 않는다. */
    FOnEntryProgressChanged OnProgressChanged;

private:
    /** 진행이 바뀌었으면 바뀐 진행을 알린다. 단계나 연결 중 표시를 바꾼 뒤에 바꾸기 전의 진행을 넘겨 부른다. */
    void NotifyProgressIfChanged(EP1EntryProgress Before);

    void SetConnectingWorld(bool bConnecting);

    /** 월드를 골라 그 월드에 접속하기 시작했고, 아직 로비에 들어가지도 실패를 알리지도 않았다. */
    bool bConnectingWorld = false;

    /**
     * 맵 로드 단계에 들어간 뒤 아직 내 플레이어가 스폰되지 않았다. 맵을 다 열어도 서버가 플레이어를 룸에 넣기 전까지는
     * 빈 맵이 보이므로 그동안도 맵을 불러오는 중으로 본다.
     */
    bool bAwaitingMyPlayer = false;

    //~ World Selection
public:
    /** 월드 선택 단계에서 WorldId 월드에 접속한다. 이미 접속하는 중이면 아무것도 하지 않는다. */
    void SelectWorld(int32 WorldId);

    /** 월드 선택 단계에서 로그인 세션을 버리고 로그인 단계로 돌아간다. 접속하는 중이면 아무것도 하지 않는다. */
    void Logout();

    /**
     * 로비나 게임 단계에서 게임 서버와의 연결을 닫고, 다시 로그인하지 않고 월드 선택 단계로 돌아간다.
     * 게임에서 돌아가면 로그인 맵을 연다. 그사이 로그인 세션이 만료됐으면 월드 목록을 받을 때 로그인 화면으로 간다.
     */
    void ReturnToWorldSelect();

private:
    void HandleLoginSucceeded();
    void HandleWorldSelected(const FString& Host, int32 Port, const FString& AuthTicket);
    void HandleLoginSessionExpired(const FString& Message);

    //~ Lobby Connection
private:
    void HandleConnected();
    void HandleConnectFailed();
    void HandleLobbyEntered();

    /** 서버가 로비 복귀의 캐릭터 목록을 불러오지 못했다. 연결을 닫고 로그인 화면에 사유를 보여 준다. */
    void HandleReturnToLobbyFailed();

    /** 서버가 로비 접속을 거절하면 부른다. 월드 선택 화면에 Message를 보여 준다. */
    void HandleLobbyConnectFailed(const FText& Message);

    /** 연결되면 로비 접속에 낼 인증 티켓. 내자마자 비운다. */
    FString PendingAuthTicket;

    //~ Game Entry
private:
    /**
     * 게임 입장에 성공했거나 서버가 다른 맵으로 보내면 맵 로드 단계로 넘어가 그 맵의 레벨을 연다.
     * 데이터 설정에 그 맵의 레벨이 없으면 연결을 끊고 로그인 화면에 사유를 보여 준다.
     */
    void EnterMapLoad();

    /** 맵 로드 단계에서 이 게임 인스턴스의 맵이 열리면 맵 로드 완료를 알린다. */
    void HandlePostLoadMap(UWorld* LoadedWorld);

    void SendMapLoadComplete();

    void HandleMyPlayerSpawned(AP1MyPlayer* MyPlayer);

    FDelegateHandle PostLoadMapHandle;

    //~ Return To Lobby
public:
    /** 게임 단계에서 로비 복귀를 요청한다. 서버가 진행을 저장한 뒤 캐릭터 목록을 보내면 로그인 맵을 열고 캐릭터 선택 화면을 보인다. */
    void ReturnToLobby();

private:
    /** 게임에서 로그인 맵으로 돌아갈 때 내 플레이어의 칸과 퀵슬롯을 비운다. 다음 입장 패킷이 다시 채운다. */
    void ClearMyPlayerData();

    //~ Notice
public:
    /** 로그인 화면에 보여 줄 문구를 꺼내고 비운다. 로그인 맵을 다시 열어 돌아온 것이 아니면 빈 문자열. */
    FString ConsumeLoginNotice();

    /**
     * 이 서브시스템의 델리게이트에서 Listener가 붙인 것을 모두 뗀다.
     * 서브시스템은 레벨보다 오래 살기 때문에, 레벨과 함께 사라지는 객체는 사라지기 전에 불러야 한다.
     */
    void RemoveListener(const UObject* Listener);

    /**
     * 로그인 맵에 있는 동안 생긴 사유를 지금 단계의 화면(로그인 또는 월드 선택)에 보여 줄 문구로 알린다.
     * 단계를 바꾸며 알릴 때는 OnPhaseChanged 뒤에 온다.
     */
    FOnEntryNotice OnNotice;

private:
    void HandleConnectionLost(Protocol::DisconnectReason Reason);

    /**
     * 로그인 세션을 버리고 로그인 단계로 돌아가 Notice를 보여 준다. 게임 중이면 로그인 맵을 연다.
     * Notice가 비었으면 문구를 알리지 않는다.
     */
    void ReturnToLogin(const FString& Notice);

    /** 월드 접속을 그만두고 월드 선택 화면에 Notice를 보여 준다. 로그인 세션은 남으므로 다시 월드를 고를 수 있다. */
    void ShowWorldSelectNotice(const FString& Notice);

    /** 로그인 메뉴의 화면(로그인, 월드 선택, 캐릭터 선택)을 쓰는 단계면 true. */
    static bool IsLoginMenuPhase(EP1EntryPhase InPhase);

    /** 로그인 맵이 열린 뒤 로그인 화면에 보여 줄 문구. 레벨 전환을 건너 들고 간다. */
    FString PendingLoginNotice;

    //~ Level Transition
private:
    /**
     * 다음 프레임에 LevelName 레벨을 연다. 패킷 핸들러 호출 안에서 레벨을 바꾸지 않고, 레벨을 여는 동안 화면이 멈추기 전에
     * 로딩 화면이 한 번 그려지게 하려는 것이다.
     */
    void OpenLevelNextTick(FName LevelName);

    FTSTicker::FDelegateHandle OpenLevelTickerHandle;

    /** 열려 있거나 여는 중인 레벨이 로그인 맵이면 true. 게임은 로그인 맵에서 시작한다. */
    bool bLoginMapOpen = true;

    /** 레벨을 열기로 했고 아직 그 레벨이 열렸다는 알림을 받지 않았다. 그동안에는 열린 화면이 없다. */
    bool bOpeningLevel = false;

    //~ 공통
private:
    UPROPERTY()
    TObjectPtr<UP1AuthClient> AuthClient;

    UPROPERTY()
    TObjectPtr<UP1ConnectionSubsystem> Connection;

    UPROPERTY()
    TObjectPtr<UP1LobbySubsystem> Lobby;

    UPROPERTY()
    TObjectPtr<UP1MyPlayerData> MyPlayerData;
};
