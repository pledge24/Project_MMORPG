#include "Core/pch.h"
#include "Game/Room/MonsterSpawner.h"

void MonsterSpawner::Init(const vector<SpawnPointPlacement>& spawnPoints)
{
    _spawnPoints.clear();
    _pendingRespawns.clear();

    for (const SpawnPointPlacement& placement : spawnPoints)
    {
        // 테스트처럼 스폰 그룹 표 없이 마커만 넣은 경우다. 운영 데이터는 기동할 때 대조했다.
        const DesignData::SpawnGroupRow* group = Gamedata::FindSpawnGroup(placement.spawnGroupId);
        if (group == nullptr || group->Entries.empty())
        {
            GLogger->Warning("스폰 그룹 {}이 스폰 그룹 표에 없거나 비어 있어 그 마커를 건너뛴다", placement.spawnGroupId);
            continue;
        }

        _spawnPoints.push_back(SpawnPoint{ placement, *group });
    }
}

vector<int32> MonsterSpawner::GetInitialSpawns() const
{
    vector<int32> spawns;
    for (int32 index = 0; index < static_cast<int32>(_spawnPoints.size()); ++index)
    {
        for (int32 i = 0; i < _spawnPoints[index].group.MaxCount; ++i)
            spawns.push_back(index);
    }
    return spawns;
}

int32 MonsterSpawner::PickMonster(int32 spawnPointIndex) const
{
    // 가중치는 1 이상이다(DataBuilder가 검사한다).
    const vector<DesignData::SpawnEntryRow>& entries = _spawnPoints[spawnPointIndex].group.Entries;
    int32 totalWeight = 0;
    for (const DesignData::SpawnEntryRow& entry : entries)
        totalWeight += entry.Weight;

    int32 pick = Utils::GetRandom(1, totalWeight);
    for (const DesignData::SpawnEntryRow& entry : entries)
    {
        pick -= entry.Weight;
        if (pick <= 0)
            return entry.MonsterId;
    }

    return entries.back().MonsterId;
}

void MonsterSpawner::OnMonsterDied(int32 spawnPointIndex)
{
    if (spawnPointIndex < 0 || spawnPointIndex >= static_cast<int32>(_spawnPoints.size()))
        return;

    const float respawnTime = static_cast<float>(_spawnPoints[spawnPointIndex].group.RespawnSec);
    _pendingRespawns.push_back(PendingRespawn{ spawnPointIndex, respawnTime });
}

vector<int32> MonsterSpawner::Tick(float deltaTime)
{
    vector<int32> due;
    for (PendingRespawn& respawn : _pendingRespawns)
    {
        respawn.remainingTime -= deltaTime;
        if (respawn.remainingTime <= 0.f)
            due.push_back(respawn.spawnPointIndex);
    }

    std::erase_if(_pendingRespawns, [](const PendingRespawn& respawn) { return respawn.remainingTime <= 0.f; });
    return due;
}
