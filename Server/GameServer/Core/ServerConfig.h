#pragma once
#include <functional>
#include <optional>

/*----------------------
      ServerConfig
-----------------------*/
// 게임 서버의 접속 정보다. 기본값은 로컬 개발 환경이고, 환경 변수가 있으면 그 값을 쓴다.
//   P1_GAME_DB_CONNECTION_STRING  GameDB ODBC 접속 문자열
//   P1_REDIS_URI                  Redis URI
//   P1_GAME_SERVER_PORT           게임 서버가 여는 포트
struct ServerConfig
{
    wstring dbConnectionString;
    string redisUri;
    uint16 port = 0;

    // 이름을 받아 값이 있으면 돌려준다. 테스트는 실제 환경 대신 가짜 조회를 넘긴다.
    using EnvLookup = std::function<std::optional<string>(const char* name)>;

    static ServerConfig Load(const EnvLookup& lookup);

    // 프로세스 환경 변수를 읽는다. 없거나 빈 값이면 nullopt.
    static std::optional<string> ReadProcessEnv(const char* name);
};
