#include "Core/pch.h"
#include "Network/WorldListing.h"

namespace
{
    // 키 이름은 인증 서버와의 계약이다. 바꾸면 인증 서버가 이 월드를 찾지 못한다.
    const char* WORLD_SET_KEY = "worlds";

    string MakeEntryKey(int32 worldId)
    {
        return format("world:{}", worldId);
    }
}

WorldCongestion WorldListing::ComputeCongestion(int32 connectedAccountCount, int32 capacity)
{
    // 비율을 실수로 나누지 않고 정수로 비교한다. 경계(정확히 50%, 80%)에서 반올림 오차로 단계가 갈리지 않게 하려는 것이다.
    const int64 scaledCount = static_cast<int64>(connectedAccountCount) * 10;
    const int64 scaledCapacity = static_cast<int64>(capacity);

    if (scaledCount < scaledCapacity * 5)
        return WorldCongestion::Smooth;

    if (scaledCount < scaledCapacity * 8)
        return WorldCongestion::Normal;

    return WorldCongestion::Crowded;
}

const char* WorldListing::ToContractName(WorldCongestion congestion)
{
    switch (congestion)
    {
    case WorldCongestion::Smooth: return "SMOOTH";
    case WorldCongestion::Normal: return "NORMAL";
    case WorldCongestion::Crowded: return "CROWDED";
    }

    return "CROWDED";
}

void WorldListing::Register(Redis& redis, int32 worldId)
{
    // 집합에는 TTL이 없다. 꺼진 월드는 번호가 남아도 항목이 만료되어 목록에서 빠진다.
    redis.sadd(WORLD_SET_KEY, to_string(worldId));
}

void WorldListing::Publish(Redis& redis, const WorldEntry& entry)
{
    // 항목 쓰기와 TTL을 한 트랜잭션으로 보낸다. 둘 사이에 서버가 죽으면 TTL 없는 항목이 남아 꺼진 월드가 목록에 계속 보인다.
    // 이 머신의 Redis(3.0)는 HSET에 필드를 하나만 받으므로 HMSET을 쓴다.
    const string key = MakeEntryKey(entry.worldId);
    redis.transaction()
        .hmset(key, {
            make_pair(string("name"), entry.name),
            make_pair(string("host"), entry.host),
            make_pair(string("port"), to_string(entry.port)),
            make_pair(string("congestion"), string(ToContractName(entry.congestion))),
        })
        .expire(key, ENTRY_TTL_SECONDS)
        .exec();
}
