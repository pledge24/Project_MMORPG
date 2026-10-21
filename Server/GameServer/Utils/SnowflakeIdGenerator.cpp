#include "Core/pch.h"
#include "Utils/SnowflakeIdGenerator.h"
#include <chrono>

namespace
{
    constexpr int32 NODE_SHIFT = SnowflakeIdGenerator::SEQUENCE_BITS;
    constexpr int32 TIME_SHIFT = SnowflakeIdGenerator::SEQUENCE_BITS + SnowflakeIdGenerator::NODE_BITS;
    constexpr int64 NODE_MASK = (int64{ 1 } << SnowflakeIdGenerator::NODE_BITS) - 1;
    constexpr int64 SEQUENCE_MASK = (int64{ 1 } << SnowflakeIdGenerator::SEQUENCE_BITS) - 1;
}

SnowflakeIdGenerator::SnowflakeIdGenerator(int32 nodeId, Clock clock)
    : _nodeId(nodeId), _clock(std::move(clock)), _lastId(0)
{
}

void SnowflakeIdGenerator::StartAfter(int64 lastIssuedId)
{
    // 시각과 순번만 따른다. 다른 노드의 ID가 넘어와도 이 노드 번호로 이어 가야 ID가 겹치지 않는다.
    const Parts parts = Decompose(lastIssuedId);
    const int64 startId = Compose(parts.elapsedMs, parts.sequence);

    int64 last = _lastId.load();
    while (last < startId && _lastId.compare_exchange_weak(last, startId) == false)
    {
    }
}

int64 SnowflakeIdGenerator::Next()
{
    const int64 now = _clock() - EPOCH_UNIX_MS;

    int64 last = _lastId.load();
    while (true)
    {
        const Parts lastParts = Decompose(last);
        int64 elapsedMs = now;
        int64 sequence = 0;

        // 논리 시계. 시계가 마지막 시각을 넘지 않았으면(같은 밀리초이거나 시계가 뒤로 갔으면) 마지막 시각에서 순번을 올린다.
        // 기다리지 않는다. 룸 스레드가 시계를 기다리면 그 룸의 모든 잡이 함께 멈춘다.
        if (now <= lastParts.elapsedMs)
        {
            elapsedMs = lastParts.elapsedMs;
            sequence = lastParts.sequence + 1;

            // 순번이 넘치면 다음 밀리초를 미리 쓴다. 시계가 따라올 때까지 그 시각에서 순번을 올린다.
            if (sequence > SEQUENCE_MASK)
            {
                elapsedMs++;
                sequence = 0;
            }
        }

        const int64 id = Compose(elapsedMs, sequence);
        if (_lastId.compare_exchange_weak(last, id))
            return id;
    }
}

SnowflakeIdGenerator::Parts SnowflakeIdGenerator::Decompose(int64 id)
{
    Parts parts;
    parts.elapsedMs = id >> TIME_SHIFT;
    parts.nodeId = static_cast<int32>((id >> NODE_SHIFT) & NODE_MASK);
    parts.sequence = static_cast<int32>(id & SEQUENCE_MASK);
    return parts;
}

int64 SnowflakeIdGenerator::NowUnixMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

int64 SnowflakeIdGenerator::Compose(int64 elapsedMs, int64 sequence) const
{
    return (elapsedMs << TIME_SHIFT) | (int64{ _nodeId } << NODE_SHIFT) | sequence;
}
