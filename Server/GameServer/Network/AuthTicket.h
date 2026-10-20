#pragma once

/**
 * 인증 서버가 발급한 인증 티켓의 값.
 *
 * 티켓 값을 읽어 로그인한 계정이 누구인지 알려 준다. 값의 형식은 인증 서버가 정한다.
 */
namespace AuthTicket
{
    /** 티켓 값에서 계정 번호를 읽는다. 형식이 틀리면 nullopt. 어느 스레드에서 불러도 된다. */
    optional<int64> ParseAccountId(const string& value);
}
