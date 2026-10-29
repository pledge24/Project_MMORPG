#pragma once
#include "Game/Data/Templates.h"

/**
 * 룸 하나의 스폰 마커마다 몬스터를 언제 몇 마리 둘지 정하는 일정표.
 *
 * 처음에는 마커마다 그 스폰 그룹의 최대 수를 채우고, 사망한 몬스터는 그룹의 리젠 시간 뒤에 같은 마커에 다시 둔다.
 * 몬스터를 만들어 룸에 넣는 일은 룸이 한다. 마커는 Init에 넘긴 순서의 번호(0부터)로 가리킨다. 룸 큐 위에서만 쓴다.
 */
class MonsterSpawner
{
public:
    /** 마커마다 그 스폰 그룹의 행을 찾아 둔다. 스폰 그룹 표에 없는 마커는 건너뛴다(기동할 때 대조했다). */
    void Init(const vector<SpawnPointPlacement>& spawnPoints);

    /** 처음 채울 몬스터마다 그 마커의 번호. 마커마다 그룹의 최대 수만큼 들어 있다. */
    vector<int32> GetInitialSpawns() const;
    /** 번호가 0 이상 마커 수 미만인지 쓰는 쪽이 지킨다. */
    const SpawnPointPlacement& GetSpawnPoint(int32 spawnPointIndex) const { return _spawnPoints[spawnPointIndex].placement; }
    /** 그 마커의 스폰 그룹에서 가중치에 비례해 몬스터 한 종류를 뽑는다. */
    int32 PickMonster(int32 spawnPointIndex) const;

    /** 그 마커에서 나온 몬스터가 사망했다. 그룹의 리젠 시간 뒤의 Tick이 그 마커를 돌려준다. 없는 번호면 무시한다. */
    void OnMonsterDied(int32 spawnPointIndex);
    /** 리젠 시간이 다 된 몬스터마다 그 마커의 번호. deltaTime은 초 단위다. */
    vector<int32> Tick(float deltaTime);

private:
    struct SpawnPoint
    {
        SpawnPointPlacement placement;
        DesignData::SpawnGroupRow group;
    };

    struct PendingRespawn
    {
        int32 spawnPointIndex = 0;
        /** 초. */
        float remainingTime = 0.f;
    };

    vector<SpawnPoint> _spawnPoints;
    vector<PendingRespawn> _pendingRespawns;
};
