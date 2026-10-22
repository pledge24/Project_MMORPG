#include "Lobby/P1LobbySubsystem.h"
#include "Network/P1PacketSender.h"
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
}

void UP1LobbySubsystem::RemoveListener(const UObject* Listener)
{
    OnLobbyEntered.RemoveAll(Listener);
    OnCharactersChanged.RemoveAll(Listener);
    OnCharacterListFailed.RemoveAll(Listener);
    OnCreateCharacterFailed.RemoveAll(Listener);
    OnDeleteCharacterFailed.RemoveAll(Listener);
    OnEnterGameFailed.RemoveAll(Listener);
    OnEnterGameSucceeded.RemoveAll(Listener);
}

void UP1LobbySubsystem::ConnectLobby(const FString& AuthTicket)
{
    // 티켓은 게임 서버가 한 번 쓰고 지운다. 다시 들어가려면 인증 서버에 다시 로그인해야 한다.
    Protocol::C_LOGIN Pkt;
    Pkt.set_access_token(TCHAR_TO_UTF8(*AuthTicket));
    SendRequest(PacketSender, GetGameInstance(), Pkt);
}

void UP1LobbySubsystem::Reset()
{
    Characters.Reset();
    CharacterSlotCount = 0;
    PendingCreate.Reset();
}

void UP1LobbySubsystem::CreateCharacter(const FString& CharacterName, EP1CharacterClass CharacterClass)
{
    Protocol::CharacterOverview* CharacterOverview = new Protocol::CharacterOverview();
    CharacterOverview->set_class_(static_cast<Protocol::CharacterClass>(CharacterClass));
    CharacterOverview->set_name(TCHAR_TO_UTF8(*CharacterName));

    Protocol::C_CREATE_CHARACTER Pkt;
    Pkt.set_allocated_character(CharacterOverview);

    // 응답을 기다리는 동안 다시 요청하면 마지막 요청의 값으로 덮는다. 위젯이 들고 있던 때와 같은 동작이다.
    PendingCreate = FPendingCreate{ CharacterName, CharacterClass };

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

void UP1LobbySubsystem::HandleCharacterList(const Protocol::S_LOGIN& LoginPkt)
{
    // 실패 응답은 서버가 비워서 보내므로 슬롯 수가 0이다. 사본에 옮기면 슬롯이 모두 숨은 선택 화면이 뜬다.
    if (LoginPkt.success() == false)
    {
        UE_LOG(LogP1Lobby, Warning, TEXT("서버가 캐릭터 목록을 불러오지 못함"));
        OnCharacterListFailed.Broadcast();
        return;
    }

    Characters.Reset();
    for (const Protocol::CharacterOverview& Character : LoginPkt.characters())
    {
        FP1LobbyCharacter& LobbyCharacter = Characters.AddDefaulted_GetRef();
        LobbyCharacter.CharacterId = Character.character_id();
        LobbyCharacter.CharacterName = UTF8_TO_TCHAR(Character.name().c_str());
        LobbyCharacter.CharacterClass = static_cast<EP1CharacterClass>(Character.class_());
        LobbyCharacter.CharacterLevel = Character.level();
    }

    CharacterSlotCount = LoginPkt.character_slot_count();

    OnLobbyEntered.Broadcast();
    OnCharactersChanged.Broadcast();
}

void UP1LobbySubsystem::HandleCreateCharacter(const Protocol::S_CREATE_CHARACTER& CreateCharacterPkt)
{
    const TOptional<FPendingCreate> Request = MoveTemp(PendingCreate);
    PendingCreate.Reset();

    if (CreateCharacterPkt.success() == false)
    {
        OnCreateCharacterFailed.Broadcast(UTF8_TO_TCHAR(CreateCharacterPkt.cause().c_str()));
        return;
    }

    if (Request.IsSet() == false)
    {
        UE_LOG(LogP1Lobby, Warning, TEXT("보내지 않은 생성 요청의 응답을 받음. character_id: %lld"), CreateCharacterPkt.character_id());
    }

    // 새 캐릭터는 1레벨에서 시작한다.
    FP1LobbyCharacter& LobbyCharacter = Characters.AddDefaulted_GetRef();
    LobbyCharacter.CharacterId = CreateCharacterPkt.character_id();
    LobbyCharacter.CharacterLevel = 1;
    if (Request.IsSet())
    {
        LobbyCharacter.CharacterName = Request->CharacterName;
        LobbyCharacter.CharacterClass = Request->CharacterClass;
    }

    OnCharactersChanged.Broadcast();
}

void UP1LobbySubsystem::HandleDeleteCharacter(const Protocol::S_DELETE_CHARACTER& DeleteCharacterPkt)
{
    if (DeleteCharacterPkt.success() == false)
    {
        OnDeleteCharacterFailed.Broadcast();
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
