#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Core/Config.h"

/*--------------------------------------------------------------
    서버 설정 로더 테스트

    Config::Load는 환경 변수 조회를 인자로 받는다. 실제 프로세스 환경을
    건드리지 않고 가짜 조회로 기본값과 덮어쓰기, 값끼리의 관계 검증을 확인한다.
---------------------------------------------------------------*/

namespace
{
    Config::EnvLookup FakeEnv(map<string, string> values)
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
    const Config config = Config::Load(FakeEnv({}));

    EXPECT_NE(config.dbConnectionString.find(L"(localdb)\\ProjectModels"), wstring::npos);
    EXPECT_NE(config.dbConnectionString.find(L"Database=GameDB"), wstring::npos);
    EXPECT_EQ(config.redisUri, "tcp://127.0.0.1:6379");
    EXPECT_EQ(config.port, 7777);
    EXPECT_EQ(config.bindAddress, "127.0.0.1");
    EXPECT_EQ(config.maxSessionCount, 30);
    EXPECT_EQ(config.workerThreadCount, 5);
    EXPECT_EQ(config.dbThreadCount, 5);
    EXPECT_EQ(config.dbConnectionCount, 6) << "DB 스레드마다 하나와 부팅 때 main이 쓰는 하나";
    EXPECT_FALSE(config.Validate().has_value());
}

TEST(ServerConfigTest, EnvOverridesEachValue)
{
    const Config config = Config::Load(FakeEnv({
        {"P1_GAME_DB_CONNECTION_STRING", "Driver={ODBC Driver 17 for SQL Server};Database=GameDB_Test;"},
        {"P1_REDIS_URI", "tcp://127.0.0.1:6380"},
        {"P1_GAME_SERVER_PORT", "7778"},
        {"P1_GAME_SERVER_BIND_ADDRESS", "0.0.0.0"},
        {"P1_GAME_SERVER_MAX_SESSIONS", "100"},
        {"P1_GAME_SERVER_WORKER_THREADS", "3"},
        {"P1_GAME_DB_THREADS", "2"},
        {"P1_GAME_DB_CONNECTIONS", "4"}}));

    EXPECT_EQ(config.dbConnectionString, L"Driver={ODBC Driver 17 for SQL Server};Database=GameDB_Test;");
    EXPECT_EQ(config.redisUri, "tcp://127.0.0.1:6380");
    EXPECT_EQ(config.port, 7778);
    EXPECT_EQ(config.bindAddress, "0.0.0.0");
    EXPECT_EQ(config.maxSessionCount, 100);
    EXPECT_EQ(config.workerThreadCount, 3);
    EXPECT_EQ(config.dbThreadCount, 2);
    EXPECT_EQ(config.dbConnectionCount, 4);
    EXPECT_FALSE(config.Validate().has_value());
}

// 연결 수를 따로 주지 않으면 DB 스레드 수를 따라간다. 스레드만 늘리고 풀을 그대로 두면 잡이 연결을 빌리지 못한다.
TEST(ServerConfigTest, DbConnectionCountFollowsDbThreadCount)
{
    const Config config = Config::Load(FakeEnv({{"P1_GAME_DB_THREADS", "8"}}));

    EXPECT_EQ(config.dbConnectionCount, 9);
    EXPECT_FALSE(config.Validate().has_value());
}

// 연결 풀은 DB 스레드마다 하나와 부팅 때 main이 쓰는 하나가 있어야 한다. 모자라면 동시에 돈 잡 하나가 연결을 빌리지 못한다.
TEST(ServerConfigTest, TooFewDbConnectionsIsRejected)
{
    const Config config = Config::Load(FakeEnv({
        {"P1_GAME_DB_THREADS", "5"},
        {"P1_GAME_DB_CONNECTIONS", "5"}}));

    ASSERT_TRUE(config.Validate().has_value());
    EXPECT_NE(config.Validate()->find("P1_GAME_DB_CONNECTIONS"), string::npos) << "고칠 변수를 알린다";
}

TEST(ServerConfigTest, InvalidBindAddressIsRejected)
{
    const Config config = Config::Load(FakeEnv({{"P1_GAME_SERVER_BIND_ADDRESS", "localhost"}}));

    ASSERT_TRUE(config.Validate().has_value());
    EXPECT_NE(config.Validate()->find("P1_GAME_SERVER_BIND_ADDRESS"), string::npos);
}

// 개수가 숫자가 아니거나 1보다 작으면 기본값으로 뜬다. 0개의 워커나 DB 스레드로는 아무 요청도 처리하지 못한다.
TEST(ServerConfigTest, InvalidCountFallsBackToDefault)
{
    for (const char* invalidCount : {"abc", "0", "-3", "5x"})
    {
        SCOPED_TRACE(invalidCount);

        const Config config = Config::Load(FakeEnv({
            {"P1_GAME_SERVER_MAX_SESSIONS", invalidCount},
            {"P1_GAME_SERVER_WORKER_THREADS", invalidCount},
            {"P1_GAME_DB_THREADS", invalidCount},
            {"P1_GAME_DB_CONNECTIONS", invalidCount}}));
        EXPECT_EQ(config.maxSessionCount, 30);
        EXPECT_EQ(config.workerThreadCount, 5);
        EXPECT_EQ(config.dbThreadCount, 5);
        EXPECT_EQ(config.dbConnectionCount, 6);
    }
}

// 포트가 잘못되면 서버가 엉뚱한 포트를 열지 않고 기본값으로 뜬다.
TEST(ServerConfigTest, InvalidPortFallsBackToDefault)
{
    for (const char* invalidPort : {"abc", "0", "65536", "7777x", "-1"})
    {
        SCOPED_TRACE(invalidPort);

        const Config config = Config::Load(FakeEnv({{"P1_GAME_SERVER_PORT", invalidPort}}));
        EXPECT_EQ(config.port, 7777);
    }
}
