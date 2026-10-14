#pragma once
#include <functional>
#include <optional>

/**
 * 게임 서버의 설정 값.
 *
 * 서버를 띄우는 데 필요한 주소, 접속 문자열, 스레드와 세션 수를 한곳에 모은다.
 * 값마다 기본값이 있고, 환경 변수로 덮어쓸 수 있다.
 */
struct Config
{
    /** 이름으로 환경 변수를 찾는다. 테스트는 실제 환경 대신 가짜 조회를 넘긴다. */
    using EnvLookup = std::function<std::optional<string>(const char* name)>;

    /** 기본값에 lookup이 찾은 환경 변수를 덮어쓴다. 형식이 틀린 값은 버리고 기본값을 둔다. */
    static Config Load(const EnvLookup& lookup);

    /** 프로세스 환경 변수를 읽는다. 없거나 빈 값이면 nullopt. */
    static std::optional<string> ReadProcessEnv(const char* name);

    /** 값끼리의 관계와 주소 형식을 본다. 틀렸으면 고칠 환경 변수를 담은 사유를 돌려준다. */
    std::optional<string> Validate() const;

    /** GameDB ODBC 접속 문자열. */
    wstring dbConnectionString;
    string redisUri;
    /** 리슨할 IPv4 주소. */
    string bindAddress;
    uint16 port = 0;
    int32 maxSessionCount = 0;
    /** 메인 스레드 말고 띄울 IOCP 워커 스레드 수. */
    int32 workerThreadCount = 0;
    /** DB 큐마다 하나씩 붙는 DB 스레드의 수. */
    int32 dbThreadCount = 0;
    /** SQL Server 연결 풀의 크기. DB 스레드 수보다 작으면 안 된다. */
    int32 dbConnectionCount = 0;
};
