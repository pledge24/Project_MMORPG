#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Http.h"
#include "Protocol.pb.h"
#include "P1LoginManager.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAuthResult, bool /*bSuccess*/, const FString& /*Message*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCharacterListReceived, const Protocol::S_LOGIN&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCreateCharacterResult, const Protocol::S_CREATE_CHARACTER&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDeleteCharacterResult, const Protocol::S_DELETE_CHARACTER&);
DECLARE_MULTICAST_DELEGATE(FOnEnterGameFailed);
DECLARE_MULTICAST_DELEGATE(FOnEnterGameSucceeded);

/** 인증 서버 로그인과 게임 서버의 캐릭터 목록, 생성, 삭제, 입장 응답을 맡는다. 화면에는 델리게이트로만 알린다. */
UCLASS()
class P1_API UP1LoginManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

    //~ Listeners
public:
    /**
     * 이 서브시스템의 델리게이트에서 Listener가 붙인 것을 모두 뗀다.
     * 서브시스템은 레벨보다 오래 살기 때문에, 레벨과 함께 사라지는 객체는 사라지기 전에 불러야 한다.
     */
    void RemoveListener(const UObject* Listener);

    //~ Auth Request
public:
    void RequestLogin(const FString& Username, const FString& Password);
    void RequestRegister(const FString& Username, const FString& Password);

    /** 로그인과 회원가입 요청의 결과 문구다. 게임 스레드에서 알린다. */
    FOnAuthResult OnAuthResult;

private:
    void OnLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnRegisterResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    /** 인증 서버 주소에 경로를 붙인 URL. 주소는 UP1NetworkSettings에서 읽는다. */
    static FString MakeAuthUrl(const TCHAR* Path);

    //~ Login Packet Handlers
public:
    /** 패킷 핸들러가 수신 펌프에서 부른다. 게임 스레드 전용. 아래 핸들러도 같다. */
    void HandleLogin(const Protocol::S_LOGIN& LoginPkt);
    void HandleCreateCharacter(const Protocol::S_CREATE_CHARACTER& CreateCharacterPkt);
    void HandleDeleteCharacter(const Protocol::S_DELETE_CHARACTER& DeleteCharacterPkt);

    /** 성공하면 내 플레이어 데이터를 채운 뒤 OnEnterGameSucceeded를, 거절되면 OnEnterGameFailed를 알린다. */
    void HandleEnterGame(const Protocol::S_ENTER_GAME& EnterGamePkt);

    FOnCharacterListReceived OnCharacterListReceived;

    /** 실패 응답에도 알린다. 성공 여부는 패킷에 있다. */
    FOnCreateCharacterResult OnCreateCharacterResult;

    /** 실패 응답에도 알린다. 성공 여부는 패킷에 있다. */
    FOnDeleteCharacterResult OnDeleteCharacterResult;

    FOnEnterGameFailed OnEnterGameFailed;

    /** 게임 인스턴스가 구독해 인게임 맵을 연다. */
    FOnEnterGameSucceeded OnEnterGameSucceeded;
};
