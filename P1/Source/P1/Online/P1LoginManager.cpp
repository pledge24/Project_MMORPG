#include "Online/P1LoginManager.h"
#include "Network/P1NetworkSettings.h"
#include "UI/Frontend/P1LoginWidget.h"
#include "Http.h"
#include "HttpModule.h"
#include "Network/P1ConnectionSubsystem.h"
#include "Utils/LogCategory.h"

void UP1LoginManager::SetLoginWidget(UP1LoginWidget* Widget)
{
	LoginWidget = Widget;
}

FString UP1LoginManager::MakeAuthUrl(const TCHAR* Path)
{
    const UP1NetworkSettings* NetworkSettings = GetDefault<UP1NetworkSettings>();
    return FString::Printf(TEXT("http://%s:%d%s"), *NetworkSettings->AuthServerIp, NetworkSettings->AuthServerPort, Path);
}

void UP1LoginManager::RequestLogin(const FString& Username, const FString& Password)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> RequestRef = FHttpModule::Get().CreateRequest();

	// JSON 데이터 생성
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("username"), Username);
	JsonObject->SetStringField(TEXT("password"), Password);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	// 요청 설정
	FString URL = MakeAuthUrl(TEXT("/Login"));

    RequestRef->OnProcessRequestComplete().BindUObject(this, &UP1LoginManager::OnLoginResponse);
    RequestRef->SetURL(URL);
    RequestRef->SetVerb("POST");
    RequestRef->SetHeader("Content-Type", "application/json");
    RequestRef->SetContentAsString(OutputString);
    RequestRef->SetTimeout(10.0f); // 10초 타임아웃

    RequestRef->ProcessRequest();
}

void UP1LoginManager::RequestRegister(const FString& Username, const FString& Password)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> RequestRef = FHttpModule::Get().CreateRequest();

	// JSON 데이터 생성
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("username"), Username);
	JsonObject->SetStringField(TEXT("password"), Password);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	// 요청 설정
	FString URL = MakeAuthUrl(TEXT("/Account/Register"));

    RequestRef->OnProcessRequestComplete().BindUObject(this, &UP1LoginManager::OnRegisterResponse);
    RequestRef->SetURL(URL);
    RequestRef->SetVerb("POST");
    RequestRef->SetHeader("Content-Type", "application/json");
    RequestRef->SetContentAsString(OutputString);
    RequestRef->SetTimeout(10.0f); // 10초 타임아웃

    RequestRef->ProcessRequest();
}

void UP1LoginManager::OnLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
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
				token = JsonObject->GetStringField(TEXT("accessToken"));

				Message = TEXT("로그인 성공! 캐릭터 불러오는 중...");
				loginSuccess = true;
			}
		}
		else
		{
			if (FJsonSerializer::Deserialize(Reader, JsonObject))
			{
				Message = JsonObject->GetStringField(TEXT("errorMessage"));
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
        // 토큰 값은 로그 파일에 남기지 않는다.
        UE_LOG(LogP1Network, Display, TEXT("인증 서버에서 액세스 토큰을 받았다"));

		// 토큰은 저장하지 않고 게임 서버 로그인에 바로 넘긴다. 게임 서버가 한 번 쓰고 지운다.
		UGameInstance* GameInstance = GetWorld()->GetGameInstance();
		if (UP1ConnectionSubsystem* Connection = GameInstance ? GameInstance->GetSubsystem<UP1ConnectionSubsystem>() : nullptr)
		{
            Connection->Connect(token);
		}
        else
        {
            UE_LOG(LogP1Network, Error, TEXT("게임 서버 연결 서브시스템이 없습니다"));
        }
	}
}

void UP1LoginManager::OnRegisterResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
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
				Message = JsonObject->GetStringField(TEXT("message"));
			}
		}
		else
		{
			RegisterSuccess = false;
			if (FJsonSerializer::Deserialize(Reader, JsonObject))
			{
				Message = JsonObject->GetStringField(TEXT("errorMessage"));
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