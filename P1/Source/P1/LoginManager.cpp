// Fill out your copyright notice in the Description page of Project Settings.

#include "LoginManager.h"
#include "HttpModule.h"
#include "Json.h"

ALoginManager::ALoginManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ALoginManager::RequestRegister(const FString& Username, const FString& Password, const FString& Email)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    // JSON 데이터 생성
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField("username", Username);
    JsonObject->SetStringField("password", Password);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    // 요청 설정
    FString URL = FString::Printf(TEXT("http://%s:%d/Account/Register"), *ServerIP, ServerPort);

    Request->OnProcessRequestComplete().BindUObject(this, &ALoginManager::OnRegisterResponse);
    Request->SetURL(URL);
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", TEXT("application/json"));
    Request->SetContentAsString(OutputString);
    Request->SetTimeout(10.0f);

    Request->ProcessRequest();
}

void ALoginManager::RequestLogin(const FString& Username, const FString& Password)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

    // JSON 데이터 생성
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetStringField("username", Username);
    JsonObject->SetStringField("password", Password);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    // 요청 설정
    FString URL = FString::Printf(TEXT("http://%s:%d/Login"), *ServerIP, ServerPort);

    Request->OnProcessRequestComplete().BindUObject(this, &ALoginManager::OnLoginResponse);
    Request->SetURL(URL);
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", TEXT("application/json"));
    Request->SetContentAsString(OutputString);
    Request->SetTimeout(10.0f); // 10초 타임아웃

    Request->ProcessRequest();
}

void ALoginManager::OnLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
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

void ALoginManager::OnRegisterResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
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