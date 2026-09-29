#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Http.h"
#include "P1LoginManager.generated.h"

UCLASS()
class P1_API UP1LoginManager : public UObject
{
    GENERATED_BODY()

    //~ Auth Request
public:
    void RequestLogin(const FString& Username, const FString& Password);
    void RequestRegister(const FString& Username, const FString& Password);

    void OnLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnRegisterResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

private:
    /** 인증 서버 주소에 경로를 붙인 URL. 주소는 UP1NetworkSettings에서 읽는다. */
    static FString MakeAuthUrl(const TCHAR* Path);

    //~ Login Widget
public:
    void SetLoginWidget(class UP1LoginWidget* Widget);
    UP1LoginWidget* GetLoginWidget() { return LoginWidget; }

private:
    UPROPERTY()
    TObjectPtr<UP1LoginWidget> LoginWidget;
};
