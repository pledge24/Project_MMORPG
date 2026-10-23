#include "Core/pch.h"
#include "Network/AuthTicket.h"
#include <charconv>

optional<string> RedisAuthTicketStore::Get(const string& key)
{
    return GRedisManager->GetRedis()->get(key);
}

bool RedisAuthTicketStore::Delete(const string& key)
{
    return GRedisManager->GetRedis()->del(key) > 0;
}

string AuthTicket::MakeKey(int32 worldId, const string& ticket)
{
    // 키에 월드 번호가 들어가므로 다른 월드에 발급한 티켓은 이 서버가 찾지 못한다.
    return format("authTicket:{}:{}", worldId, ticket);
}

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

optional<int64> AuthTicket::Consume(AuthTicketStore& store, int32 worldId, const string& ticket)
{
    // 인증 티켓은 한 번만 쓴다. 같은 티켓의 로비 접속이 다른 DB 큐에서 동시에 와도 키를 지운 쪽 하나만 통과한다.
    // 이 머신의 Redis(3.0)에는 GETDEL이 없어서 GET 뒤에 DEL을 부르고, DEL이 지운 쪽만 통과시킨다.
    const string key = MakeKey(worldId, ticket);
    optional<string> value = store.Get(key);
    if (value.has_value() == false || store.Delete(key) == false)
    {
        GLogger->Warning("인증 티켓이 없거나, 이미 쓰였거나, 다른 월드의 것이다");
        return nullopt;
    }

    // 티켓은 이미 지웠다. 값을 읽지 못하면 다시 쓸 수 없으므로 거절한다.
    optional<int64> accountId = ParseAccountId(value.value());
    if (accountId.has_value() == false)
        GLogger->Warning("인증 티켓 값의 형식이 틀렸다");

    return accountId;
}
