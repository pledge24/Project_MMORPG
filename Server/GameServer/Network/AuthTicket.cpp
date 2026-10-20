#include "Core/pch.h"
#include "Network/AuthTicket.h"
#include <charconv>

optional<int64> AuthTicket::ParseAccountId(const string& value)
{
    // 형식(앞자리 0과 부호가 없는 10진수, 1 이상)은 인증 서버가 정한다. 한쪽만 바꾸면 모든 로그인이 여기서 거절된다.
    // from_chars는 '-'와 앞자리 0을 받아들이므로 숫자만 있는지, 첫 자리가 0이 아닌지를 먼저 본다.
    if (value.empty() || value[0] == '0')
        return nullopt;
    for (char c : value)
    {
        if (c < '0' || c > '9')
            return nullopt;
    }

    // int64를 넘는 값은 result_out_of_range로 거절된다.
    int64 accountId = 0;
    const char* end = value.data() + value.size();
    auto [ptr, ec] = std::from_chars(value.data(), end, accountId);
    if (ec != std::errc() || ptr != end)
        return nullopt;

    return accountId;
}
