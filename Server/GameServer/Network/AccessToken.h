#pragma once

/**
 * 인증 서버가 Redis의 토큰 키에 넣어 둔 값. 로그인 잡이 토큰 키를 읽고 지운 뒤 이 값으로 계정을 안다.
 * 값의 형식은 인증 서버가 정한다(JSON 객체, userId는 정수, username은 문자열).
 */
namespace AccessToken
{
    struct Payload
    {
        int64 userId = 0;
        string username;
    };

    /** 토큰 값을 읽는다. 상태가 없으므로 어느 스레드에서 불러도 된다. */
    optional<Payload> ParsePayload(const string& value);
}
