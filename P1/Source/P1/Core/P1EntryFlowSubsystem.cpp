#include "Core/P1EntryFlowSubsystem.h"
#include "Auth/P1AuthClient.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Lobby/P1LobbySubsystem.h"
#include "Network/P1ConnectionSubsystem.h"
#include "Network/P1NetworkSettings.h"
#include "Network/P1PacketSender.h"
#include "Protocol.pb.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"
#include "Utils/LogCategory.h"

namespace
{
    const FName LOGIN_LEVEL_NAME(TEXT("L_LoginMap"));
    const FName IN_GAME_LEVEL_NAME(TEXT("L_InGameMap"));

    /** 사유를 모르는 끊김에 보여 줄 문구. 서버가 먼저 내려갔거나 사유 패킷을 잃은 경우다. */
    const TCHAR* const CONNECTION_LOST_NOTICE = TEXT("게임 서버와 연결이 끊겼습니다.");

    const TCHAR* const CONNECT_FAILED_NOTICE = TEXT("게임 서버에 접속하지 못했습니다.");

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
        Lobby->OnEnterGameSucceeded.AddUObject(this, &UP1EntryFlowSubsystem::EnterMapLoad);
    }

    if (MyPlayerData)
        MyPlayerData->OnMapEntered.AddUObject(this, &UP1EntryFlowSubsystem::EnterMapLoad);

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
    Phase = NewPhase;
    return true;
}

void UP1EntryFlowSubsystem::HandleLoginSucceeded(const FString& AuthTicket)
{
    // 월드 선택이 생기기 전까지는 설정의 주소로 접속한다.
    const UP1NetworkSettings* NetworkSettings = GetDefault<UP1NetworkSettings>();

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
    TryEnterPhase(EP1EntryPhase::Lobby);
}

void UP1EntryFlowSubsystem::EnterMapLoad()
{
    if (TryEnterPhase(EP1EntryPhase::MapLoad))
        OpenLevelNextTick(IN_GAME_LEVEL_NAME);
}

void UP1EntryFlowSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
    // PIE 창이 여럿이면 다른 창의 맵 로드도 여기로 온다.
    if (LoadedWorld == nullptr || LoadedWorld->GetGameInstance() != GetGameInstance())
        return;

    if (Phase != EP1EntryPhase::MapLoad)
        return;

    // 엔진은 이 맵의 BeginPlay를 마친 뒤에 이 알림을 보낸다. 그래서 인게임 컨트롤러의 BeginPlay에서 보내던 때와 같은 시점이다.
    SendEnterRoom();
    TryEnterPhase(EP1EntryPhase::Game);
}

void UP1EntryFlowSubsystem::SendEnterRoom()
{
    Protocol::C_ENTER_ROOM EnterRoomPkt;
    EnterRoomPkt.set_enter_type(Protocol::ENTER_TYPE_CROSS_MAP_TRANSFER);
    EnterRoomPkt.set_room_id(MyPlayerData->GetRoomId());

    FP1PacketSender::Send(GetGameInstance(), EnterRoomPkt);
}

FString UP1EntryFlowSubsystem::ConsumeLoginNotice()
{
    return MoveTemp(PendingLoginNotice);
}

void UP1EntryFlowSubsystem::RemoveListener(const UObject* Listener)
{
    OnLoginNotice.RemoveAll(Listener);
}

void UP1EntryFlowSubsystem::HandleConnectionLost(Protocol::LeaveReason Reason)
{
    switch (Reason)
    {
    case Protocol::LEAVE_REASON_DUPLICATE_LOGIN:
        ReturnToLogin(TEXT("다른 곳에서 같은 계정으로 로그인해 연결이 끊겼습니다."));
        break;
    case Protocol::LEAVE_REASON_INVALID_TOKEN:
        ReturnToLogin(TEXT("로그인 정보가 만료되었습니다. 다시 로그인하세요."));
        break;
    default:
        ReturnToLogin(CONNECTION_LOST_NOTICE);
        break;
    }
}

void UP1EntryFlowSubsystem::ReturnToLogin(const FString& Notice)
{
    UE_LOG(LogP1Network, Warning, TEXT("게임 서버에 연결하지 못했거나 연결이 끊겨 로그인 화면으로 돌아간다: %s"), *Notice);

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

    OpenLevelTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this, LevelName](float)
    {
        OpenLevelTickerHandle.Reset();
        UGameplayStatics::OpenLevel(GetGameInstance()->GetWorld(), LevelName);
        return false;
    }));
}
