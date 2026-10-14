#include "Core/pch.h"
#include "Network/AccessToken.h"

optional<AccessToken::Payload> AccessToken::ParsePayload(const string& value)
{
    // 형식(JSON 객체, userId는 정수, username은 문자열)은 인증 서버가 정한다. 한쪽만 바꾸면 모든 로그인이 여기서 거절된다.
    // 예외를 던지지 않는 해석을 쓴다. 틀린 값이면 discarded가 나온다.
    const Json json = Json::parse(value, nullptr, false);
    if (json.is_object() == false)
        return nullopt;

    // operator[]로 읽으면 없는 키가 null이 되어 변환이 예외를 던진다. 키와 타입을 먼저 본다.
    auto userId = json.find("userId");
    auto username = json.find("username");
    if (userId == json.end() || userId->is_number_integer() == false)
        return nullopt;
    if (username == json.end() || username->is_string() == false)
        return nullopt;

    Payload payload;
    payload.userId = userId->get<int64>();
    payload.username = username->get<string>();
    return payload;
}
