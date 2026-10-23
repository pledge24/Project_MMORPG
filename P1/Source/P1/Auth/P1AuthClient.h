#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Auth/P1World.h"
#include "P1AuthClient.generated.h"

class FJsonObject;
class FJsonValue;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAuthResult, bool /*bSuccess*/, const FString& /*Message*/);
DECLARE_MULTICAST_DELEGATE(FOnAuthLoginSucceeded);
DECLARE_MULTICAST_DELEGATE(FOnAuthWorldsChanged);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnAuthWorldSelected, const FString& /*Host*/, int32 /*Port*/, const FString& /*AuthTicket*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnAuthFailed, const FString& /*Message*/);

/**
 * 인증 서버의 클라이언트.
 *
 * 인증 서버와의 HTTP(회원가입, 로그인, 월드 목록, 월드 선택)를 모두 맡고, 로그인 세션과 월드 목록의 사본을 갖는다.
 * 결과는 델리게이트로만 알린다. 게임 서버를 모른다. 모든 멤버 함수는 게임 스레드 전용이다.
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
    /** 결과는 OnAuthResult로 알리고, 성공하면 월드 목록을 바꾼 뒤 OnLoginSucceeded로 알린다. */
    void RequestLogin(const FString& LoginId, const FString& Password);

    /** 결과는 OnAuthResult로 알린다. */
    void RequestRegister(const FString& LoginId, const FString& Password);

    /** 로그인 세션과 월드 목록을 버린다. 인증 서버의 로그인 세션은 수명이 끝날 때까지 남는다. */
    void Logout();

    bool HasLoginSession() const { return LoginSession.IsEmpty() == false; }

    /** 로그인과 회원가입 요청의 성공 여부와 화면에 보여 줄 결과 문구. */
    FOnAuthResult OnAuthResult;

    /** 로그인에 성공해 로그인 세션과 월드 목록을 받았다. */
    FOnAuthLoginSucceeded OnLoginSucceeded;

    /**
     * 로그인 세션이 만료돼 버렸다. 다시 로그인해야 한다. 화면에 보여 줄 문구를 싣는다.
     * 로그인 세션을 쓰는 요청(월드 목록, 월드 선택)이 거절됐을 때 온다.
     */
    FOnAuthFailed OnLoginSessionExpired;

private:
    void OnLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnRegisterResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    /** 로그인 응답이 준 값. 월드 목록과 월드 선택 요청의 Authorization 헤더에 싣는다. 로그인 전에는 비어 있다. */
    FString LoginSession;

    //~ Worlds
public:
    /** 마지막으로 받은 월드 목록. 월드 번호 순이다. */
    const TArray<FP1World>& GetWorlds() const { return Worlds; }

    /** 로그인 세션으로 월드 목록을 다시 받는다. 받으면 OnWorldsChanged로 알린다. 로그인 세션이 없으면 아무것도 하지 않는다. */
    void RequestWorlds();

    /**
     * WorldId 월드를 고른다. 성공하면 그 월드의 주소와 인증 티켓을 OnWorldSelected로, 실패하면 사유를
     * OnWorldSelectFailed로 알린다. 로그인 세션이 만료됐으면 OnLoginSessionExpired로 알린다.
     */
    void RequestSelectWorld(int32 WorldId);

    /**
     * 인증 서버가 준 월드 목록 JSON 배열을 읽는다. 형식이 틀린 항목은 버리고 경고를 남긴다.
     * 응답을 받는 쪽이 부르는 순수 함수다. 테스트가 HTTP 없이 부른다.
     */
    static TArray<FP1World> ParseWorlds(const TArray<TSharedPtr<FJsonValue>>& WorldValues);

    /** 월드 목록이 바뀌었다. 로그인, 월드 목록 요청, 로그아웃 뒤에 온다. */
    FOnAuthWorldsChanged OnWorldsChanged;

    /** 월드를 골라 그 월드에서만 쓸 수 있는 인증 티켓을 받았다. 인증 티켓은 저장하지 않으므로 받은 쪽이 바로 쓴다. */
    FOnAuthWorldSelected OnWorldSelected;

    /** 월드 선택이 거절됐다. 로그인 세션은 남아 있으므로 다른 월드를 고를 수 있다. 화면에 보여 줄 문구를 싣는다. */
    FOnAuthFailed OnWorldSelectFailed;

private:
    void OnWorldsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnSelectWorldResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    /**
     * 실패 봉투가 로그인 세션 만료면 로그인 세션과 월드 목록을 버리고 OnLoginSessionExpired로 알린 뒤 true.
     * 그 밖의 실패면 아무것도 하지 않고 false.
     */
    bool HandleLoginSessionExpired(const FString& ErrorCode, const FString& Message);

    void SetWorlds(TArray<FP1World>&& NewWorlds);

    TArray<FP1World> Worlds;

    //~ 공통
private:
    /** 인증 서버 주소에 경로를 붙인 요청. 주소는 UP1NetworkSettings에서 읽는다. 로그인 세션이 있으면 헤더에 싣는다. */
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> CreateRequest(const FString& Verb, const FString& Path) const;

    /**
     * 응답 봉투를 읽는다. 성공이면 OutData에 data를 담고 true, 실패면 OutErrorCode와 OutMessage에 error의 값을 담고 false.
     * 응답을 받지 못했거나 봉투가 아니면 OutMessage를 바꾸지 않고 false.
     */
    static bool ReadEnvelope(FHttpResponsePtr Response, bool bWasSuccessful, TSharedPtr<FJsonObject>& OutData, FString& OutErrorCode, FString& OutMessage);
};
