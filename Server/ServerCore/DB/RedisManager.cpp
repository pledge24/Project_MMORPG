#include "ServerCore/Core/pch.h"
#include "ServerCore/DB/RedisManager.h"

/*------------------
    RedisManager
-------------------*/

RedisManager::RedisManager()
{
}

RedisManager::~RedisManager()
{
}

bool RedisManager::Connect(const string& uri)
{
    try
    {
        _uri = uri;
        _redis = make_shared<Redis>(uri);
        GLogger->Info("Redis에 연결했다");
    }
    catch (const Error& err)
    {
        GLogger->Error("Redis에 연결하지 못했다: {}", err.what());
        return false;
    }

    return true;
}
