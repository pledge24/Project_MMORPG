#pragma once

/*------------------
	RedisManager
-------------------*/

// Redis 연결 하나를 들고 있다. 전역 객체 GRedisManager 하나만 있다.
// 인증 서버가 넣은 액세스 토큰을 읽는 데 쓴다.
class RedisManager
{
public:
    RedisManager();
    ~RedisManager();

    // 실패하면 false를 돌려준다.
    bool Connect(const string& uri);
    // Connect 전에는 nullptr를 돌려준다.
    RedisRef GetRedis() { return _redis; }

private:
    string _uri = "";
    RedisRef _redis;
};

