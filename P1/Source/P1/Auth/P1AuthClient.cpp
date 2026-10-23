#include "Auth/P1AuthClient.h"
#include "Network/P1NetworkSettings.h"
#include "Http.h"
#include "HttpModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Utils/LogCategory.h"

namespace
{
    // 인증 서버가 로그인 세션이 없거나 만료됐을 때 돌려주는 실패 코드. 클라이언트는 문구가 아니라 코드로 가른다.
    const TCHAR* const LOGIN_SESSION_EXPIRED_CODE = TEXT("LOGIN_SESSION_EXPIRED");

    // 고른 월드의 항목이 인증 서버에서 사라졌을 때의 실패 코드.
    const TCHAR* const WORLD_UNAVAILABLE_CODE = TEXT("WORLD_UNAVAILABLE");

    const TCHAR* const AUTH_SERVER_UNREACHABLE = TEXT("인증 서버에 접속 실패");

    /** 인증 서버의 혼잡도 이름을 읽는다. 모르는 이름이면 false. */
    bool ParseCongestion(const FString& Name, EP1WorldCongestion& OutCongestion)
    {
        if (Name == TEXT("SMOOTH"))
            OutCongestion = EP1WorldCongestion::Smooth;
        else if (Name == TEXT("NORMAL"))
            OutCongestion = EP1WorldCongestion::Normal;
        else if (Name == TEXT("CROWDED"))
            OutCongestion = EP1WorldCongestion::Crowded;
        else
            return false;

        return true;
    }

    FString MakeCredentialsBody(const FString& LoginId, const FString& Password)
    {
        TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
        JsonObject->SetStringField(TEXT("loginId"), LoginId);
        JsonObject->SetStringField(TEXT("password"), Password);

        FString Body;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
        return Body;
    }
}

void UP1AuthClient::RemoveListener(const UObject* Listener)
{
    OnAuthResult.RemoveAll(Listener);
    OnLoginSucceeded.RemoveAll(Listener);
    OnLoginSessionExpired.RemoveAll(Listener);
    OnWorldsChanged.RemoveAll(Listener);
    OnWorldSelected.RemoveAll(Listener);
    OnWorldSelectFailed.RemoveAll(Listener);
}

void UP1AuthClient::RequestLogin(const FString& LoginId, const FString& Password)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = CreateRequest(TEXT("POST"), TEXT("/api/auth/login"));
    Request->SetContentAsString(MakeCredentialsBody(LoginId, Password));
    Request->OnProcessRequestComplete().BindUObject(this, &UP1AuthClient::OnLoginResponse);
    Request->ProcessRequest();
}

void UP1AuthClient::RequestRegister(const FString& LoginId, const FString& Password)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = CreateRequest(TEXT("POST"), TEXT("/api/auth/register"));
    Request->SetContentAsString(MakeCredentialsBody(LoginId, Password));
    Request->OnProcessRequestComplete().BindUObject(this, &UP1AuthClient::OnRegisterResponse);
    Request->ProcessRequest();
}

void UP1AuthClient::Logout()
{
    LoginSession.Reset();
    SetWorlds({});
}

void UP1AuthClient::OnLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FString Message = bWasSuccessful ? TEXT("로그인 실패") : AUTH_SERVER_UNREACHABLE;

    // 성공 봉투의 data는 { loginSession, worlds }다.
    TSharedPtr<FJsonObject> Data;
    FString ErrorCode;
    FString NewLoginSession;
    const TArray<TSharedPtr<FJsonValue>>* WorldValues = nullptr;
    const bool bLoginSuccess = ReadEnvelope(Response, bWasSuccessful, Data, ErrorCode, Message)
        && Data->TryGetStringField(TEXT("loginSession"), NewLoginSession)
        && NewLoginSession.IsEmpty() == false
        && Data->TryGetArrayField(TEXT("worlds"), WorldValues);

    if (bLoginSuccess == false)
    {
        OnAuthResult.Broadcast(false, Message);
        return;
    }

    // 로그인 세션 값은 로그 파일에 남기지 않는다.
    UE_LOG(LogP1Network, Display, TEXT("인증 서버에 로그인해 로그인 세션과 월드 %d개를 받았다"), WorldValues->Num());

    LoginSession = NewLoginSession;
    SetWorlds(ParseWorlds(*WorldValues));

    OnAuthResult.Broadcast(true, TEXT("로그인 성공"));
    OnLoginSucceeded.Broadcast();
}

