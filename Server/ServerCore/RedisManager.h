#pragma once

class RedisManager
{
public:
    RedisManager();
    ~RedisManager();

    bool Connect(const string& uri);
    RedisRef GetRedis() { return _redis; }

private:
    string _uri = "";
    RedisRef _redis;
};

