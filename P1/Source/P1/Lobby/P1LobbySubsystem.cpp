#include "Lobby/P1LobbySubsystem.h"
#include "Network/P1PacketSender.h"
#include "Lobby.pb.h"
#include "Protocol.pb.h"
#include "Utils/LogCategory.h"

static_assert(static_cast<int32>(EP1CharacterClass::Warrior) == Protocol::CLASS_TYPE_WARRIOR, "직업 값이 프로토콜과 같아야 한다");
static_assert(static_cast<int32>(EP1CharacterClass::Mage) == Protocol::CLASS_TYPE_MAGE, "직업 값이 프로토콜과 같아야 한다");

namespace
{
    /** 테스트가 바꾼 송신 함수가 있으면 그것으로, 없으면 게임 인스턴스의 연결로 보낸다. */
    template <typename T>
    void SendRequest(const UP1LobbySubsystem::FPacketSender& PacketSender, const UGameInstance* GameInstance, T& Pkt)
    {
        if (PacketSender)
        {
            PacketSender(ClientPacketHandler::MakeSerializedPacket(Pkt));
            return;
        }

        FP1PacketSender::Send(GameInstance, Pkt);
    }

    FP1LobbyCharacter ToLobbyCharacter(const Protocol::LobbyCharacter& Character)
    {
        FP1LobbyCharacter LobbyCharacter;
        LobbyCharacter.CharacterId = Character.character_id();
        LobbyCharacter.CharacterName = UTF8_TO_TCHAR(Character.name().c_str());
        LobbyCharacter.CharacterClass = static_cast<EP1CharacterClass>(Character.character_class());
        LobbyCharacter.CharacterLevel = Character.level();
        return LobbyCharacter;
    }
}

void UP1LobbySubsystem::RemoveListener(const UObject* Listener)
{
    OnLobbyEntered.RemoveAll(Listener);
    OnLobbyConnectFailed.RemoveAll(Listener);
    OnCharactersChanged.RemoveAll(Listener);
    OnCreateCharacterFailed.RemoveAll(Listener);
    OnDeleteCharacterFailed.RemoveAll(Listener);
    OnEnterGameFailed.RemoveAll(Listener);
    OnEnterGameSucceeded.RemoveAll(Listener);
}

void UP1LobbySubsystem::ConnectLobby(const FString& AuthTicket)
{
    // 티켓은 게임 서버가 한 번 쓰고 지운다. 다시 들어가려면 인증 서버에 다시 로그인해야 한다.
    Protocol::C_CONNECT_LOBBY Pkt;
    Pkt.set_auth_ticket(TCHAR_TO_UTF8(*AuthTicket));
    SendRequest(PacketSender, GetGameInstance(), Pkt);
}

void UP1LobbySubsystem::Reset()
{
    Characters.Reset();
    CharacterSlotCount = 0;
}

void UP1LobbySubsystem::CreateCharacter(const FString& CharacterName, EP1CharacterClass CharacterClass)
{
    Protocol::C_CREATE_CHARACTER Pkt;
    Pkt.set_name(TCHAR_TO_UTF8(*CharacterName));
    Pkt.set_character_class(static_cast<Protocol::CharacterClass>(CharacterClass));
    SendRequest(PacketSender, GetGameInstance(), Pkt);
}

void UP1LobbySubsystem::DeleteCharacter(int64 CharacterId)
{
    Protocol::C_DELETE_CHARACTER Pkt;
    Pkt.set_character_id(CharacterId);
    SendRequest(PacketSender, GetGameInstance(), Pkt);
}

void UP1LobbySubsystem::SelectCharacter(int64 CharacterId)
{
    Protocol::C_ENTER_GAME Pkt;
    Pkt.set_character_id(CharacterId);
    SendRequest(PacketSender, GetGameInstance(), Pkt);
}

