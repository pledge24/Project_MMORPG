#include "pch.h"
#include <gtest/gtest.h>
#include "ServerConfig.h"

/*--------------------------------------------------------------
    서버 접속 정보 로더 테스트

    ServerConfig::Load는 환경 변수 조회를 인자로 받는다. 실제 프로세스 환경을
    건드리지 않고 가짜 조회로 기본값과 덮어쓰기를 확인한다.
---------------------------------------------------------------*/

namespace
{
    ServerConfig::EnvLookup FakeEnv(map<string, string> values)
    {
        return [values](const char* name) -> std::optional<string>
            {
                auto it = values.find(name);
                if (it == values.end())
                    return std::nullopt;

                return it->second;
            };
    }
}

TEST(ServerConfigTest, UsesLocalDefaultsWithoutEnv)
{
    const ServerConfig config = ServerConfig::Load(FakeEnv({}));

    EXPECT_NE(config.dbConnectionString.find(L"(localdb)\\ProjectModels"), wstring::npos);
    EXPECT_NE(config.dbConnectionString.find(L"Database=GameDB"), wstring::npos);
    EXPECT_EQ(config.redisUri, "tcp://127.0.0.1:6379");
    EXPECT_EQ(config.port, 7777);
}

TEST(ServerConfigTest, EnvOverridesEachValue)
{
    const ServerConfig config = ServerConfig::Load(FakeEnv({
        {"P1_GAME_DB_CONNECTION_STRING", "Driver={ODBC Driver 17 for SQL Server};Database=GameDB_Test;"},
        {"P1_REDIS_URI", "tcp://127.0.0.1:6380"},
        {"P1_GAME_SERVER_PORT", "7778"}}));

    EXPECT_EQ(config.dbConnectionString, L"Driver={ODBC Driver 17 for SQL Server};Database=GameDB_Test;");
    EXPECT_EQ(config.redisUri, "tcp://127.0.0.1:6380");
    EXPECT_EQ(config.port, 7778);
}

// 포트가 잘못되면 서버가 엉뚱한 포트를 열지 않고 기본값으로 뜬다.
TEST(ServerConfigTest, InvalidPortFallsBackToDefault)
{
    for (const char* invalidPort : {"abc", "0", "65536", "7777x", "-1"})
    {
        SCOPED_TRACE(invalidPort);

        const ServerConfig config = ServerConfig::Load(FakeEnv({{"P1_GAME_SERVER_PORT", invalidPort}}));
        EXPECT_EQ(config.port, 7777);
    }
}
