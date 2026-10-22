#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Lobby/P1LobbyCharacter.h"
#include "Utils/Types.h"
#include "P1LobbySubsystem.generated.h"

// 화면이 이 헤더를 부르므로 protobuf 헤더를 끌고 오지 않는다.
namespace Protocol
{
    class S_LOGIN;
    class S_CREATE_CHARACTER;
    class S_DELETE_CHARACTER;
}

DECLARE_MULTICAST_DELEGATE(FOnLobbyEntered);
DECLARE_MULTICAST_DELEGATE(FOnLobbyCharactersChanged);
DECLARE_MULTICAST_DELEGATE(FOnLobbyCharacterListFailed);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCreateCharacterFailed, const FString& /*Cause*/);
DECLARE_MULTICAST_DELEGATE(FOnDeleteCharacterFailed);
DECLARE_MULTICAST_DELEGATE(FOnEnterGameFailed);
DECLARE_MULTICAST_DELEGATE(FOnEnterGameSucceeded);

/**
 * 게임 서버의 로비.
 *
 * 로비 접속, 캐릭터 목록과 슬롯 수의 사본, 캐릭터 생성·삭제·선택 요청과 그 응답을 맡는다.
 * 화면에는 FP1LobbyCharacter와 델리게이트로만 알린다. 모든 멤버 함수는 게임 스레드 전용이다.
 */
UCLASS()
class P1_API UP1LobbySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

    //~ Listeners
public:
    /**
     * 이 서브시스템의 델리게이트에서 Listener가 붙인 것을 모두 뗀다.
     * 서브시스템은 레벨보다 오래 살기 때문에, 레벨과 함께 사라지는 객체는 사라지기 전에 불러야 한다.
     */
    void RemoveListener(const UObject* Listener);

    //~ Lobby Connection
public:
    /** 연결된 게임 서버에 인증 티켓을 내고 로비에 들어간다. 결과는 목록 응답이 알린다. */
    void ConnectLobby(const FString& AuthTicket);

    /** 캐릭터 목록 사본과 생성 요청 대기를 비운다. 게임 서버와의 연결이 끊기면 부른다. */
    void Reset();

    /** 로비 접속에 성공해 첫 캐릭터 목록을 받으면 OnCharactersChanged보다 먼저 알린다. */
    FOnLobbyEntered OnLobbyEntered;

    //~ Characters
public:
    const TArray<FP1LobbyCharacter>& GetCharacters() const { return Characters; }

    /** 계정이 가질 수 있는 캐릭터 수. 캐릭터 목록을 받기 전에는 0. */
    int32 GetCharacterSlotCount() const { return CharacterSlotCount; }

    /** 목록을 받거나 생성·삭제에 성공해 사본이 바뀌면 알린다. */
    FOnLobbyCharactersChanged OnCharactersChanged;

    /** 서버가 캐릭터 목록을 보내지 못했다. 사본은 그대로다. */
    FOnLobbyCharacterListFailed OnCharacterListFailed;

private:
    TArray<FP1LobbyCharacter> Characters;

    int32 CharacterSlotCount = 0;

    //~ Character Requests
public:
    /** 이름과 직업을 검사하지 않고 보낸다. 실패하면 OnCreateCharacterFailed로 서버의 사유를 알린다. */
    void CreateCharacter(const FString& CharacterName, EP1CharacterClass CharacterClass);

    /** 실패하면 OnDeleteCharacterFailed를 알린다. */
    void DeleteCharacter(int64 CharacterId);

    /** 고른 캐릭터로 게임 입장을 요청한다. 결과는 OnEnterGameSucceeded나 OnEnterGameFailed로 알린다. */
    void SelectCharacter(int64 CharacterId);

    /** 요청을 보내는 함수를 바꾼다. 테스트가 송신을 기록하려고 쓴다. 비우면 연결로 보낸다. */
    using FPacketSender = TFunction<void(SendBufferRef)>;
    void SetPacketSender(FPacketSender InSender) { PacketSender = MoveTemp(InSender); }

    FOnCreateCharacterFailed OnCreateCharacterFailed;
    FOnDeleteCharacterFailed OnDeleteCharacterFailed;
    FOnEnterGameFailed OnEnterGameFailed;

    /** 게임 입장에 성공했다. 내 플레이어 데이터는 이미 채워져 있다. */
    FOnEnterGameSucceeded OnEnterGameSucceeded;

private:
    /** 응답에는 캐릭터 번호만 오므로 이름과 직업을 요청할 때 적어 둔다. */
    struct FPendingCreate
    {
        FString CharacterName;
        EP1CharacterClass CharacterClass = EP1CharacterClass::None;
    };

    /** 응답을 기다리는 생성 요청. 응답을 받으면 비운다. */
    TOptional<FPendingCreate> PendingCreate;

    FPacketSender PacketSender;

    //~ Lobby Packet Handlers
public:
    void HandleCharacterList(const Protocol::S_LOGIN& LoginPkt);
    void HandleCreateCharacter(const Protocol::S_CREATE_CHARACTER& CreateCharacterPkt);
    void HandleDeleteCharacter(const Protocol::S_DELETE_CHARACTER& DeleteCharacterPkt);

    /** 성공했으면 내 플레이어 데이터를 먼저 채운 뒤 부른다. */
    void HandleEnterGame(bool bSuccess);
};
