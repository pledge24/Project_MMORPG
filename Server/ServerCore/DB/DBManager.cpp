#include "pch.h"
#include "DBQueue.h"
#include "DBManager.h"
#include <functional>

DBManager::DBManager()
{

}

DBManager::~DBManager()
{
    Clear();
}

void DBManager::Init(int32 dbQueueCount)
{
    _dbQueueCount = dbQueueCount;
    _dbQueueList.clear();

    for (int32 dbQueueId = 0; dbQueueId < dbQueueCount; dbQueueId++)
    {
        DBQueueRef dbQueue = make_shared<DBQueue>(dbQueueId);
        _dbQueueList.push_back(dbQueue);
    }
}

void DBManager::Clear()
{
    _dbQueueList.clear();
    _dbQueueCount = 0;
}

DBQueueRef DBManager::GetDBQueue(int32 index)
{
    return _dbQueueList[index];
}

DBQueueRef DBManager::GetDBQueueFromId(int64 id)
{
    // 해시값은 size_t(부호 없음)다. int64로 바꾸면 최상위 비트가 선 값이 음수가 되어 인덱스도 음수가 되므로,
    // 큐 개수를 size_t로 맞춰 부호 없는 나머지를 구한다.
    const size_t index = _hashGenerator(id) % static_cast<size_t>(_dbQueueCount);
    return _dbQueueList[index];
}
