#include "Core/pch.h"
#include "Network/SaveGate.h"

SaveGate GSaveGate;

void SaveGate::Hold(int64 userId)
{
    USE_LOCK;

    // 밀어낼 때 한 번, 접속 종료 때 한 번 불린다. 저장은 한 번이므로 이미 걸린 대기를 새로 만들지 않는다.
    _entries.try_emplace(userId);
}

SaveGate::ParkTicket SaveGate::Park(int64 userId, ParkedLoad load)
{
    USE_LOCK;

    auto it = _entries.find(userId);
    if (it == _entries.end())
        return { ParkResult::NOT_HELD, 0 };

    Entry& entry = it->second;
    if (entry.parked.has_value())
        return { ParkResult::BUSY, 0 };

    entry.parked = std::move(load);
    entry.token = _nextToken++;
    return { ParkResult::PARKED, entry.token };
}

optional<SaveGate::ParkedLoad> SaveGate::Release(int64 userId)
{
    USE_LOCK;

    auto it = _entries.find(userId);
    if (it == _entries.end())
        return nullopt;

    optional<ParkedLoad> parked = std::move(it->second.parked);
    _entries.erase(it);
    return parked;
}

optional<SaveGate::ParkedLoad> SaveGate::Expire(int64 userId, uint64 token)
{
    USE_LOCK;

    auto it = _entries.find(userId);
    if (it == _entries.end() || it->second.parked.has_value() == false || it->second.token != token)
        return nullopt;

    optional<ParkedLoad> parked = std::move(it->second.parked);
    _entries.erase(it);
    return parked;
}
