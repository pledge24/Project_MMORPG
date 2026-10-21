#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Core/Config.h"

/*--------------------------------------------------------------
    서버 설정 로더 테스트

    Config::Load는 환경 변수 조회와 빌드 구성을 인자로 받는다. 실제 프로세스
    환경을 건드리지 않고 가짜 조회로 기본값과 덮어쓰기, 값끼리의 관계 검증을
    확인한다. 노드 번호처럼 빌드 구성마다 기본값이 다른 설정은 두 구성을 모두 넘긴다.
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

    // 검증이 거절하고, 사유가 고칠 환경 변수를 알리는지 본다.
    void ExpectRejectedFor(const Config& config, const char* envName)
    {
        const std::optional<string> error = config.Validate();
        ASSERT_TRUE(error.has_value());
        EXPECT_NE(error->find(envName), string::npos) << "고칠 변수를 알린다: " << error.value();
    }
}

TEST(ServerConfigTest, UsesLocalDefaultsWithoutEnv)
{
    const Config config = Config::Load(FakeEnv({}));

    EXPECT_NE(config.dbConnectionString.find(L"Server=(localdb)\\P1Game;"), wstring::npos);
    EXPECT_NE(config.dbConnectionString.find(L"Database=GameDB"), wstring::npos);
    EXPECT_EQ(config.redisUri, "tcp://127.0.0.1:6379");
    EXPECT_EQ(config.port, 7777);
    EXPECT_EQ(config.bindAddress, "127.0.0.1");
    EXPECT_EQ(config.maxSessionCount, 30);
    EXPECT_EQ(config.workerThreadCount, 5);
    EXPECT_EQ(config.dbThreadCount, 5);
    EXPECT_EQ(config.dbConnectionCount, 5) << "DB 스레드마다 하나";
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

    EXPECT_EQ(config.dbConnectionCount, 8);
    EXPECT_FALSE(config.Validate().has_value());
}

// 연결 풀은 DB 스레드마다 하나가 있어야 한다. 모자라면 동시에 돈 잡 하나가 연결을 빌리지 못한다.
TEST(ServerConfigTest, TooFewDbConnectionsIsRejected)
{
    const Config config = Config::Load(FakeEnv({
        {"P1_GAME_DB_THREADS", "5"},
        {"P1_GAME_DB_CONNECTIONS", "4"}}));

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
        EXPECT_EQ(config.dbConnectionCount, 5);
    }
}

// 개발 머신에서는 노드가 하나뿐이라 노드 번호를 주지 않아도 0으로 뜬다.
TEST(ServerConfigTest, DebugUsesNodeZeroWithoutEnv)
{
    const Config config = Config::Load(FakeEnv({}), Config::BuildConfiguration::Debug);

    EXPECT_EQ(config.serverNodeId, 0);
    EXPECT_FALSE(config.Validate().has_value());
}

// 운영 서버가 노드 번호 없이 뜨면 다른 노드와 같은 번호(0)로 ID를 발급해 캐릭터 ID와 아이템 UID가 겹친다.
TEST(ServerConfigTest, ReleaseRejectsMissingNodeId)
{
    const Config config = Config::Load(FakeEnv({}), Config::BuildConfiguration::Release);

    ExpectRejectedFor(config, "P1_SERVER_NODE_ID");
}

TEST(ServerConfigTest, NodeIdEnvIsUsedInBothBuilds)
{
    for (Config::BuildConfiguration build : { Config::BuildConfiguration::Debug, Config::BuildConfiguration::Release })
    {
        SCOPED_TRACE(static_cast<int32>(build));

        for (const char* nodeId : { "0", "1023" })
        {
            SCOPED_TRACE(nodeId);

            const Config config = Config::Load(FakeEnv({{"P1_SERVER_NODE_ID", nodeId}}), build);
            EXPECT_EQ(config.serverNodeId, std::stoi(nodeId));
            EXPECT_FALSE(config.Validate().has_value());
        }
    }
}

// 노드 번호는 ID의 10비트에 들어간다. 범위를 넘으면 순번이나 시각 비트를 덮어 ID가 겹친다.
TEST(ServerConfigTest, NodeIdOutOfRangeIsRejected)
{
    for (const char* nodeId : { "-1", "1024", "70000" })
    {
        SCOPED_TRACE(nodeId);

        const Config config = Config::Load(FakeEnv({{"P1_SERVER_NODE_ID", nodeId}}), Config::BuildConfiguration::Debug);
        ExpectRejectedFor(config, "P1_SERVER_NODE_ID");
    }
}

// 정수가 아닌 노드 번호는 Debug에서도 0으로 바꾸지 않고 거절한다. 준 값을 조용히 무시하면 번호가 겹친 줄 모른다.
TEST(ServerConfigTest, NonIntegerNodeIdIsRejectedEvenInDebug)
{
    for (const char* nodeId : { "abc", "5x", "1.5" })
    {
        SCOPED_TRACE(nodeId);

        const Config config = Config::Load(FakeEnv({{"P1_SERVER_NODE_ID", nodeId}}), Config::BuildConfiguration::Debug);
        ExpectRejectedFor(config, "P1_SERVER_NODE_ID");
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
