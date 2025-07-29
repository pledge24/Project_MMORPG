#include "pch.h"
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
    _dbQueueList.resize(dbQueueCount);
    _dbQueueCount = dbQueueCount;
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

DBQueueRef DBManager::GetDBQueueFromId(int32 id)
{
    int index = _hashGenerator(id) % _dbQueueCount;
    return _dbQueueList[index];
}