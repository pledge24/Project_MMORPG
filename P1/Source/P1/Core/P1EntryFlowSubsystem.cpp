#include "Core/P1EntryFlowSubsystem.h"
#include "Auth/P1AuthClient.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Lobby/P1LobbySubsystem.h"
#include "Network/P1ConnectionSubsystem.h"
#include "Network/P1NetworkSettings.h"
#include "Network/P1PacketSender.h"
#include "Common.pb.h"
#include "Entry.pb.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"
#include "Utils/LogCategory.h"

namespace
{
    const FName LOGIN_LEVEL_NAME(TEXT("L_LoginMap"));

    /** 사유를 모르는 끊김에 보여 줄 문구. 서버가 먼저 내려갔거나 사유 패킷을 잃은 경우다. */
    const TCHAR* const CONNECTION_LOST_NOTICE = TEXT("게임 서버와 연결이 끊겼습니다.");

    const TCHAR* const CONNECT_FAILED_NOTICE = TEXT("게임 서버에 접속하지 못했습니다.");

    /** 서버가 이 클라이언트에 없는 맵으로 보냈을 때 보여 줄 문구. 클라이언트와 서버의 기획 데이터가 어긋난 경우다. */
    const TCHAR* const UNKNOWN_MAP_NOTICE = TEXT("이 클라이언트에 없는 맵입니다. 클라이언트를 최신으로 업데이트해 주세요.");

    const TCHAR* PhaseName(EP1EntryPhase Phase)
    {
        switch (Phase)
        {
        case EP1EntryPhase::Login: return TEXT("Login");
        case EP1EntryPhase::Lobby: return TEXT("Lobby");
        case EP1EntryPhase::MapLoad: return TEXT("MapLoad");
        case EP1EntryPhase::Game: return TEXT("Game");
        }

        return TEXT("Unknown");
    }
}

void UP1EntryFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // 단계를 잇는 알림 구독. 구독할 서브시스템이 먼저 초기화돼 있어야 한다.
    AuthClient = Collection.InitializeDependency<UP1AuthClient>();
    Connection = Collection.InitializeDependency<UP1ConnectionSubsystem>();
    Lobby = Collection.InitializeDependency<UP1LobbySubsystem>();
    MyPlayerData = Collection.InitializeDependency<UP1MyPlayerData>();

    if (AuthClient)
        AuthClient->OnLoginSucceeded.AddUObject(this, &UP1EntryFlowSubsystem::HandleLoginSucceeded);

    if (Connection)
    {
        Connection->OnConnected.AddUObject(this, &UP1EntryFlowSubsystem::HandleConnected);
        Connection->OnConnectFailed.AddUObject(this, &UP1EntryFlowSubsystem::HandleConnectFailed);
        Connection->OnConnectionLost.AddUObject(this, &UP1EntryFlowSubsystem::HandleConnectionLost);
    }

    if (Lobby)
    {
        Lobby->OnLobbyEntered.AddUObject(this, &UP1EntryFlowSubsystem::HandleLobbyEntered);
        Lobby->OnLobbyConnectFailed.AddUObject(this, &UP1EntryFlowSubsystem::HandleLobbyConnectFailed);
        Lobby->OnEnterGameSucceeded.AddUObject(this, &UP1EntryFlowSubsystem::EnterMapLoad);
    }

    if (MyPlayerData)
    {
        MyPlayerData->OnMapChanged.AddUObject(this, &UP1EntryFlowSubsystem::EnterMapLoad);
        MyPlayerData->OnMyPlayerSpawned.AddUObject(this, &UP1EntryFlowSubsystem::HandleMyPlayerSpawned);
    }

    PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UP1EntryFlowSubsystem::HandlePostLoadMap);
}

void UP1EntryFlowSubsystem::Deinitialize()
{
    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
    PostLoadMapHandle.Reset();

    // 종료 중에 레벨을 열면 이미 정리된 월드를 건드린다.
    if (OpenLevelTickerHandle.IsValid())
    {
        FTSTicker::RemoveTicker(OpenLevelTickerHandle);
        OpenLevelTickerHandle.Reset();
    }

    Super::Deinitialize();
}

bool UP1EntryFlowSubsystem::CanTransition(EP1EntryPhase From, EP1EntryPhase To)
{
    switch (To)
    {
    case EP1EntryPhase::Login:
        // 연결이 끊기면 어느 단계에서든 돌아온다.
        return From != EP1EntryPhase::Login;
    case EP1EntryPhase::Lobby:
        return From == EP1EntryPhase::Login;
    case EP1EntryPhase::MapLoad:
        // 게임에서 맵 로드로 가는 것은 맵을 옮길 때다.
        return From == EP1EntryPhase::Lobby || From == EP1EntryPhase::Game;
    case EP1EntryPhase::Game:
        return From == EP1EntryPhase::MapLoad;
    }

    return false;
}

