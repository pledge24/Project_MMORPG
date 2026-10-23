#include "Core/P1EntryFlowSubsystem.h"
#include "Auth/P1AuthClient.h"
#include "Game/Chat/P1ChatSubsystem.h"
#include "Game/Data/P1GameDataSettings.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Lobby/P1LobbySubsystem.h"
#include "Network/P1ConnectionSubsystem.h"
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

    /** 로비 복귀의 저장은 끝났지만 서버가 캐릭터 목록을 불러오지 못했을 때 보여 줄 문구. 서버는 연결을 끊는다. */
    const TCHAR* const RETURN_TO_LOBBY_FAILED_NOTICE = TEXT("캐릭터 목록을 불러오지 못했습니다. 다시 로그인해 주세요.");

    /** 서버가 이 클라이언트에 없는 맵으로 보냈을 때 보여 줄 문구. 클라이언트와 서버의 기획 데이터가 어긋난 경우다. */
    const TCHAR* const UNKNOWN_MAP_NOTICE = TEXT("이 클라이언트에 없는 맵입니다. 클라이언트를 최신으로 업데이트해 주세요.");

    const TCHAR* PhaseName(EP1EntryPhase Phase)
    {
        switch (Phase)
        {
        case EP1EntryPhase::Login: return TEXT("Login");
        case EP1EntryPhase::WorldSelect: return TEXT("WorldSelect");
        case EP1EntryPhase::Lobby: return TEXT("Lobby");
        case EP1EntryPhase::MapLoad: return TEXT("MapLoad");
        case EP1EntryPhase::Game: return TEXT("Game");
        case EP1EntryPhase::ReturningToLobby: return TEXT("ReturningToLobby");
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
    Chat = Collection.InitializeDependency<UP1ChatSubsystem>();

    if (AuthClient)
    {
        AuthClient->OnLoginSucceeded.AddUObject(this, &UP1EntryFlowSubsystem::HandleLoginSucceeded);
        AuthClient->OnWorldSelected.AddUObject(this, &UP1EntryFlowSubsystem::HandleWorldSelected);
        AuthClient->OnWorldSelectFailed.AddUObject(this, &UP1EntryFlowSubsystem::ShowWorldSelectNotice);
        AuthClient->OnLoginSessionExpired.AddUObject(this, &UP1EntryFlowSubsystem::HandleLoginSessionExpired);
    }

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
        Lobby->OnReturnToLobbyFailed.AddUObject(this, &UP1EntryFlowSubsystem::HandleReturnToLobbyFailed);
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
        // 연결이 끊기거나 로그인 세션이 만료되면 어느 단계에서든 돌아온다.
        return From != EP1EntryPhase::Login;
    case EP1EntryPhase::WorldSelect:
        // 로비나 게임에서 월드 선택으로 돌아갈 때는 다시 로그인하지 않는다.
        return From == EP1EntryPhase::Login || From == EP1EntryPhase::Lobby || From == EP1EntryPhase::Game;
    case EP1EntryPhase::Lobby:
        return From == EP1EntryPhase::WorldSelect || From == EP1EntryPhase::ReturningToLobby;
    case EP1EntryPhase::MapLoad:
        // 게임에서 맵 로드로 가는 것은 맵을 옮길 때다.
        return From == EP1EntryPhase::Lobby || From == EP1EntryPhase::Game;
    case EP1EntryPhase::Game:
        return From == EP1EntryPhase::MapLoad;
    case EP1EntryPhase::ReturningToLobby:
        return From == EP1EntryPhase::Game;
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

    // 맵을 옮길 때마다 새 맵의 스폰을 기다린다. 게임을 떠나면 기다릴 스폰이 없다.
    if (NewPhase == EP1EntryPhase::MapLoad)
        bAwaitingMyPlayer = true;
    else if (NewPhase != EP1EntryPhase::Game)
        bAwaitingMyPlayer = false;

    OnPhaseChanged.Broadcast(NewPhase);
    NotifyProgressIfChanged(Before);
    return true;
}

EP1EntryProgress UP1EntryFlowSubsystem::GetProgress() const
{
    if (Phase == EP1EntryPhase::MapLoad)
        return EP1EntryProgress::LoadingMap;

    if (Phase == EP1EntryPhase::Game && bAwaitingMyPlayer)
        return EP1EntryProgress::LoadingMap;

    if (Phase == EP1EntryPhase::ReturningToLobby)
        return EP1EntryProgress::ReturningToLobby;

    // 게임에서 돌아가며 로그인 맵을 여는 동안이다. 맵이 열리기 전까지는 게임 맵이 그대로 보인다.
    if (bOpeningLevel && bLoginMapOpen)
    {
        if (Phase == EP1EntryPhase::Lobby)
            return EP1EntryProgress::ReturningToLobby;
        if (Phase == EP1EntryPhase::WorldSelect)
            return EP1EntryProgress::ReturningToWorldSelect;
    }

    if (Phase == EP1EntryPhase::WorldSelect && bConnectingWorld)
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

void UP1EntryFlowSubsystem::SelectWorld(int32 WorldId)
{
    if (Phase != EP1EntryPhase::WorldSelect || bConnectingWorld)
        return;

    // 월드 선택 응답을 기다리는 동안에도 접속 중으로 보인다. 그사이 다른 월드를 또 고르지 못하게 한다.
    SetConnectingWorld(true);
    AuthClient->RequestSelectWorld(WorldId);
}

void UP1EntryFlowSubsystem::Logout()
{
    if (Phase != EP1EntryPhase::WorldSelect || bConnectingWorld)
        return;

    AuthClient->Logout();
    TryEnterPhase(EP1EntryPhase::Login);
}

void UP1EntryFlowSubsystem::ReturnToWorldSelect()
{
    if (Phase != EP1EntryPhase::Lobby && Phase != EP1EntryPhase::Game)
        return;

    // 연결을 닫으면 서버는 접속 종료 경로를 탄다. 닫기는 끊김을 알리지 않으므로 로그인으로 돌아가지 않는다.
    Connection->Close();
    Lobby->Reset();

    if (Phase == EP1EntryPhase::Lobby)
    {
        TryEnterPhase(EP1EntryPhase::WorldSelect);

        // 로비에 있는 동안 월드의 혼잡도가 바뀌었거나 월드가 꺼졌을 수 있다.
        AuthClient->RequestWorlds();
        return;
    }

    // 레벨을 먼저 정해 두고 단계를 바꾼다. 반대로 하면 진행 표시가 없음으로 한 번 바뀌어 로딩 화면이 사라지기 시작한다.
    // 월드 목록은 로그인 맵이 열린 뒤에 받는다(HandlePostLoadMap). 그 전에 로그인 세션 만료를 받으면 알릴 화면이 없다.
    ClearMyPlayerData();
    OpenLevelNextTick(LOGIN_LEVEL_NAME);
    TryEnterPhase(EP1EntryPhase::WorldSelect);
}

void UP1EntryFlowSubsystem::HandleLoginSucceeded()
{
    TryEnterPhase(EP1EntryPhase::WorldSelect);
}

void UP1EntryFlowSubsystem::HandleWorldSelected(const FString& Host, int32 Port, const FString& AuthTicket)
{
    // 응답을 기다리는 사이 로그인으로 돌아갔으면 티켓을 쓰지 않는다. 티켓은 유효 시간이 지나면 저절로 사라진다.
    if (Phase != EP1EntryPhase::WorldSelect || bConnectingWorld == false)
        return;

    // 연결은 Connect 안에서 끝나고 그 안에서 결과를 알리므로, 부르기 전에 티켓을 맡겨 둔다.
    PendingAuthTicket = AuthTicket;
    Connection->Connect(Host, Port);
    PendingAuthTicket.Reset();
}

void UP1EntryFlowSubsystem::HandleLoginSessionExpired(const FString& Message)
{
    // 로그인 세션은 로그인 메뉴에서만 쓴다. 게임 중에는 만료돼도 게임 서버와의 연결이 그대로다.
    if (IsLoginMenuPhase(Phase))
        ReturnToLogin(Message);
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
    ShowWorldSelectNotice(CONNECT_FAILED_NOTICE);
}

void UP1EntryFlowSubsystem::HandleLobbyEntered()
{
    // 로비 복귀의 저장이 끝나 목록이 왔다. 레벨을 먼저 정해 두는 이유는 ReturnToWorldSelect와 같다.
    if (Phase == EP1EntryPhase::ReturningToLobby)
    {
        ClearMyPlayerData();
        OpenLevelNextTick(LOGIN_LEVEL_NAME);
        TryEnterPhase(EP1EntryPhase::Lobby);
        return;
    }

    SetConnectingWorld(false);
    TryEnterPhase(EP1EntryPhase::Lobby);
}

void UP1EntryFlowSubsystem::HandleReturnToLobbyFailed()
{
    // 서버가 이 응답 뒤에 끊는다. 먼저 닫지 않으면 끊김이 사유 없는 끊김으로 와서 이 문구를 덮는다.
    Connection->Close();
    ReturnToLogin(RETURN_TO_LOBBY_FAILED_NOTICE);
}

void UP1EntryFlowSubsystem::HandleLobbyConnectFailed(const FText& Message)
{
    // 서버가 이 응답 뒤에 끊는다. 먼저 닫지 않으면 끊김이 사유 없는 끊김으로 와서 결과의 문구를 덮는다.
    // 티켓은 게임 서버가 이미 지웠지만, 로그인 세션이 남아 있으므로 월드를 다시 고르면 새 티켓을 받는다.
    Connection->Close();
    ShowWorldSelectNotice(Message.ToString());
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

    const EP1EntryProgress Before = GetProgress();
    bOpeningLevel = false;
    NotifyProgressIfChanged(Before);

    // 게임에서 월드 선택으로 돌아왔다. 게임에 있는 동안 월드 목록이 바뀌었을 수 있고, 로그인 세션이 만료됐으면 여기서 알게 된다.
    if (bLoginMapOpen && Phase == EP1EntryPhase::WorldSelect)
    {
        AuthClient->RequestWorlds();
        return;
    }

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

void UP1EntryFlowSubsystem::ReturnToLobby()
{
    if (Phase != EP1EntryPhase::Game)
        return;

    // 서버는 받자마자 룸에서 퇴장시키므로 게임 맵이 남아 있어도 더는 게임이 아니다. 단계를 먼저 바꿔 두 번 보내지 않는다.
    TryEnterPhase(EP1EntryPhase::ReturningToLobby);

    Protocol::C_RETURN_TO_LOBBY ReturnToLobbyPkt;
    FP1PacketSender::Send(GetGameInstance(), ReturnToLobbyPkt);
}

void UP1EntryFlowSubsystem::ClearMyPlayerData()
{
    // 비우지 않으면 다른 캐릭터로 들어가기 전까지 이전 캐릭터의 칸과 퀵슬롯이 남는다.
    MyPlayerData->ClearContainers();
    MyPlayerData->ClearQuickSlots();

    // 채팅은 캐릭터를 따라가지 않는다. 비우지 않으면 다른 캐릭터의 채팅창에 이전 캐릭터의 대화가 남는다.
    if (Chat)
        Chat->ClearLog();
}

FString UP1EntryFlowSubsystem::ConsumeLoginNotice()
{
    return MoveTemp(PendingLoginNotice);
}

void UP1EntryFlowSubsystem::RemoveListener(const UObject* Listener)
{
    OnProgressChanged.RemoveAll(Listener);
    OnPhaseChanged.RemoveAll(Listener);
    OnNotice.RemoveAll(Listener);
}

void UP1EntryFlowSubsystem::HandleConnectionLost(Protocol::DisconnectReason Reason)
{
    // 로비 접속 응답을 기다리다 끊기면 월드를 다시 고르게 한다.
    if (Phase == EP1EntryPhase::WorldSelect)
    {
        ShowWorldSelectNotice(CONNECTION_LOST_NOTICE);
        return;
    }

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
    UE_LOG(LogP1Network, Warning, TEXT("로그인 화면으로 돌아간다: %s"), *Notice);

    // 문구를 알리기 전에 끈다. 로그인 화면이 진행 표시에 가려진 채 문구를 받지 않게 하려는 것이다.
    SetConnectingWorld(false);

    ClearMyPlayerData();
    Lobby->Reset();

    // 로그인 화면으로 가면 다시 로그인한다. 남은 로그인 세션은 쓰지 않으므로 버린다.
    // 끊김으로 왔으면 연결은 이미 닫혔다. 로그인 세션 만료처럼 연결과 무관하게 왔으면 여기서 닫는다.
    // 닫힌 연결을 다시 닫아도 아무 일도 없고, 닫기는 끊김을 알리지 않는다.
    AuthClient->Logout();
    Connection->Close();

    // 로그인 맵이 열려 있으면 맵을 다시 열지 않고 열린 화면에 알린다. 여는 중이면 아직 화면이 없으므로 열린 뒤에 보여 준다.
    if (bLoginMapOpen)
    {
        if (Phase != EP1EntryPhase::Login)
            TryEnterPhase(EP1EntryPhase::Login);

        if (bOpeningLevel)
            PendingLoginNotice = Notice;
        else if (Notice.IsEmpty() == false)
            OnNotice.Broadcast(Notice);
        return;
    }

    TryEnterPhase(EP1EntryPhase::Login);
    PendingLoginNotice = Notice;
    OpenLevelNextTick(LOGIN_LEVEL_NAME);
}

void UP1EntryFlowSubsystem::ShowWorldSelectNotice(const FString& Notice)
{
    if (Phase != EP1EntryPhase::WorldSelect)
        return;

    UE_LOG(LogP1Network, Warning, TEXT("월드에 접속하지 못해 월드 선택 화면에 남는다: %s"), *Notice);

    // 문구를 알리기 전에 끈다. 월드 선택 화면이 진행 표시에 가려진 채 문구를 받지 않게 하려는 것이다.
    SetConnectingWorld(false);
    Lobby->Reset();
    OnNotice.Broadcast(Notice);
}

bool UP1EntryFlowSubsystem::IsLoginMenuPhase(EP1EntryPhase InPhase)
{
    return InPhase == EP1EntryPhase::Login || InPhase == EP1EntryPhase::WorldSelect || InPhase == EP1EntryPhase::Lobby;
}

void UP1EntryFlowSubsystem::OpenLevelNextTick(FName LevelName)
{
    // 한 틱 안에 두 번 불리면 나중 것만 연다.
    if (OpenLevelTickerHandle.IsValid())
        FTSTicker::RemoveTicker(OpenLevelTickerHandle);

    const EP1EntryProgress Before = GetProgress();
    bLoginMapOpen = LevelName == LOGIN_LEVEL_NAME;
    bOpeningLevel = true;
    NotifyProgressIfChanged(Before);

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
