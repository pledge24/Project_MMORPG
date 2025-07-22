// Fill out your copyright notice in the Description page of Project Settings.

#include "LoginWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "HttpModule.h"
#include "Json.h"
#include "P1.h"

void ULoginWidget::RequestRegister(const FString& Username, const FString& Password)
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Request:Register - ID: %s, PW: %s"), *Username, *Password));

}

void ULoginWidget::RequestLogin(const FString& Username, const FString& Password)
{
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Request:Login - ID: %s, PW: %s"), *Username, *Password));

}

void ULoginWidget::OnLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    bool bLoginSuccess = false;
    FString Message = TEXT("failed to login");

    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseBody = Response->GetContentAsString();

        if (ResponseCode == 200)
        {
            // JSON 응답 파싱
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

            if (FJsonSerializer::Deserialize(Reader, JsonObject))
            {
                // 토큰이나 사용자 정보 저장 (필요시)
                FString Token = JsonObject->GetStringField("accessToken");
                UE_LOG(LogTemp, Warning, TEXT("Login successful! Token: %s"), *Token);
            }
        }
        else
        {
            TSharedPtr<FJsonObject> JsonObject;
            Message = JsonObject->GetStringField("errorMessage");
        }
    }
    else
    {
        Message = TEXT("failed to connect AuthServer");
    }

    // 블루프린트로 결과 전달
    OnLoginComplete.Broadcast(bLoginSuccess, Message);
}

void ULoginWidget::OnRegisterResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    bool bRegisterSuccess = false;
    FString Message = TEXT("failed to register");

    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseBody = Response->GetContentAsString();

        if (ResponseCode == 200 || ResponseCode == 201)
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

            if (FJsonSerializer::Deserialize(Reader, JsonObject))
            {
                Message = JsonObject->GetStringField("message");
            }
        }
        else
        {
            TSharedPtr<FJsonObject> JsonObject;
            Message = JsonObject->GetStringField("errorMessage");
        }
    }
    else
    {
        Message = TEXT("failed to connect AuthServer");
    }

    // 블루프린트로 결과 전달
    OnRegisterComplete.Broadcast(bRegisterSuccess, Message);
}