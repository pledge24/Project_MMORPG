#pragma once

/** 정원에 비춘 월드의 혼잡도. 월드 선택을 막지 않는다. */
enum class WorldCongestion : uint8
{
    Smooth,     // 연결 계정이 정원의 50% 미만
    Normal,     // 50% 이상 80% 미만
    Crowded,    // 80% 이상
};

/** 월드 목록에 알릴 이 서버의 월드 항목. */
struct WorldEntry
{
    int32 worldId = 0;
    string name;
    string host;
    uint16 port = 0;
    WorldCongestion congestion = WorldCongestion::Smooth;
};

/**
 * 인증 서버의 월드 목록에 이 서버의 월드를 알리는 곳.
 *
 * 혼잡도를 정하고, 월드 번호와 월드 항목을 Redis에 쓴다. 키와 필드의 형식은 인증 서버와의 계약이다.
 */
namespace WorldListing
{
    /** 월드 항목을 다시 쓰는 주기(ms). 항목의 TTL보다 충분히 짧아야 살아 있는 월드가 목록에서 깜빡이지 않는다. */
    constexpr uint64 REFRESH_INTERVAL_MS = 5000;

    /** 월드 항목의 TTL(초). 서버가 꺼지면 이 시간 안에 월드가 목록에서 빠진다. */
    constexpr int32 ENTRY_TTL_SECONDS = 15;

    /** capacity는 1 이상이어야 한다. 정원을 넘어도 혼잡이다. */
    WorldCongestion ComputeCongestion(int32 connectedAccountCount, int32 capacity);

    /** 월드 항목에 쓰는 혼잡도 값. 인증 서버가 이 이름으로 읽는다. */
    const char* ToContractName(WorldCongestion congestion);

    /** 월드 번호 집합에 worldId를 넣는다. 실패하면 Redis 예외를 그대로 던진다. */
    void Register(Redis& redis, int32 worldId);

    /** 월드 항목을 쓰고 TTL을 다시 건다. 실패하면 Redis 예외를 그대로 던진다. */
    void Publish(Redis& redis, const WorldEntry& entry);
}
