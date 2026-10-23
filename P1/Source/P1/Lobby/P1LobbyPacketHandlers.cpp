// 로비 계열 S_* 핸들러의 정의다. 선언은 생성된 Network/ClientPacketHandler.h에 있다.
// Network/는 Lobby/를 부르지 않으므로, 로비 서브시스템을 부르는 핸들러는 여기 둔다.
#include "Network/ClientPacketHandler.h"
#include "Network/PacketSession.h"
#include "Lobby/P1LobbySubsystem.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace
{
    /** 세션을 연 게임 인스턴스의 서브시스템. 월드나 게임 인스턴스가 없으면 nullptr. */
    template <typename T>
    T* GetSessionSubsystem(const PacketSessionRef& session)
    {
        if (UWorld* World = session->GetWorld())
        {
            if (UGameInstance* GameInstance = World->GetGameInstance())
                return GameInstance->GetSubsystem<T>();
        }

        return nullptr;
    }
}

bool Handle_S_CONNECT_LOBBY(PacketSessionRef& session, Protocol::S_CONNECT_LOBBY& pkt)
{
    if (auto* Lobby = GetSessionSubsystem<UP1LobbySubsystem>(session))
    {
        Lobby->HandleConnectLobby(pkt);
        return true;
    }

    return false;
}

bool Handle_S_CHARACTER_LIST(PacketSessionRef& session, Protocol::S_CHARACTER_LIST& pkt)
{
    if (auto* Lobby = GetSessionSubsystem<UP1LobbySubsystem>(session))
    {
        Lobby->HandleCharacterList(pkt);
        return true;
    }

    return false;
}

bool Handle_S_CREATE_CHARACTER(PacketSessionRef& session, Protocol::S_CREATE_CHARACTER& pkt)
{
    if (auto* Lobby = GetSessionSubsystem<UP1LobbySubsystem>(session))
    {
        Lobby->HandleCreateCharacter(pkt);
        return true;
    }

    return false;
}

bool Handle_S_DELETE_CHARACTER(PacketSessionRef& session, Protocol::S_DELETE_CHARACTER& pkt)
{
    if (auto* Lobby = GetSessionSubsystem<UP1LobbySubsystem>(session))
    {
        Lobby->HandleDeleteCharacter(pkt);
        return true;
    }

    return false;
}

bool Handle_S_ENTER_GAME(PacketSessionRef& session, Protocol::S_ENTER_GAME& pkt)
{
    UP1LobbySubsystem* Lobby = GetSessionSubsystem<UP1LobbySubsystem>(session);
    UP1MyPlayerData* MyPlayerData = GetSessionSubsystem<UP1MyPlayerData>(session);
    if (Lobby == nullptr || MyPlayerData == nullptr)
        return false;

    // 입장한 캐릭터의 개인 정보와 열 맵은 내 플레이어 데이터가 갖는다. 입장 성공을 알리면 맵을 열기 시작하므로 그 전에 채운다.
    const bool bSuccess = pkt.result() == Protocol::ENTRY_RESULT_SUCCESS;
    if (bSuccess)
        MyPlayerData->InitMyPlayerData(pkt);

    Lobby->HandleEnterGame(bSuccess);
    return true;
}

bool Handle_S_RETURN_TO_LOBBY(PacketSessionRef& session, Protocol::S_RETURN_TO_LOBBY& pkt)
{
    if (auto* Lobby = GetSessionSubsystem<UP1LobbySubsystem>(session))
    {
        Lobby->HandleReturnToLobby(pkt.result() == Protocol::ENTRY_RESULT_SUCCESS);
        return true;
    }

    return false;
}
