// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Http.h"
#include "LoginManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLoginComplete, bool, bSuccess, FString, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRegisterComplete, bool, bSuccess, FString, Message);

UCLASS()
class P1_API ALoginManager : public AActor
{
    GENERATED_BODY()

public:
    ALoginManager();

    UFUNCTION(BlueprintCallable, Category = "Login")
    void RequestLogin(const FString& Username, const FString& Password);

    UFUNCTION(BlueprintCallable, Category = "Login")
    void RequestRegister(const FString& Username, const FString& Password, const FString& Email);

    UPROPERTY(BlueprintAssignable)
    FOnLoginComplete OnLoginComplete;

    UPROPERTY(BlueprintAssignable)
    FOnRegisterComplete OnRegisterComplete;

private:
    // 서버 설정
    FString ServerIP = TEXT("127.0.0.1");
    int32 ServerPort = 5000;

    void OnLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnRegisterResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
