#pragma once
#include <functional>
#include <optional>

/**
 * 게임 서버의 설정 값.
 *
 * 서버를 띄우는 데 필요한 주소, 접속 문자열, 스레드와 세션 수, 노드 번호를 한곳에 모은다.
 * 값마다 기본값이 있고, 환경 변수로 덮어쓸 수 있다.
 */
struct Config
{
    /** 이름으로 환경 변수를 찾는다. 테스트는 실제 환경 대신 가짜 조회를 넘긴다. */
    using EnvLookup = std::function<std::optional<string>(const char* name)>;

    /** 기본값이 빌드 구성에 따라 다른 설정이 있다. 테스트는 두 구성을 모두 넘겨 본다. */
    enum class BuildConfiguration { Debug, Release };

#ifdef _DEBUG
    static constexpr BuildConfiguration CURRENT_BUILD = BuildConfiguration::Debug;
#else
    static constexpr BuildConfiguration CURRENT_BUILD = BuildConfiguration::Release;
#endif

    /**
     * 기본값에 lookup이 찾은 환경 변수를 덮어쓴다. 형식이 틀린 값은 버리고 기본값을 둔다.
     * 노드 번호만은 틀린 값을 기본값으로 바꾸지 않으므로 Validate로 거른다.
     */
    static Config Load(const EnvLookup& lookup, BuildConfiguration build = CURRENT_BUILD);

    /** 프로세스 환경 변수를 읽는다. 없거나 빈 값이면 nullopt. */
    static std::optional<string> ReadProcessEnv(const char* name);

    /** 값끼리의 관계, 주소 형식, 노드 번호를 본다. 틀렸으면 고칠 환경 변수를 담은 사유를 돌려준다. */
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
    /** ID 발급기의 노드 번호(0~1023). 주지 않았거나(Release만) 정수가 아니면 nullopt. */
    std::optional<int32> serverNodeId;

    /** 이 서버가 맡은 월드의 번호(1 이상). 인증 티켓은 이 월드에 발급된 것만 받는다. */
    int32 worldId = 0;
    /** 월드 목록에 보이는 이름(UTF-8). */
    string worldName;
    /** 클라이언트가 이 월드에 접속할 IPv4 주소. 포트는 리슨 포트(port)다. */
    string worldHost;
    /** 혼잡도를 정하는 기준이 되는 연결 계정 수. 이 수를 넘는 접속을 막지는 않는다. */
    int32 worldCapacity = 0;
    /** 맵마다 만드는 룸(채널)의 수(1~20). 1이면 맵 하나에 룸 하나다. */
    int32 channelCount = 0;
};