void UP1AuthClient::OnRegisterResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FString Message = bWasSuccessful ? TEXT("회원가입 실패") : AUTH_SERVER_UNREACHABLE;

    TSharedPtr<FJsonObject> Data;
    FString ErrorCode;
    const bool bRegisterSuccess = ReadEnvelope(Response, bWasSuccessful, Data, ErrorCode, Message);
    if (bRegisterSuccess)
    {
        // 서버가 소문자로 바꿔 저장한 로그인 ID를 보여 준다. 입력과 다를 수 있다.
        FString RegisteredLoginId;
        Data->TryGetStringField(TEXT("loginId"), RegisteredLoginId);
        Message = FString::Printf(TEXT("회원가입 완료. 로그인 ID: %s"), *RegisteredLoginId);
    }

    OnAuthResult.Broadcast(bRegisterSuccess, Message);
}

void UP1AuthClient::RequestWorlds()
{
    if (HasLoginSession() == false)
        return;

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = CreateRequest(TEXT("GET"), TEXT("/api/worlds"));
    Request->OnProcessRequestComplete().BindUObject(this, &UP1AuthClient::OnWorldsResponse);
    Request->ProcessRequest();
}

void UP1AuthClient::RequestSelectWorld(int32 WorldId)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = CreateRequest(TEXT("POST"), FString::Printf(TEXT("/api/worlds/%d/select"), WorldId));
    Request->OnProcessRequestComplete().BindUObject(this, &UP1AuthClient::OnSelectWorldResponse);
    Request->ProcessRequest();
}

TArray<FP1World> UP1AuthClient::ParseWorlds(const TArray<TSharedPtr<FJsonValue>>& WorldValues)
{
    TArray<FP1World> Parsed;
    for (const TSharedPtr<FJsonValue>& Value : WorldValues)
    {
        const TSharedPtr<FJsonObject>* WorldObject = nullptr;
        FP1World World;
        FString CongestionName;
        const bool bValid = Value.IsValid()
            && Value->TryGetObject(WorldObject)
            && (*WorldObject)->TryGetNumberField(TEXT("worldId"), World.WorldId)
            && World.WorldId > 0
            && (*WorldObject)->TryGetStringField(TEXT("name"), World.Name)
            && (*WorldObject)->TryGetStringField(TEXT("congestion"), CongestionName)
            && ParseCongestion(CongestionName, World.Congestion);

        // 인증 서버가 새 혼잡도를 더했거나 형식이 바뀌었다. 그 항목만 빼고 나머지 월드는 보여 준다.
        if (bValid == false)
        {
            UE_LOG(LogP1Network, Warning, TEXT("월드 목록에서 읽지 못한 항목을 뺀다"));
            continue;
        }

        Parsed.Add(MoveTemp(World));
    }

    return Parsed;
}

void UP1AuthClient::OnWorldsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    TSharedPtr<FJsonObject> Data;
    FString ErrorCode;
    FString Message;
    const TArray<TSharedPtr<FJsonValue>>* WorldValues = nullptr;
    if (ReadEnvelope(Response, bWasSuccessful, Data, ErrorCode, Message) && Data->TryGetArrayField(TEXT("worlds"), WorldValues))
    {
        SetWorlds(ParseWorlds(*WorldValues));
        return;
    }

    // 목록 갱신은 화면이 주기적으로 부른다. 한 번 실패하면 지난 목록을 그대로 두고 다음 갱신을 기다린다.
    if (HandleLoginSessionExpired(ErrorCode, Message) == false)
        UE_LOG(LogP1Network, Warning, TEXT("월드 목록을 받지 못함: %s %s"), *ErrorCode, *Message);
}

