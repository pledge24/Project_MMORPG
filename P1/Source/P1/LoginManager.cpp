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
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> RequestRef = FHttpModule::Get().CreateRequest();

	// JSON 데이터 생성
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField("username", Username);
	JsonObject->SetStringField("password", Password);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	// 요청 설정
	FString URL = FString::Printf(TEXT("http://%s:%d/Login"), *ServerIP, ServerPort);

    RequestRef->OnProcessRequestComplete().BindUObject(this, &ULoginManager::OnLoginResponse);
    RequestRef->SetURL(URL);
    RequestRef->SetVerb("POST");
    RequestRef->SetHeader("Content-Type", "application/json");
    RequestRef->SetContentAsString(OutputString);
    RequestRef->SetTimeout(10.0f); // 10초 타임아웃

    RequestRef->ProcessRequest();
}

void ULoginManager::RequestRegister(const FString& Username, const FString& Password)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> RequestRef = FHttpModule::Get().CreateRequest();

	// JSON 데이터 생성
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField("username", Username);
	JsonObject->SetStringField("password", Password);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	// 요청 설정
	FString URL = FString::Printf(TEXT("http://%s:%d/Account/Register"), *ServerIP, ServerPort);

    RequestRef->OnProcessRequestComplete().BindUObject(this, &ULoginManager::OnRegisterResponse);
    RequestRef->SetURL(URL);
    RequestRef->SetVerb("POST");
    RequestRef->SetHeader("Content-Type", "application/json");
    RequestRef->SetContentAsString(OutputString);
    RequestRef->SetTimeout(10.0f); // 10초 타임아웃

    RequestRef->ProcessRequest();
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
				// 토큰을 GameInstance에 저장
				token = JsonObject->GetStringField("accessToken");
				if (auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance()))
				{
					GameInstance->SetToken(token);
				}

				Message = TEXT("로그인 성공! 캐릭터 불러오는 중...");
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
		LoginWidget->SetResultText(loginSuccess, Message);
	}

	if (loginSuccess)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("AccessToken: %s"), *token));

		auto* GameInstance = Cast<UP1GameInstance>(GetWorld()->GetGameInstance());

		// 게임서버에 입장쓰.
		if (GameInstance)
		{
			GameInstance->ConnectToGameServer();
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("게임인스턴스가 없습니다")));
		}
		
	}
}

void ULoginManager::OnRegisterResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	FString Message = TEXT("로그인 실패");

	bool RegisterSuccess = false;
	if (bWasSuccessful && Response.IsValid())
	{
		int32 ResponseCode = Response->GetResponseCode();
		FString ResponseBody = Response->GetContentAsString();

		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

		if (ResponseCode == 200 || ResponseCode == 201)
		{
			RegisterSuccess = true;
			if (FJsonSerializer::Deserialize(Reader, JsonObject))
			{
				Message = JsonObject->GetStringField("message");
			}
		}
		else
		{
			RegisterSuccess = false;
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
		LoginWidget->SetResultText(RegisterSuccess, Message);
	}
}