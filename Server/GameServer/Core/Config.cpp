#include "Core/pch.h"
#include "Core/Config.h"
#include "Utils/EncodingConverter.h"
#include <cstdlib>

namespace
{
    const wchar_t* DEFAULT_DB_CONNECTION_STRING =
        L"Driver={ODBC Driver 17 for SQL Server};Server=(localdb)\\ProjectModels;Database=GameDB;Trusted_Connection=Yes;";
    const char* DEFAULT_REDIS_URI = "tcp://127.0.0.1:6379";
    constexpr uint16 DEFAULT_PORT = 7777;

    // 1~65535가 아니거나 숫자가 아니면 nullopt.
    std::optional<uint16> ParsePort(const string& text)
    {
        try
        {
            size_t parsedLength = 0;
            const int value = std::stoi(text, &parsedLength);
            if (parsedLength != text.size() || value < 1 || value > 65535)
                return std::nullopt;

            return static_cast<uint16>(value);
        }
        catch (const std::exception&)
        {
            return std::nullopt;
        }
    }
}

Config Config::Load(const EnvLookup& lookup)
{
    Config config;
    config.dbConnectionString = DEFAULT_DB_CONNECTION_STRING;
    config.redisUri = DEFAULT_REDIS_URI;
    config.port = DEFAULT_PORT;

    if (std::optional<string> value = lookup("P1_GAME_DB_CONNECTION_STRING"))
        config.dbConnectionString = EncodingConverter::StringToWString(value.value());

    if (std::optional<string> value = lookup("P1_REDIS_URI"))
        config.redisUri = value.value();

    if (std::optional<string> value = lookup("P1_GAME_SERVER_PORT"))
    {
        if (std::optional<uint16> port = ParsePort(value.value()))
            config.port = port.value();
        else
            cout << "P1_GAME_SERVER_PORT 값이 포트가 아니라 기본값(" << DEFAULT_PORT << ")을 쓴다: " << value.value() << '\n';
    }

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
