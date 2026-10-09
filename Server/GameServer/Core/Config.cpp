#include "Core/pch.h"
#include "Core/Config.h"
#include "Utils/EncodingConverter.h"
#include <cstdlib>

namespace
{
    const wchar_t* DEFAULT_DB_CONNECTION_STRING =
        L"Driver={ODBC Driver 17 for SQL Server};Server=(localdb)\\ProjectModels;Database=GameDB;Trusted_Connection=Yes;";
    const char* DEFAULT_REDIS_URI = "tcp://127.0.0.1:6379";
    const char* DEFAULT_BIND_ADDRESS = "127.0.0.1";
    constexpr uint16 DEFAULT_PORT = 7777;
    constexpr int32 DEFAULT_MAX_SESSION_COUNT = 30;
    constexpr int32 DEFAULT_WORKER_THREAD_COUNT = 5;
    constexpr int32 DEFAULT_DB_THREAD_COUNT = 5;

    // min~max가 아니거나 숫자가 아니면 nullopt.
    std::optional<int32> ParseInt(const string& text, int32 min, int32 max)
    {
        try
        {
            size_t parsedLength = 0;
            const int value = std::stoi(text, &parsedLength);
            if (parsedLength != text.size() || value < min || value > max)
                return std::nullopt;

            return value;
        }
        catch (const std::exception&)
        {
            return std::nullopt;
        }
    }

    // 환경 변수가 있으면 min~max의 정수로 읽어 value에 쓴다. 틀린 값이면 기본값(value)을 두고 로그를 남긴다.
    template<typename T>
    void OverrideInt(const Config::EnvLookup& lookup, const char* name, int32 min, int32 max, IN OUT T& value)
    {
        std::optional<string> text = lookup(name);
        if (text.has_value() == false)
            return;

        if (std::optional<int32> parsed = ParseInt(text.value(), min, max))
            value = static_cast<T>(parsed.value());
        else
            GLogger->Warning("{} 값이 {}~{}의 정수가 아니라 기본값({})을 쓴다: {}", name, min, max, value, text.value());
    }
}

Config Config::Load(const EnvLookup& lookup)
{
    Config config;
    config.dbConnectionString = DEFAULT_DB_CONNECTION_STRING;
    config.redisUri = DEFAULT_REDIS_URI;
    config.bindAddress = DEFAULT_BIND_ADDRESS;
    config.port = DEFAULT_PORT;
    config.maxSessionCount = DEFAULT_MAX_SESSION_COUNT;
    config.workerThreadCount = DEFAULT_WORKER_THREAD_COUNT;
    config.dbThreadCount = DEFAULT_DB_THREAD_COUNT;

    if (std::optional<string> value = lookup("P1_GAME_DB_CONNECTION_STRING"))
        config.dbConnectionString = EncodingConverter::StringToWString(value.value());

    if (std::optional<string> value = lookup("P1_REDIS_URI"))
        config.redisUri = value.value();

    if (std::optional<string> value = lookup("P1_GAME_SERVER_BIND_ADDRESS"))
        config.bindAddress = value.value();

    OverrideInt(lookup, "P1_GAME_SERVER_PORT", 1, 65535, config.port);
    OverrideInt(lookup, "P1_GAME_SERVER_MAX_SESSIONS", 1, INT32_MAX, config.maxSessionCount);
    OverrideInt(lookup, "P1_GAME_SERVER_WORKER_THREADS", 1, INT32_MAX, config.workerThreadCount);
    OverrideInt(lookup, "P1_GAME_DB_THREADS", 1, INT32_MAX, config.dbThreadCount);

    // 연결 수는 DB 스레드 수가 정해진 뒤에 기본값을 정한다. 스레드만 늘리고 풀을 그대로 두면 잡이 연결을 빌리지 못한다.
    config.dbConnectionCount = config.dbThreadCount;
    OverrideInt(lookup, "P1_GAME_DB_CONNECTIONS", 1, INT32_MAX, config.dbConnectionCount);

    return config;
}

std::optional<string> Config::ReadProcessEnv(const char* name)
{
    char* buffer = nullptr;
    size_t length = 0;
    if (_dupenv_s(&buffer, &length, name) != 0 || buffer == nullptr)
        return std::nullopt;

    string value(buffer);
    free(buffer);

    if (value.empty())
        return std::nullopt;

    return value;
}

std::optional<string> Config::Validate() const
{
    IN_ADDR address = {};
    if (::inet_pton(AF_INET, bindAddress.c_str(), &address) != 1)
        return format("P1_GAME_SERVER_BIND_ADDRESS({})가 IPv4 주소가 아니다", bindAddress);

    if (dbConnectionCount < dbThreadCount)
        return format("P1_GAME_DB_CONNECTIONS({})가 DB 스레드 수({})보다 작다. DB 스레드마다 연결이 하나씩 있어야 한다",
            dbConnectionCount, dbThreadCount);

    return std::nullopt;
}
