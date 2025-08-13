// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Http.h"
#include "LoginManager.generated.h"

/**
 * 
 */
UCLASS()
class P1_API ULoginManager : public UObject
{
    GENERATED_BODY()

public:
    void RequestLogin(const FString& Username, const FString& Password);
    void RequestRegister(const FString& Username, const FString& Password);

    void OnLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnRegisterResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    void SetLoginWidget(class ULoginWidget* Widget);
    ULoginWidget* GetLoginWidget() { return LoginWidget; }

private:
    ULoginWidget* LoginWidget;

private:
    // 서버 설정
    FString ServerIP = TEXT("127.0.0.1");
    int32 ServerPort = 5000;
};