bool UP1EntryFlowSubsystem::TryEnterPhase(EP1EntryPhase NewPhase)
{
    if (CanTransition(Phase, NewPhase) == false)
    {
        UE_LOG(LogP1System, Warning, TEXT("허용되지 않은 접속 단계 전이를 거절함: %s → %s"), PhaseName(Phase), PhaseName(NewPhase));
        return false;
    }

    UE_LOG(LogP1System, Log, TEXT("접속 단계: %s → %s"), PhaseName(Phase), PhaseName(NewPhase));
    const EP1EntryProgress Before = GetProgress();
    Phase = NewPhase;

    // 맵을 옮길 때마다 새 맵의 스폰을 기다린다. 로그인으로 돌아가면 기다릴 스폰이 없다.
    if (NewPhase == EP1EntryPhase::MapLoad)
        bAwaitingMyPlayer = true;
    else if (NewPhase == EP1EntryPhase::Login)
        bAwaitingMyPlayer = false;

    NotifyProgressIfChanged(Before);
    return true;
}

EP1EntryProgress UP1EntryFlowSubsystem::GetProgress() const
{
    if (Phase == EP1EntryPhase::MapLoad)
        return EP1EntryProgress::LoadingMap;

    if (Phase == EP1EntryPhase::Game && bAwaitingMyPlayer)
        return EP1EntryProgress::LoadingMap;

    if (Phase == EP1EntryPhase::Login && bConnectingWorld)
        return EP1EntryProgress::ConnectingWorld;

    return EP1EntryProgress::None;
}

void UP1EntryFlowSubsystem::NotifyProgressIfChanged(EP1EntryProgress Before)
{
    const EP1EntryProgress After = GetProgress();
    if (After != Before)
        OnProgressChanged.Broadcast(After);
}

void UP1EntryFlowSubsystem::SetConnectingWorld(bool bConnecting)
{
    const EP1EntryProgress Before = GetProgress();
    bConnectingWorld = bConnecting;
    NotifyProgressIfChanged(Before);
}

void UP1EntryFlowSubsystem::HandleLoginSucceeded(const FString& AuthTicket)
{
    // 월드 선택이 생기기 전까지는 설정의 주소로 접속한다.
    const UP1NetworkSettings* NetworkSettings = GetDefault<UP1NetworkSettings>();

    // Connect 안에서 실패를 알리면 로그인으로 돌아가며 이 표시를 끈다. 그래서 부르기 전에 켠다.
    SetConnectingWorld(true);

    // 연결은 Connect 안에서 끝나고 그 안에서 결과를 알리므로, 부르기 전에 티켓을 맡겨 둔다.
    PendingAuthTicket = AuthTicket;
    Connection->Connect(NetworkSettings->GameServerIp, NetworkSettings->GameServerPort);
    PendingAuthTicket.Reset();
}

void UP1EntryFlowSubsystem::HandleConnected()
{
    if (PendingAuthTicket.IsEmpty())
    {
        UE_LOG(LogP1Network, Warning, TEXT("인증 티켓 없이 게임 서버에 연결됨. 로비에 접속하지 않는다"));
        return;
    }

    Lobby->ConnectLobby(PendingAuthTicket);
    PendingAuthTicket.Reset();
}

void UP1EntryFlowSubsystem::HandleConnectFailed()
{
    ReturnToLogin(CONNECT_FAILED_NOTICE);
}

void UP1EntryFlowSubsystem::HandleLobbyEntered()
{
    SetConnectingWorld(false);
    TryEnterPhase(EP1EntryPhase::Lobby);
}

void UP1EntryFlowSubsystem::HandleLobbyConnectFailed(const FText& Message)
{
    // 서버가 이 응답 뒤에 끊는다. 먼저 닫지 않으면 끊김이 사유 없는 끊김으로 와서 결과의 문구를 덮는다.
    // 티켓은 게임 서버가 이미 지웠으므로 다시 로그인해야 한다.
    Connection->Close();
    ReturnToLogin(Message.ToString());
}

void UP1EntryFlowSubsystem::EnterMapLoad()
{
    const int32 MapId = MyPlayerData->GetMapId();
    const TSoftObjectPtr<UWorld> Level = GetDefault<UP1GameDataSettings>()->FindMapLevel(MapId);
    if (Level.IsNull())
    {
        // 열 레벨이 없는데 남아 있으면 서버는 로드 완료를 기다리고 클라이언트는 아무 맵도 열지 않은 채 멈춘다.
        UE_LOG(LogP1System, Error, TEXT("맵 %d의 레벨이 데이터 설정(P1 Game Data)에 없어 연결을 끊는다"), MapId);
        Connection->Close();
        ReturnToLogin(UNKNOWN_MAP_NOTICE);
        return;
    }

    if (TryEnterPhase(EP1EntryPhase::MapLoad))
        OpenLevelNextTick(FName(*Level.GetLongPackageName()));
}