void UP1AuthClient::OnSelectWorldResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    FString Message = bWasSuccessful ? TEXT("월드를 선택하지 못했습니다.") : AUTH_SERVER_UNREACHABLE;

    // 성공 봉투의 data는 { host, port, ticket }다.
    TSharedPtr<FJsonObject> Data;
    FString ErrorCode;
    FString Host;
    int32 Port = 0;
    FString Ticket;
    const bool bSelected = ReadEnvelope(Response, bWasSuccessful, Data, ErrorCode, Message)
        && Data->TryGetStringField(TEXT("host"), Host)
        && Data->TryGetNumberField(TEXT("port"), Port)
        && Data->TryGetStringField(TEXT("ticket"), Ticket);

    if (bSelected)
    {
        // 티켓 값은 로그 파일에 남기지 않는다.
        UE_LOG(LogP1Network, Display, TEXT("월드를 골라 %s:%d의 인증 티켓을 받았다"), *Host, Port);
        OnWorldSelected.Broadcast(Host, Port, Ticket);
        return;
    }

    if (HandleLoginSessionExpired(ErrorCode, Message))
        return;

    OnWorldSelectFailed.Broadcast(Message);

    // 사라진 월드를 골랐다. 목록을 다시 받아 그 월드를 지운다.
    if (ErrorCode == WORLD_UNAVAILABLE_CODE)
        RequestWorlds();
}

bool UP1AuthClient::HandleLoginSessionExpired(const FString& ErrorCode, const FString& Message)
{
    if (ErrorCode != LOGIN_SESSION_EXPIRED_CODE)
        return false;

    UE_LOG(LogP1Network, Display, TEXT("로그인 세션이 만료됐다"));
    Logout();
    OnLoginSessionExpired.Broadcast(Message);
    return true;
}

void UP1AuthClient::SetWorlds(TArray<FP1World>&& NewWorlds)
{
    Worlds = MoveTemp(NewWorlds);
    OnWorldsChanged.Broadcast();
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe> UP1AuthClient::CreateRequest(const FString& Verb, const FString& Path) const
{
    const UP1NetworkSettings* NetworkSettings = GetDefault<UP1NetworkSettings>();

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(FString::Printf(TEXT("http://%s:%d%s"), *NetworkSettings->AuthServerIp, NetworkSettings->AuthServerPort, *Path));
    Request->SetVerb(Verb);
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetTimeout(10.0f);

    if (HasLoginSession())
        Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *LoginSession));

    return Request;
}

bool UP1AuthClient::ReadEnvelope(FHttpResponsePtr Response, bool bWasSuccessful, TSharedPtr<FJsonObject>& OutData, FString& OutErrorCode, FString& OutMessage)
{
    if (bWasSuccessful == false || Response.IsValid() == false)
        return false;

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
    if (FJsonSerializer::Deserialize(Reader, JsonObject) == false || JsonObject.IsValid() == false)
        return false;

    // 응답은 성공이면 { ok: true, data }, 실패면 { ok: false, error: { code, message } }다. 성공 여부는 상태 코드가 아니라 ok로 가른다.
    bool bOk = false;
    JsonObject->TryGetBoolField(TEXT("ok"), bOk);

    const TSharedPtr<FJsonObject>* DataObject = nullptr;
    if (bOk && JsonObject->TryGetObjectField(TEXT("data"), DataObject))
    {
        OutData = *DataObject;
        return true;
    }

    // 서버의 문구는 화면에 그대로 보여 주는 한국어 문장이다.
    const TSharedPtr<FJsonObject>* ErrorObject = nullptr;
    if (JsonObject->TryGetObjectField(TEXT("error"), ErrorObject))
    {
        (*ErrorObject)->TryGetStringField(TEXT("code"), OutErrorCode);
        (*ErrorObject)->TryGetStringField(TEXT("message"), OutMessage);
    }

    return false;
}
