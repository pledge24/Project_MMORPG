#include "pch.h"
#include "RedisManager.h"

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
        wcout << L"Redis++로 레디스 연결 성공!" << endl;
    }
    catch (const Error& err)
    {
        wcerr << L"Redis 오류: " << err.what() << endl;
        return false;
    }

    return true;
}
