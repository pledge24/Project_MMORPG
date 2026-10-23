#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Lobby/P1LobbyCharacter.h"
#include "Utils/Types.h"
#include "P1LobbySubsystem.generated.h"

// 화면이 이 헤더를 부르므로 protobuf 헤더를 끌고 오지 않는다. 생성된 정의와 바탕 타입이 같아야 한다.
namespace Protocol
{
    enum LobbyResult : int;
    class S_CONNECT_LOBBY;
    class S_CHARACTER_LIST;
    class S_CREATE_CHARACTER;
    class S_DELETE_CHARACTER;
}

DECLARE_MULTICAST_DELEGATE(FOnLobbyEntered);
DECLARE_MULTICAST_DELEGATE(FOnLobbyCharactersChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyConnectFailed, const FText& /*Message*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCreateCharacterFailed, const FText& /*Message*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDeleteCharacterFailed, const FText& /*Message*/);
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
    /** 연결된 게임 서버에 인증 티켓을 내고 로비에 들어간다. 성공은 OnLobbyEntered로, 실패는 OnLobbyConnectFailed로 알린다. */
    void ConnectLobby(const FString& AuthTicket);

    /** 캐릭터 목록 사본을 비운다. 게임 서버와의 연결이 끊기면 부른다. */
    void Reset();

    /** 로비 접속에 성공해 캐릭터 목록을 받으면 OnCharactersChanged보다 먼저 알린다. */
    FOnLobbyEntered OnLobbyEntered;

    /** 서버가 로비 접속을 거절했다. 서버는 이 응답 뒤에 연결을 끊는다. */
    FOnLobbyConnectFailed OnLobbyConnectFailed;

    //~ Characters
public:
    const TArray<FP1LobbyCharacter>& GetCharacters() const { return Characters; }

    /** 계정이 가질 수 있는 캐릭터 수. 캐릭터 목록을 받기 전에는 0. */
    int32 GetCharacterSlotCount() const { return CharacterSlotCount; }

    /** 캐릭터 생성 화면이 보여 줄 직업. 서버가 캐릭터 목록과 함께 알려 주며, 목록을 받기 전에는 비어 있다. */
    const TArray<EP1CharacterClass>& GetCreatableClasses() const { return CreatableClasses; }

    /** 목록을 받거나 생성·삭제에 성공해 사본이 바뀌면 알린다. */
    FOnLobbyCharactersChanged OnCharactersChanged;

private:
    TArray<FP1LobbyCharacter> Characters;

    int32 CharacterSlotCount = 0;

    TArray<EP1CharacterClass> CreatableClasses;

    //~ Character Requests
public:
    /** 이름과 직업을 검사하지 않고 보낸다. 실패하면 OnCreateCharacterFailed로 결과의 문구를 알린다. */
    void CreateCharacter(const FString& CharacterName, EP1CharacterClass CharacterClass);

    /** 실패하면 OnDeleteCharacterFailed로 결과의 문구를 알린다. */
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
    FPacketSender PacketSender;

    //~ Lobby Result
public:
    /** 서버가 보낸 로비 결과 값을 화면 문구로 바꾼다. 모르는 값이면 값을 담은 일반 오류 문구. */
    static FText GetResultText(Protocol::LobbyResult Result);

    //~ Lobby Packet Handlers
public:
    void HandleConnectLobby(const Protocol::S_CONNECT_LOBBY& ConnectLobbyPkt);
    void HandleCharacterList(const Protocol::S_CHARACTER_LIST& CharacterListPkt);
    void HandleCreateCharacter(const Protocol::S_CREATE_CHARACTER& CreateCharacterPkt);
    void HandleDeleteCharacter(const Protocol::S_DELETE_CHARACTER& DeleteCharacterPkt);

    /** 성공했으면 내 플레이어 데이터를 먼저 채운 뒤 부른다. */
    void HandleEnterGame(bool bSuccess);
};
