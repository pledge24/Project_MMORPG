// Fill out your copyright notice in the Description page of Project Settings.

#include "LoginManager.h"
#include "LoginWidget.h"
#include "Http.h"
#include "HttpModule.h"
#include "P1.h"

void ULoginManager::SetLoginWidget(ULoginWidget* Widget)
{
	LoginWidget = Widget;
}

void ULoginManager::RequestLogin(const FString& Username, const FString& Password)
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

	Request->OnProcessRequestComplete().BindUObject(this, &ULoginManager::OnLoginResponse);
	Request->SetURL(URL);
	Request->SetVerb("POST");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(OutputString);
	Request->SetTimeout(10.0f); // 10초 타임아웃

	Request->ProcessRequest();
}

void ULoginManager::RequestRegister(const FString& Username, const FString& Password)
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

	Request->OnProcessRequestComplete().BindUObject(this, &ULoginManager::OnRegisterResponse);
	Request->SetURL(URL);
	Request->SetVerb("POST");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(OutputString);
	Request->SetTimeout(10.0f); // 10초 타임아웃

	Request->ProcessRequest();
}

void ULoginManager::OnLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	FString Message = TEXT("로그인 실패");

	FString token;
	bool loginSuccess = false;

	if (bWasSuccessful && Response.IsValid())
	{
		int32 ResponseCode = Response->GetResponseCode();
		FString ResponseBody = Response->GetContentAsString();

		// JSON 응답 파싱
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

		if (ResponseCode == 200)
		{
			if (FJsonSerializer::Deserialize(Reader, JsonObject))
			{
				// 토큰 저장 (필요시)
				token = JsonObject->GetStringField("accessToken");
				if (auto* GameInstance = Cast<UP1GameInstance>(GWorld->GetGameInstance()))
				{
					GameInstance->SetToken(token);
				}

				Message = TEXT("로그인 성공! 캐릭터 선택 창으로 이동중...");
				loginSuccess = true;
			}
		}
		else
		{
			if (FJsonSerializer::Deserialize(Reader, JsonObject))
			{
				Message = JsonObject->GetStringField("errorMessage");
			}
		}
	}
	else
	{
		Message = TEXT("인증 서버에 접속 실패");
	}

	if (LoginWidget)
	{
		LoginWidget->SetResultMessage(Message);
	}

	if (loginSuccess)
	{
		// 게임서버에 입장쓰.
		
		// 캐릭터 선택창으로 이동.
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("AccessToken: %s"), *token));
		LoginWidget->SwitchToIndex(1);
	}
}

void ULoginManager::OnRegisterResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	FString Message = TEXT("로그인 실패");

	if (bWasSuccessful && Response.IsValid())
	{
		int32 ResponseCode = Response->GetResponseCode();
		FString ResponseBody = Response->GetContentAsString();

		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

		if (ResponseCode == 200 || ResponseCode == 201)
		{

			if (FJsonSerializer::Deserialize(Reader, JsonObject))
			{
				Message = JsonObject->GetStringField("message");
			}
		}
		else
		{
			if (FJsonSerializer::Deserialize(Reader, JsonObject))
			{
				Message = JsonObject->GetStringField("errorMessage");
			}
		}
	}
	else
	{
		Message = TEXT("인증 서버에 접속 실패");
	}

	if (LoginWidget)
	{
		LoginWidget->SetResultMessage(Message);
		LoginWidget->ClearMessage();
	}
}
