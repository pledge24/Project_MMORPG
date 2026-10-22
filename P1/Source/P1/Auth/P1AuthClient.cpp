#include "Auth/P1AuthClient.h"
#include "Network/P1NetworkSettings.h"
#include "Http.h"
#include "HttpModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Utils/LogCategory.h"

void UP1AuthClient::RemoveListener(const UObject* Listener)
{
    OnAuthResult.RemoveAll(Listener);
    OnLoginSucceeded.RemoveAll(Listener);
}

void UP1AuthClient::RequestLogin(const FString& LoginId, const FString& Password)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> RequestRef = FHttpModule::Get().CreateRequest();

	// 요청 본문 JSON 작성.
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("loginId"), LoginId);
	JsonObject->SetStringField(TEXT("password"), Password);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	// 요청 설정.
	FString URL = MakeAuthUrl(TEXT("/api/auth/login"));

    RequestRef->OnProcessRequestComplete().BindUObject(this, &UP1AuthClient::OnLoginResponse);
    RequestRef->SetURL(URL);
    RequestRef->SetVerb("POST");
    RequestRef->SetHeader("Content-Type", "application/json");
    RequestRef->SetContentAsString(OutputString);
    RequestRef->SetTimeout(10.0f);

    RequestRef->ProcessRequest();
}

void UP1AuthClient::RequestRegister(const FString& LoginId, const FString& Password)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> RequestRef = FHttpModule::Get().CreateRequest();

	// 요청 본문 JSON 작성.
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("loginId"), LoginId);
	JsonObject->SetStringField(TEXT("password"), Password);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	// 요청 설정.
	FString URL = MakeAuthUrl(TEXT("/api/auth/register"));

    RequestRef->OnProcessRequestComplete().BindUObject(this, &UP1AuthClient::OnRegisterResponse);
    RequestRef->SetURL(URL);
    RequestRef->SetVerb("POST");
    RequestRef->SetHeader("Content-Type", "application/json");
    RequestRef->SetContentAsString(OutputString);
    RequestRef->SetTimeout(10.0f);

    RequestRef->ProcessRequest();
}

void UP1AuthClient::OnLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	FString Message = TEXT("로그인 실패");

	FString Ticket;
	bool bLoginSuccess = false;

	if (bWasSuccessful && Response.IsValid())
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		// 응답은 성공이면 { ok: true, data: { ticket } }, 실패면 { ok: false, error: { code, message } }다. 성공 여부는 상태 코드가 아니라 ok로 가른다.
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			bool bOk = false;
			JsonObject->TryGetBoolField(TEXT("ok"), bOk);

			const TSharedPtr<FJsonObject>* DataObject = nullptr;
			const TSharedPtr<FJsonObject>* ErrorObject = nullptr;
			if (bOk && JsonObject->TryGetObjectField(TEXT("data"), DataObject) && (*DataObject)->TryGetStringField(TEXT("ticket"), Ticket))
			{
				Message = TEXT("로그인 성공! 캐릭터 불러오는 중...");
				bLoginSuccess = true;
			}
			else if (JsonObject->TryGetObjectField(TEXT("error"), ErrorObject))
			{
				// 서버의 문구는 화면에 그대로 보여 주는 한국어 문장이다.
				(*ErrorObject)->TryGetStringField(TEXT("message"), Message);
			}
		}
	}
	else
	{
		Message = TEXT("인증 서버에 접속 실패");
	}

	OnAuthResult.Broadcast(bLoginSuccess, Message);

	if (bLoginSuccess)
	{
        // 티켓 값은 로그 파일에 남기지 않는다.
        UE_LOG(LogP1Network, Display, TEXT("인증 서버에서 인증 티켓을 받았다"));

		OnLoginSucceeded.Broadcast(Ticket);
	}
}

void UP1AuthClient::OnRegisterResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	FString Message = TEXT("회원가입 실패");

	bool RegisterSuccess = false;
	if (bWasSuccessful && Response.IsValid())
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		// 응답은 성공이면 { ok: true, data }, 실패면 { ok: false, error: { code, message } }다. 성공 여부는 상태 코드가 아니라 ok로 가른다.
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			bool bOk = false;
			JsonObject->TryGetBoolField(TEXT("ok"), bOk);

			const TSharedPtr<FJsonObject>* DataObject = nullptr;
			const TSharedPtr<FJsonObject>* ErrorObject = nullptr;
			if (bOk && JsonObject->TryGetObjectField(TEXT("data"), DataObject))
			{
				RegisterSuccess = true;

				// 서버가 소문자로 바꿔 저장한 로그인 ID를 보여 준다. 입력과 다를 수 있다.
				FString RegisteredLoginId;
				(*DataObject)->TryGetStringField(TEXT("loginId"), RegisteredLoginId);
				Message = FString::Printf(TEXT("회원가입 완료. 로그인 ID: %s"), *RegisteredLoginId);
			}
			else if (JsonObject->TryGetObjectField(TEXT("error"), ErrorObject))
			{
				// 서버의 문구는 화면에 그대로 보여 주는 한국어 문장이다.
				(*ErrorObject)->TryGetStringField(TEXT("message"), Message);
			}
		}
	}
	else
	{
		Message = TEXT("인증 서버에 접속 실패");
	}

	OnAuthResult.Broadcast(RegisterSuccess, Message);
}

FString UP1AuthClient::MakeAuthUrl(const TCHAR* Path)
{
    const UP1NetworkSettings* NetworkSettings = GetDefault<UP1NetworkSettings>();
    return FString::Printf(TEXT("http://%s:%d%s"), *NetworkSettings->AuthServerIp, NetworkSettings->AuthServerPort, Path);
}
