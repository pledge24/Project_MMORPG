#include "Core/P1GameInstance.h"

#include "Network/P1ConnectionSubsystem.h"
#include "Online/P1LoginManager.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/LogCategory.h"

namespace
{
    /** 사유를 모르는 끊김에 보여 줄 문구다. 서버가 먼저 내려갔거나 사유 패킷을 잃은 경우다. */
    const TCHAR* const CONNECTION_LOST_NOTICE = TEXT("게임 서버와 연결이 끊겼습니다.");
}

UP1GameInstance::UP1GameInstance()
{
}

void UP1GameInstance::Init()
{
    Super::Init();

    if (UP1LoginManager* LoginManager = GetSubsystem<UP1LoginManager>())
        LoginManager->OnEnterGameSucceeded.AddUObject(this, &UP1GameInstance::OpenInGameMap);
    else
        UE_LOG(LogP1System, Warning, TEXT("로그인 관리자 서브시스템을 찾지 못했다"));

    if (UP1MyPlayerData* MyPlayerData = GetSubsystem<UP1MyPlayerData>())
        MyPlayerData->OnMapEntered.AddUObject(this, &UP1GameInstance::OpenInGameMap);
    else
        UE_LOG(LogP1System, Warning, TEXT("내 플레이어 데이터 서브시스템을 찾지 못했다"));

    if (UP1ConnectionSubsystem* Connection = GetSubsystem<UP1ConnectionSubsystem>())
        Connection->OnConnectionLost.AddUObject(this, &UP1GameInstance::HandleConnectionLost);
    else
        UE_LOG(LogP1System, Warning, TEXT("연결 서브시스템을 찾지 못했다"));
}

void UP1GameInstance::OpenInGameMap()
{
    // 임시
    UGameplayStatics::OpenLevel(GetWorld(), FName("L_InGameMap"));
}

FString UP1GameInstance::ConsumeLoginNotice()
{
    return MoveTemp(PendingLoginNotice);
}

void UP1GameInstance::HandleConnectionLost(Protocol::LeaveReason Reason)
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

void UP1GameInstance::ReturnToLogin(const FString& Notice)
{
    UE_LOG(LogP1Network, Warning, TEXT("게임 서버 연결이 끊겨 로그인 화면으로 돌아간다: %s"), *Notice);

    // 토큰은 게임 서버가 한 번 쓰고 지웠다. 다시 들어가려면 인증 서버에 다시 로그인해야 하므로 다시 잇지 않는다.
    // 연결은 끊김을 알리기 전에 연결 서브시스템이 닫았다.
    PendingLoginNotice = Notice;
    UGameplayStatics::OpenLevel(GetWorld(), FName("L_LoginMap"));
}
