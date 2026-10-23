#pragma once

/**
 * 인증 티켓 키를 읽고 지우는 곳.
 *
 * 인증 티켓의 소비가 Redis를 직접 모르게 한다. 실제 서버는 Redis를, 테스트는 메모리 구현을 넘긴다.
 */
class AuthTicketStore
{
public:
    virtual ~AuthTicketStore() = default;

    /** 키가 없으면 nullopt. */
    virtual optional<string> Get(const string& key) = 0;

    /** 키를 실제로 지웠으면 true. 이미 없었으면 false. */
    virtual bool Delete(const string& key) = 0;
};

/**
 * 전역 Redis 연결로 인증 티켓 키를 다루는 곳.
 *
 * 로비 접속 잡이 DB 스레드에서 쓴다. Redis 명령은 응답이 올 때까지 그 스레드를 붙든다.
 */
class RedisAuthTicketStore : public AuthTicketStore
{
public:
    optional<string> Get(const string& key) override;
    bool Delete(const string& key) override;
};

/**
 * 인증 서버가 월드 선택에서 발급한 인증 티켓.
 *
 * 티켓을 한 번만 소비해 그 티켓이 가리키는 계정을 알려 준다. 키와 값의 형식은 인증 서버와의 계약이다.
 */
namespace AuthTicket
{
    /** worldId 월드에 발급한 티켓의 키. */
    string MakeKey(int32 worldId, const string& ticket);

    /** 티켓 값에서 계정 번호를 읽는다. 형식이 틀리면 nullopt. 어느 스레드에서 불러도 된다. */
    optional<int64> ParseAccountId(const string& value);

    /**
     * worldId 월드에 발급한 티켓을 지우고 그 계정 번호를 돌려준다. 다른 월드의 티켓, 없거나 이미 쓰인 티켓,
     * 값의 형식이 틀린 티켓은 nullopt. 같은 티켓을 여러 스레드가 함께 소비해도 하나만 계정 번호를 받는다.
     */
    optional<int64> Consume(AuthTicketStore& store, int32 worldId, const string& ticket);
}
