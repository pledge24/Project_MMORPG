// 로그인 계열 S_* 핸들러의 정의다. 선언은 생성된 Network/ClientPacketHandler.h에 있다.
// Network/는 Online/을 부르지 않으므로, 로그인 관리자를 부르는 핸들러는 여기 둔다.
#include "Network/ClientPacketHandler.h"
#include "Network/PacketSession.h"
#include "Online/P1LoginManager.h"
#include "Engine/GameInstance.h"

namespace
{
    /** 세션을 연 게임 인스턴스의 로그인 관리자다. 월드나 게임 인스턴스가 없으면 nullptr. */
    UP1LoginManager* GetLoginManager(const PacketSessionRef& session)
    {
        if (UWorld* World = session->GetWorld())
        {
            if (UGameInstance* GameInstance = World->GetGameInstance())
                return GameInstance->GetSubsystem<UP1LoginManager>();
        }

        return nullptr;
    }
}

bool Handle_S_LOGIN(PacketSessionRef& session, Protocol::S_LOGIN& pkt)
{
    if (auto* LoginManager = GetLoginManager(session))
    {
        LoginManager->HandleLogin(pkt);
        return true;
    }

    return false;
}

bool Handle_S_CREATE_CHARACTER(PacketSessionRef& session, Protocol::S_CREATE_CHARACTER& pkt)
{
    if (auto* LoginManager = GetLoginManager(session))
    {
        LoginManager->HandleCreateCharacter(pkt);
        return true;
    }

    return false;
}

bool Handle_S_DELETE_CHARACTER(PacketSessionRef& session, Protocol::S_DELETE_CHARACTER& pkt)
{
    if (auto* LoginManager = GetLoginManager(session))
    {
        LoginManager->HandleDeleteCharacter(pkt);
        return true;
    }

    return false;
}

bool Handle_S_ENTER_GAME(PacketSessionRef& session, Protocol::S_ENTER_GAME& pkt)
{
    if (auto* LoginManager = GetLoginManager(session))
    {
        LoginManager->HandleEnterGame(pkt);
        return true;
    }

    return false;
}