FText UP1LobbySubsystem::GetResultText(Protocol::LobbyResult Result)
{
    // 서버는 문구를 보내지 않는다. 결과 값을 더하면 여기에도 문구를 더한다. 빠뜨리면 UE 테스트가 알린다.
    switch (Result)
    {
    case Protocol::LOBBY_RESULT_SUCCESS:
        return FText::FromString(TEXT("요청을 처리했습니다."));
    case Protocol::LOBBY_RESULT_INVALID_AUTH_TICKET:
        return FText::FromString(TEXT("로그인 정보가 만료되었습니다. 다시 로그인하세요."));
    case Protocol::LOBBY_RESULT_INVALID_NAME:
        return FText::FromString(TEXT("캐릭터 이름은 1자 이상 32자 이하여야 합니다."));
    case Protocol::LOBBY_RESULT_NAME_TAKEN:
        return FText::FromString(TEXT("이미 사용 중인 캐릭터 이름입니다."));
    case Protocol::LOBBY_RESULT_UNAVAILABLE_CLASS:
        return FText::FromString(TEXT("선택할 수 없는 직업입니다."));
    case Protocol::LOBBY_RESULT_NO_EMPTY_SLOT:
        return FText::FromString(TEXT("빈 캐릭터 슬롯이 없습니다."));
    case Protocol::LOBBY_RESULT_CHARACTER_NOT_FOUND:
        return FText::FromString(TEXT("캐릭터를 찾을 수 없습니다."));
    case Protocol::LOBBY_RESULT_INTERNAL_ERROR:
        return FText::FromString(TEXT("서버 오류로 요청을 처리하지 못했습니다."));
    default:
        break;
    }

    // 클라이언트보다 새 서버가 보낸 값이다.
    return FText::FromString(FString::Printf(TEXT("알 수 없는 오류입니다. (%d)"), static_cast<int32>(Result)));
}

void UP1LobbySubsystem::HandleConnectLobby(const Protocol::S_CONNECT_LOBBY& ConnectLobbyPkt)
{
    // 성공하면 캐릭터 목록이 뒤따르고, 목록을 받을 때 로비에 들어왔다고 알린다.
    if (ConnectLobbyPkt.result() == Protocol::LOBBY_RESULT_SUCCESS)
        return;

    UE_LOG(LogP1Lobby, Warning, TEXT("서버가 로비 접속을 거절함. 결과: %d"), static_cast<int32>(ConnectLobbyPkt.result()));
    OnLobbyConnectFailed.Broadcast(GetResultText(ConnectLobbyPkt.result()));
}

void UP1LobbySubsystem::HandleCharacterList(const Protocol::S_CHARACTER_LIST& CharacterListPkt)
{
    Characters.Reset();
    for (const Protocol::LobbyCharacter& Character : CharacterListPkt.characters())
        Characters.Add(ToLobbyCharacter(Character));

    CharacterSlotCount = CharacterListPkt.character_slot_count();

    OnLobbyEntered.Broadcast();
    OnCharactersChanged.Broadcast();
}

void UP1LobbySubsystem::HandleCreateCharacter(const Protocol::S_CREATE_CHARACTER& CreateCharacterPkt)
{
    if (CreateCharacterPkt.result() != Protocol::LOBBY_RESULT_SUCCESS)
    {
        OnCreateCharacterFailed.Broadcast(GetResultText(CreateCharacterPkt.result()));
        return;
    }

    // 응답이 만든 캐릭터를 싣는다. 목록 끝에 더하는 것은 서버의 목록 순서(만든 순서)와 같다.
    Characters.Add(ToLobbyCharacter(CreateCharacterPkt.character()));

    OnCharactersChanged.Broadcast();
}

void UP1LobbySubsystem::HandleDeleteCharacter(const Protocol::S_DELETE_CHARACTER& DeleteCharacterPkt)
{
    if (DeleteCharacterPkt.result() != Protocol::LOBBY_RESULT_SUCCESS)
    {
        OnDeleteCharacterFailed.Broadcast(GetResultText(DeleteCharacterPkt.result()));
        return;
    }

    const int64 CharacterId = DeleteCharacterPkt.character_id();
    Characters.RemoveAll([CharacterId](const FP1LobbyCharacter& Character) { return Character.CharacterId == CharacterId; });

    OnCharactersChanged.Broadcast();
}

void UP1LobbySubsystem::HandleEnterGame(bool bSuccess)
{
    // 거절되면 아직 캐릭터 선택 화면이므로 그 화면에 알린다.
    if (bSuccess == false)
    {
        UE_LOG(LogP1Lobby, Warning, TEXT("서버가 게임 입장을 거절함"));
        OnEnterGameFailed.Broadcast();
        return;
    }

    OnEnterGameSucceeded.Broadcast();
}