void UP1EntryFlowSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
    // PIE 창이 여럿이면 다른 창의 맵 로드도 여기로 온다.
    if (LoadedWorld == nullptr || LoadedWorld->GetGameInstance() != GetGameInstance())
        return;

    if (Phase != EP1EntryPhase::MapLoad)
        return;

    // 엔진은 이 맵의 BeginPlay를 마친 뒤에 이 알림을 보낸다. 서버는 이 알림을 받아야 플레이어를 룸에 넣고 스폰을 보낸다.
    SendMapLoadComplete();
    TryEnterPhase(EP1EntryPhase::Game);
}

void UP1EntryFlowSubsystem::HandleMyPlayerSpawned(AP1MyPlayer* MyPlayer)
{
    const EP1EntryProgress Before = GetProgress();
    bAwaitingMyPlayer = false;
    NotifyProgressIfChanged(Before);
}

void UP1EntryFlowSubsystem::SendMapLoadComplete()
{
    Protocol::C_MAP_LOAD_COMPLETE MapLoadCompletePkt;
    FP1PacketSender::Send(GetGameInstance(), MapLoadCompletePkt);
}

FString UP1EntryFlowSubsystem::ConsumeLoginNotice()
{
    return MoveTemp(PendingLoginNotice);
}

void UP1EntryFlowSubsystem::RemoveListener(const UObject* Listener)
{
    OnProgressChanged.RemoveAll(Listener);
    OnLoginNotice.RemoveAll(Listener);
}

void UP1EntryFlowSubsystem::HandleConnectionLost(Protocol::DisconnectReason Reason)
{
    switch (Reason)
    {
    case Protocol::DISCONNECT_REASON_DUPLICATE_CONNECTION:
        ReturnToLogin(TEXT("다른 곳에서 같은 계정으로 접속해 연결이 끊겼습니다."));
        break;
    default:
        ReturnToLogin(CONNECTION_LOST_NOTICE);
        break;
    }
}

void UP1EntryFlowSubsystem::ReturnToLogin(const FString& Notice)
{
    UE_LOG(LogP1Network, Warning, TEXT("게임 서버에 연결하지 못했거나 연결이 끊겨 로그인 화면으로 돌아간다: %s"), *Notice);

    // 문구를 알리기 전에 끈다. 로그인 화면이 진행 표시에 가려진 채 문구를 받지 않게 하려는 것이다.
    SetConnectingWorld(false);

    // 다음 입장 패킷이 다시 채운다. 비우지 않으면 다른 캐릭터로 들어가기 전까지 이전 캐릭터의 칸과 퀵슬롯이 남는다.
    MyPlayerData->ClearContainers();
    MyPlayerData->ClearQuickSlots();
    Lobby->Reset();

    // 아직 로그인 단계면 로그인 맵이 열려 있다. 맵을 다시 열지 않고 열린 화면에 알린다.
    if (Phase == EP1EntryPhase::Login)
    {
        OnLoginNotice.Broadcast(Notice);
        return;
    }

    // 티켓은 게임 서버가 한 번 쓰고 지웠다. 다시 들어가려면 인증 서버에 다시 로그인해야 하므로 다시 잇지 않는다.
    // 연결은 끊김을 알리기 전에 연결 서브시스템이 닫았다.
    TryEnterPhase(EP1EntryPhase::Login);
    PendingLoginNotice = Notice;
    OpenLevelNextTick(LOGIN_LEVEL_NAME);
}

void UP1EntryFlowSubsystem::OpenLevelNextTick(FName LevelName)
{
    // 한 틱 안에 두 번 불리면 나중 것만 연다.
    if (OpenLevelTickerHandle.IsValid())
        FTSTicker::RemoveTicker(OpenLevelTickerHandle);

    // 틱 도중에 더한 티커는 그 틱 안에서 곧바로 돈다. 패킷도 같은 틱에서 처리되므로, 프레임 번호가 바뀔 때까지 기다리지 않으면
    // 같은 프레임에 레벨을 열어 로딩 화면이 그려지기 전에 화면이 멈춘다.
    const uint64 ScheduledFrame = GFrameCounter;
    OpenLevelTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this, LevelName, ScheduledFrame](float)
    {
        if (GFrameCounter == ScheduledFrame)
            return true;

        OpenLevelTickerHandle.Reset();
        UGameplayStatics::OpenLevel(GetGameInstance()->GetWorld(), LevelName);
        return false;
    }));
}
