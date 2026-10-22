#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "P1AuthClient.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAuthResult, bool /*bSuccess*/, const FString& /*Message*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAuthLoginSucceeded, const FString& /*AuthTicket*/);

/**
 * 인증 서버의 클라이언트.
 *
 * 인증 서버와의 HTTP(회원가입, 로그인)를 모두 맡고, 결과를 델리게이트로만 알린다.
 * 게임 서버를 모른다. 모든 멤버 함수는 게임 스레드 전용이다.
 */
UCLASS()
class P1_API UP1AuthClient : public UGameInstanceSubsystem
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
    /** 결과는 OnAuthResult로 알리고, 성공하면 이어서 OnLoginSucceeded로 인증 티켓을 넘긴다. */
    void RequestLogin(const FString& LoginId, const FString& Password);

    /** 결과는 OnAuthResult로 알린다. */
    void RequestRegister(const FString& LoginId, const FString& Password);

    /** 로그인과 회원가입 요청의 성공 여부와 화면에 보여 줄 결과 문구. */
    FOnAuthResult OnAuthResult;

    /** 로그인에 성공했다. 인증 티켓은 저장하지 않으므로 받은 쪽이 바로 쓴다. */
    FOnAuthLoginSucceeded OnLoginSucceeded;

private:
    void OnLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnRegisterResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    /** 인증 서버 주소에 경로를 붙인 URL. 주소는 UP1NetworkSettings에서 읽는다. */
    static FString MakeAuthUrl(const TCHAR* Path);
};
