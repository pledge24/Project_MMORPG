#pragma once
#include <functional>
#include <optional>

/**
 * 게임 서버의 설정 정보. 값마다 아래 환경 변수가 있으면 덮어쓴다. 없거나 틀린 값이면 기본값을 쓴다.
 * - P1_GAME_DB_CONNECTION_STRING:  GameDB ODBC 접속 문자열
 * - P1_REDIS_URI:                  Redis URI
 * - P1_GAME_SERVER_BIND_ADDRESS:   게임 서버가 리슨할 IPv4 주소
 * - P1_GAME_SERVER_PORT:           게임 서버가 여는 포트
 * - P1_GAME_SERVER_MAX_SESSIONS:   동시에 접속할 수 있는 세션 수
 * - P1_GAME_SERVER_WORKER_THREADS: 메인 스레드 말고 띄울 IOCP 워커 스레드 수
 * - P1_GAME_DB_THREADS:            DB 스레드 수. DB 큐마다 하나씩 붙는다
 * - P1_GAME_DB_CONNECTIONS:        SQL Server 연결 풀의 크기. 없으면 DB 스레드 수
 */
struct Config
{
    wstring dbConnectionString;
    string redisUri;
    string bindAddress;
    uint16 port = 0;
    int32 maxSessionCount = 0;
    int32 workerThreadCount = 0;
    int32 dbThreadCount = 0;
    int32 dbConnectionCount = 0;

    // 이름을 받아 값이 있으면 돌려준다. 테스트는 실제 환경 대신 가짜 조회를 넘긴다.
    using EnvLookup = std::function<std::optional<string>(const char* name)>;

    static Config Load(const EnvLookup& lookup);

    // 프로세스 환경 변수를 읽는다. 없거나 빈 값이면 nullopt.
    static std::optional<string> ReadProcessEnv(const char* name);

    /**
     * 값끼리의 관계와 주소 형식을 본다. 틀렸으면 고칠 환경 변수를 담은 사유를 돌려준다. 서버는 그때 뜨지 않는다.
     * 연결 풀은 DB 스레드마다 하나가 있어야 한다. 부팅 때 main은 DB 스레드를 띄우기 전에 연결을 빌렸다가 돌려준다.
     */
    std::optional<string> Validate() const;
};
