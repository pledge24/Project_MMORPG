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
    // 인증 서버 주소
    FString ServerIP = TEXT("127.0.0.1");
    int32 ServerPort = 5000;

    //~ Login Widget
public:
    void SetLoginWidget(class UP1LoginWidget* Widget);
    UP1LoginWidget* GetLoginWidget() { return LoginWidget; }

private:
    UP1LoginWidget* LoginWidget;
};
